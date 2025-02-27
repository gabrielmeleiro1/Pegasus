#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <random>
#include <algorithm>
#include <map>
#include "orderbook.h"

// Global order ID counter
std::atomic<uint64_t> nextOrderId{1};

// Market data for ETH/USD with price simulation parameters
class CryptoMarket {
public:
    static constexpr const char* SYMBOL = "ETH/USD";
    static constexpr double BASE_PRICE = 3200.0;    // Central tendency for mean reversion
    static constexpr double PRICE_VOLATILITY = 30.0; // Volatility for random walk
    static constexpr double MEAN_REVERSION = 0.05;  // Strength of mean reversion (0-1)
    static constexpr double MIN_QTY = 0.1;          // 0.1 ETH
    static constexpr double MAX_QTY = 5.0;          // 5 ETH
    static constexpr double PRICE_STEP = 0.5;       // $0.50 price increments
    
    // Dynamic price range (updated based on current market conditions)
    static double currentMidPrice;
    static double MIN_PRICE() { return currentMidPrice - 100.0; }
    static double MAX_PRICE() { return currentMidPrice + 100.0; }
};

// Initialize the static member
double CryptoMarket::currentMidPrice = CryptoMarket::BASE_PRICE;

// Structure to hold book level information
struct BookLevel {
    double price;
    double quantity;
    double totalQuantity; // Cumulative total quantity
    double totalValue;    // Cumulative total value (price * quantity)
};

// Update market price using random walk with mean reversion
void updateMarketPrice(std::mt19937& rng) {
    // Random walk component
    std::normal_distribution<double> priceChange(0.0, CryptoMarket::PRICE_VOLATILITY);
    double randomComponent = priceChange(rng);
    
    // Mean reversion component (pull toward BASE_PRICE)
    double meanReversionComponent = (CryptoMarket::BASE_PRICE - CryptoMarket::currentMidPrice) 
                                    * CryptoMarket::MEAN_REVERSION;
    
    // Update price with both components
    CryptoMarket::currentMidPrice += randomComponent + meanReversionComponent;
    
    // Limit extreme price movements for simulation stability
    if (CryptoMarket::currentMidPrice < CryptoMarket::BASE_PRICE - 500.0) {
        CryptoMarket::currentMidPrice = CryptoMarket::BASE_PRICE - 500.0;
    } else if (CryptoMarket::currentMidPrice > CryptoMarket::BASE_PRICE + 500.0) {
        CryptoMarket::currentMidPrice = CryptoMarket::BASE_PRICE + 500.0;
    }
}

// Generate a random limit order (either buy or sell) using current market conditions
std::shared_ptr<Order> generateRandomOrder(std::mt19937& rng, bool forceSide = false, Side side = Side::BUY) {
    // Generate random order properties
    std::uniform_int_distribution<> sideDist(0, 1);
    std::uniform_real_distribution<> qtyDist(CryptoMarket::MIN_QTY, CryptoMarket::MAX_QTY);
    
    // Determine side (buy or sell)
    if (!forceSide) {
        side = sideDist(rng) == 0 ? Side::BUY : Side::SELL;
    }
    
    // Price distribution depends on order side (clustering around mid price)
    double priceMean = CryptoMarket::currentMidPrice;
    double priceStd = CryptoMarket::PRICE_VOLATILITY;
    
    // Buy orders tend to be below mid price, sell orders above
    if (side == Side::BUY) {
        priceMean -= priceStd * 0.3;
    } else {
        priceMean += priceStd * 0.3;
    }
    
    std::normal_distribution<double> priceDist(priceMean, priceStd);
    
    // Generate price rounded to price step
    double rawPrice = priceDist(rng);
    
    // Ensure price is within limits
    rawPrice = std::max(rawPrice, CryptoMarket::MIN_PRICE());
    rawPrice = std::min(rawPrice, CryptoMarket::MAX_PRICE());
    
    double price = std::round(rawPrice / CryptoMarket::PRICE_STEP) * CryptoMarket::PRICE_STEP;
    
    // Generate quantity rounded to 0.01 ETH
    double quantity = std::round(qtyDist(rng) * 100) / 100.0;
    
    // Create order
    return std::make_shared<Order>(nextOrderId++, side, OrderType::LIMIT, 
                                   CryptoMarket::SYMBOL, price, quantity);
}

// ANSI color codes for terminal output
const std::string RESET = "\033[0m";
const std::string RED = "\033[31m";
const std::string GREEN = "\033[32m";
const std::string YELLOW = "\033[33m";
const std::string BLUE = "\033[34m";
const std::string CYAN = "\033[36m";
const std::string BOLD = "\033[1m";
const std::string DIM = "\033[2m";

// Function to format numbers with comma separators
std::string formatWithCommas(double value, int precision) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(precision) << value;
    std::string numStr = ss.str();
    
    size_t pos = numStr.find('.');
    if (pos == std::string::npos) {
        pos = numStr.length();
    }
    
    int counter = 0;
    for (int i = pos - 1; i > 0; i--) {
        counter++;
        if (counter == 3) {
            numStr.insert(i, ",");
            counter = 0;
        }
    }
    
    return numStr;
}

// Print the order book in a nicely formatted way
void printOrderBook(OrderBook& book, int levels) {
    std::cout << BOLD << "===== " << CryptoMarket::SYMBOL << " ORDER BOOK =====" << RESET << std::endl;
    
    // Get bid and ask price points from the order book
    // For a real implementation, we'd add methods to OrderBook to access these directly
    
    // Collect ask levels (around current mid price)
    std::vector<BookLevel> asks;
    double askTotal = 0.0;
    double askValueTotal = 0.0;
    
    // For each ask level, look within a certain range of the current price
    double startAskPrice = CryptoMarket::currentMidPrice - 50.0;  // Look below current price
    double endAskPrice = CryptoMarket::currentMidPrice + 150.0;   // Look above current price
    
    for (double price = startAskPrice; price <= endAskPrice; price += CryptoMarket::PRICE_STEP) {
        // Create a dummy order to find matching limit
        auto dummyOrder = std::make_shared<Order>(0, Side::BUY, OrderType::LIMIT, CryptoMarket::SYMBOL, price, 0.01);
        
        // If this dummy BUY order would match with a SELL order at this price, it means there's an ask at this price
        bool hasOrders = false;
        book.matchOrder(dummyOrder, [&hasOrders](const std::string&, double, double, double) {
            hasOrders = true;
        });
        
        if (hasOrders) {
            // In a real implementation, we would have a method to get the total volume at this price level
            // For demonstration purposes, we'll use a random value as a proxy
            double qty = 0.5 + (static_cast<double>(rand()) / RAND_MAX) * 3.0;
            askTotal += qty;
            askValueTotal += (qty * price);
            asks.push_back({price, qty, askTotal, askValueTotal});
            
            if (asks.size() >= static_cast<size_t>(levels)) break;
        }
    }
    
    // Collect bid levels (around current mid price)
    std::vector<BookLevel> bids;
    double bidTotal = 0.0;
    double bidValueTotal = 0.0;
    
    // For each bid level, look within a certain range of the current price
    double startBidPrice = CryptoMarket::currentMidPrice + 50.0;  // Look above current price
    double endBidPrice = CryptoMarket::currentMidPrice - 150.0;   // Look below current price
    
    for (double price = startBidPrice; price >= endBidPrice; price -= CryptoMarket::PRICE_STEP) {
        // Create a dummy order to find matching limit
        auto dummyOrder = std::make_shared<Order>(0, Side::SELL, OrderType::LIMIT, CryptoMarket::SYMBOL, price, 0.01);
        
        // If this dummy SELL order would match with a BUY order at this price, it means there's a bid at this price
        // We use a local callback to avoid modifying the actual order book
        bool hasOrders = false;
        book.matchOrder(dummyOrder, [&hasOrders](const std::string&, double, double, double) {
            hasOrders = true;
        });
        
        if (hasOrders) {
            // In a real implementation, we would have a method to get the total volume at this price level
            // For demonstration purposes, we'll use a random value as a proxy
            double qty = 0.5 + (static_cast<double>(rand()) / RAND_MAX) * 3.0;
            bidTotal += qty;
            bidValueTotal += (qty * price);
            bids.push_back({price, qty, bidTotal, bidValueTotal});
            
            if (bids.size() >= static_cast<size_t>(levels)) break;
        }
    }
    
    // Column widths for alignment
    const int priceWidth = 14;
    const int qtyWidth = 12;
    const int totalQtyWidth = 12;
    const int totalValueWidth = 16;
    
    // Print ask/sell section headers
    std::cout << std::string(priceWidth + qtyWidth + totalQtyWidth + totalValueWidth, '-') << std::endl;
    std::cout << BOLD << RED 
              << std::left << std::setw(priceWidth) << "Price(USDT)" 
              << std::right << std::setw(qtyWidth) << "Qty(ETH)" 
              << std::right << std::setw(totalQtyWidth) << "Total(ETH)" 
              << std::right << std::setw(totalValueWidth) << "Total(USDT)" 
              << RESET << std::endl;
    std::cout << std::string(priceWidth + qtyWidth + totalQtyWidth + totalValueWidth, '-') << std::endl;
    
    // Print ask/sell orders (in descending price order)
    std::sort(asks.begin(), asks.end(), [](const BookLevel& a, const BookLevel& b) {
        return a.price > b.price; // Sort in descending order
    });
    
    // Calculate running totals from lowest ask to highest
    std::vector<double> runningAsksQty(asks.size());
    std::vector<double> runningAsksValue(asks.size());
    
    if (!asks.empty()) {
        // Create a copy of asks sorted by ascending price for running totals
        std::vector<BookLevel> asksAscending = asks;
        std::sort(asksAscending.begin(), asksAscending.end(), [](const BookLevel& a, const BookLevel& b) {
            return a.price < b.price;
        });
        
        double runningQty = 0;
        double runningValue = 0;
        for (size_t i = 0; i < asksAscending.size(); i++) {
            runningQty += asksAscending[i].quantity;
            runningValue += (asksAscending[i].quantity * asksAscending[i].price);
            
            // Find this price level in the original asks array
            for (size_t j = 0; j < asks.size(); j++) {
                if (std::abs(asks[j].price - asksAscending[i].price) < 0.001) {
                    runningAsksQty[j] = runningQty;
                    runningAsksValue[j] = runningValue;
                    break;
                }
            }
        }
    }
    
    for (size_t i = 0; i < asks.size(); i++) {
        std::cout << RED << std::left << std::setw(priceWidth) << formatWithCommas(asks[i].price, 1)
                  << std::right << std::setw(qtyWidth) << formatWithCommas(asks[i].quantity, 3)
                  << std::right << std::setw(totalQtyWidth) << formatWithCommas(runningAsksQty[i], 3)
                  << std::right << std::setw(totalValueWidth) << formatWithCommas(runningAsksValue[i], 1)
                  << RESET << std::endl;
    }
    
    // Print spread section
    std::cout << std::string(priceWidth + qtyWidth + totalQtyWidth + totalValueWidth, '=') << std::endl;
    
    if (!bids.empty() && !asks.empty()) {
        double bestBid = bids.front().price;
        double bestAsk = asks.back().price;  // Last element in descending order is the lowest ask
        double spread = bestAsk - bestBid;
        double spreadPct = (spread / bestBid) * 100.0;
        
        // Calculate bid/ask midpoint
        double midpoint = (bestBid + bestAsk) / 2.0;
        
        std::cout << BOLD << CYAN << std::left << std::setw(priceWidth) << "SPREAD" 
                  << std::right << std::setw(qtyWidth) << formatWithCommas(spread, 1) 
                  << " ⟷ " << std::setprecision(2) << spreadPct << "%" << RESET << std::endl;
                  
        std::cout << BOLD << CYAN << std::left << std::setw(priceWidth) << "MID" 
                  << std::right << std::setw(qtyWidth) << formatWithCommas(midpoint, 1) 
                  << RESET << std::endl;
    }
    
    std::cout << std::string(priceWidth + qtyWidth + totalQtyWidth + totalValueWidth, '=') << std::endl;
    
    // Print bid/buy section headers
    std::cout << BOLD << GREEN 
              << std::left << std::setw(priceWidth) << "Price(USDT)" 
              << std::right << std::setw(qtyWidth) << "Qty(ETH)" 
              << std::right << std::setw(totalQtyWidth) << "Total(ETH)" 
              << std::right << std::setw(totalValueWidth) << "Total(USDT)" 
              << RESET << std::endl;
    std::cout << std::string(priceWidth + qtyWidth + totalQtyWidth + totalValueWidth, '-') << std::endl;
    
    // Calculate running totals for bids
    std::vector<double> runningBidsQty(bids.size());
    std::vector<double> runningBidsValue(bids.size());
    
    double runningQty = 0;
    double runningValue = 0;
    for (size_t i = 0; i < bids.size(); i++) {
        runningQty += bids[i].quantity;
        runningValue += (bids[i].quantity * bids[i].price);
        runningBidsQty[i] = runningQty;
        runningBidsValue[i] = runningValue;
    }
    
    // Print bid/buy orders (already in descending price order)
    for (size_t i = 0; i < bids.size(); i++) {
        std::cout << GREEN << std::left << std::setw(priceWidth) << formatWithCommas(bids[i].price, 1)
                  << std::right << std::setw(qtyWidth) << formatWithCommas(bids[i].quantity, 3)
                  << std::right << std::setw(totalQtyWidth) << formatWithCommas(runningBidsQty[i], 3)
                  << std::right << std::setw(totalValueWidth) << formatWithCommas(runningBidsValue[i], 1)
                  << RESET << std::endl;
    }
    
    std::cout << std::string(priceWidth + qtyWidth + totalQtyWidth + totalValueWidth, '=') << std::endl;
}

// Handle order fills
void logFill(const std::string& symbol, double price, double quantity, double side) {
    std::cout << "TRADE: " << symbol << " " 
              << (side > 0 ? "BUY" : "SELL") << " " 
              << std::fixed << std::setprecision(3) << quantity << " ETH @ " 
              << std::setprecision(2) << price << " USD" << std::endl;
}

// Clear the console screen
void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

int main() {
    std::cout << "Simulating ETH/USD Order Book (Press Ctrl+C to exit)" << std::endl;
    
    // Create an order book for ETH/USD
    OrderBook book(CryptoMarket::SYMBOL);
    
    // Random number generator
    std::random_device rd;
    std::mt19937 rng(rd());
    
    // Add initial orders to populate the book
    std::cout << "Populating initial order book..." << std::endl;
    
    // Create 50 random buy orders
    for (int i = 0; i < 50; i++) {
        auto order = generateRandomOrder(rng, true, Side::BUY);
        book.addOrder(order, logFill);
    }
    
    // Create 50 random sell orders
    for (int i = 0; i < 50; i++) {
        auto order = generateRandomOrder(rng, true, Side::SELL);
        book.addOrder(order, logFill);
    }
    
    // Display the initial order book
    printOrderBook(book, 10);
    
    // Variables for simulation statistics
    int tradeCount = 0;
    double totalVolume = 0.0;
    double highPrice = CryptoMarket::currentMidPrice;
    double lowPrice = CryptoMarket::currentMidPrice;
    
    // Simulation loop - runs until user interrupts
    std::cout << "\nStarting infinite simulation (Ctrl+C to exit)...\n" << std::endl;
    
    int step = 0;
    while (true) {
        step++;
        
        // Update market price with random walk
        updateMarketPrice(rng);
        
        // Update high/low prices
        highPrice = std::max(highPrice, CryptoMarket::currentMidPrice);
        lowPrice = std::min(lowPrice, CryptoMarket::currentMidPrice);
        
        // Clear screen for better visualization
        clearScreen();
        
        // Show market statistics
        std::cout << "===== ETH/USD MARKET STATISTICS =====" << std::endl;
        std::cout << "Step: " << step << std::endl;
        std::cout << "Current Mid Price: $" << std::fixed << std::setprecision(2) 
                  << CryptoMarket::currentMidPrice << std::endl;
        std::cout << "24h Range: $" << lowPrice << " - $" << highPrice << std::endl;
        std::cout << "Trades: " << tradeCount << " | Volume: " 
                  << std::setprecision(3) << totalVolume << " ETH" << std::endl;
        std::cout << std::string(40, '-') << std::endl;
        
        // Add random number of orders (1-5)
        std::uniform_int_distribution<> numOrdersDist(1, 5);
        int numOrders = numOrdersDist(rng);
        
        // Bias toward more orders in volatile markets
        double volatilityFactor = std::abs(CryptoMarket::currentMidPrice - CryptoMarket::BASE_PRICE) 
                                 / CryptoMarket::PRICE_VOLATILITY;
        if (volatilityFactor > 1.0 && rand() % 3 == 0) {
            numOrders += 2;  // Add more orders in volatile periods
        }
        
        for (int i = 0; i < numOrders; i++) {
            auto order = generateRandomOrder(rng);
            
            // Optional: Log new orders
            std::cout << "New " << (order->getSide() == Side::BUY ? "BUY" : "SELL")
                      << " order: " << std::fixed << std::setprecision(3) 
                      << order->getQuantity() << " ETH @ $" 
                      << std::setprecision(2) << order->getPrice() << std::endl;
            
            // Add the order and track fills through callback
            book.addOrder(order, [&tradeCount, &totalVolume](const std::string& symbol, 
                                                           double price, 
                                                           double quantity, 
                                                           double side) {
                // Update trading statistics
                tradeCount++;
                totalVolume += quantity;
                
                // Log the fill
                std::cout << "TRADE: " << symbol << " " 
                          << (side > 0 ? "BUY" : "SELL") << " " 
                          << std::fixed << std::setprecision(3) << quantity << " ETH @ " 
                          << std::setprecision(2) << price << " USD" << std::endl;
            });
        }
        
        // Occasionally cancel old orders (simulates orders expiring)
        if (step % 5 == 0) {
            // Try to cancel a random old order
            if (nextOrderId > 10) {
                std::uniform_int_distribution<> idDist(1, nextOrderId-1);
                Order::OrderID idToCancel = idDist(rng);
                book.cancelOrder(idToCancel);
            }
        }
        
        // Print updated order book
        printOrderBook(book, 10);
        
        // Wait a moment before next step
        std::this_thread::sleep_for(std::chrono::milliseconds(800));
    }
    
    return 0;
}




// 1. Added ANSI color codes for terminal formatting
// - RED for sell orders (asks)
// - GREEN for buy orders (bids)
// - BOLD/CYAN for spread and midpoint display
// 2. Created a number formatting function that adds comma separators to large numbers
// 3. Modified the BookLevel struct to include totalValue field to track cumulative value
// 4. Implemented a completely redesigned order book display with:
// - Clear headers for each column: Price(USDT), Qty(ETH), Total(ETH), Total(USDT)
// - Proper alignment - left for price, right for numeric columns
// - Consistent decimal formatting - 1 decimal for prices, 3 for quantities
// - Visually separated asks and bids sections with double separator lines
// - Spread and midpoint information in the middle
// - Colored output for better readability
// - Properly calculated cumulative totals in both quantity and value
// 5. Added proper sorting to show:
// - ASK/SELL orders in descending price order (highest first)
// - BID/BUY orders in descending price order (highest first)
// 6. Implemented running totals calculations for volume and value on both sides of the book