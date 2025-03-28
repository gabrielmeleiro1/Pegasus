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

// Mutex for thread-safe access to console output
std::mutex consoleMutex;

// Mutex for shared data access
std::mutex dataMutex;

// Market data for ETH/USD with price simulation parameters
class CryptoMarket {
public:
    static constexpr const char* SYMBOL = "ETH/USD";
    static constexpr double BASE_PRICE = 3200.0;    // Central tendency for mean reversion
    static constexpr double PRICE_VOLATILITY = 50.0; // Higher volatility for faster price movement
    static constexpr double MEAN_REVERSION = 0.03;  // Reduced mean reversion for wilder price swings
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
    // Use mutex to protect market price updates
    std::lock_guard<std::mutex> lock(dataMutex);
    
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
    // Get a copy of current mid price with thread safety
    double currentMidPrice;
    {
        std::lock_guard<std::mutex> lock(dataMutex);
        currentMidPrice = CryptoMarket::currentMidPrice;
    }
    
    // Generate random order properties
    std::uniform_int_distribution<> sideDist(0, 1);
    std::uniform_real_distribution<> qtyDist(CryptoMarket::MIN_QTY, CryptoMarket::MAX_QTY);
    
    // Determine side (buy or sell)
    if (!forceSide) {
        side = sideDist(rng) == 0 ? Side::BUY : Side::SELL;
    }
    
    // Price distribution depends on order side (clustering around mid price)
    double priceMean = currentMidPrice;
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
    
    // Calculate price limits based on the current mid price
    double minPrice = currentMidPrice - 100.0;
    double maxPrice = currentMidPrice + 100.0;
    
    // Ensure price is within limits
    rawPrice = std::max(rawPrice, minPrice);
    rawPrice = std::min(rawPrice, maxPrice);
    
    double price = std::round(rawPrice / CryptoMarket::PRICE_STEP) * CryptoMarket::PRICE_STEP;
    
    // Generate quantity rounded to 0.01 ETH
    double quantity = std::round(qtyDist(rng) * 100) / 100.0;
    
    // Create order with thread-safe ID increment
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

// Handle order fills
void logFill(const std::string& symbol, double price, double quantity, double side) {
    std::lock_guard<std::mutex> lock(consoleMutex);
    std::cout << "TRADE: " << symbol << " " 
              << (side > 0 ? "BUY" : "SELL") << " " 
              << std::fixed << std::setprecision(3) << quantity << " ETH @ " 
              << std::setprecision(2) << price << " USD" << std::endl;
}

// Clear the console screen
void clearScreen() {
    // Use lock-free approach for screen clearing to avoid potential deadlocks
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

// Print the order book in a nicely formatted way
// Always display exactly 10 price levels on each side for consistent layout
void printOrderBook(OrderBook& book, int levels = 10) {
    // Get current mid price with thread safety
    double currentMidPrice;
    
    {
        std::lock_guard<std::mutex> lock(dataMutex);
        currentMidPrice = CryptoMarket::currentMidPrice;
    }

    // Collection of book data
    std::vector<BookLevel> asks;
    std::vector<BookLevel> bids;
    
    try {
        // Collect ask levels (around current mid price)
        double askTotal = 0.0;
        double askValueTotal = 0.0;
        
        // For each ask level, look within a certain range of the current price
        double startAskPrice = currentMidPrice - 50.0;  // Look below current price
        double endAskPrice = currentMidPrice + 150.0;   // Look above current price
        
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
        double bidTotal = 0.0;
        double bidValueTotal = 0.0;
        
        // For each bid level, look within a certain range of the current price
        double startBidPrice = currentMidPrice + 50.0;  // Look above current price
        double endBidPrice = currentMidPrice - 150.0;   // Look below current price
        
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
    } catch (const std::exception& e) {
        std::cerr << "Error collecting order book data: " << e.what() << std::endl;
    }

    // Now display the book with console lock
    try {
        std::lock_guard<std::mutex> consoleLock(consoleMutex);
        
        std::cout << BOLD << "===== " << CryptoMarket::SYMBOL << " ORDER BOOK =====" << RESET << std::endl;
        
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
        
        // Always display exactly 'levels' rows for asks, regardless of how many we have
        int displayed = 0;
        for (size_t i = 0; i < asks.size() && displayed < levels; i++) {
            std::cout << RED << std::left << std::setw(priceWidth) << formatWithCommas(asks[i].price, 1)
                      << std::right << std::setw(qtyWidth) << formatWithCommas(asks[i].quantity, 3)
                      << std::right << std::setw(totalQtyWidth) << formatWithCommas(runningAsksQty[i], 3)
                      << std::right << std::setw(totalValueWidth) << formatWithCommas(runningAsksValue[i], 1)
                      << RESET << std::endl;
            displayed++;
        }
        
        // Add empty rows if we don't have enough ask levels
        for (int i = displayed; i < levels; i++) {
            std::cout << RED << std::left << std::setw(priceWidth) << "-"
                      << std::right << std::setw(qtyWidth) << "-"
                      << std::right << std::setw(totalQtyWidth) << "-"
                      << std::right << std::setw(totalValueWidth) << "-"
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
        
        // Always display exactly 'levels' rows for bids, regardless of how many we have
        int displayedBids = 0;
        for (size_t i = 0; i < bids.size() && displayedBids < levels; i++) {
            std::cout << GREEN << std::left << std::setw(priceWidth) << formatWithCommas(bids[i].price, 1)
                      << std::right << std::setw(qtyWidth) << formatWithCommas(bids[i].quantity, 3)
                      << std::right << std::setw(totalQtyWidth) << formatWithCommas(runningBidsQty[i], 3)
                      << std::right << std::setw(totalValueWidth) << formatWithCommas(runningBidsValue[i], 1)
                      << RESET << std::endl;
            displayedBids++;
        }
        
        // Add empty rows if we don't have enough bid levels
        for (int i = displayedBids; i < levels; i++) {
            std::cout << GREEN << std::left << std::setw(priceWidth) << "-"
                      << std::right << std::setw(qtyWidth) << "-"
                      << std::right << std::setw(totalQtyWidth) << "-"
                      << std::right << std::setw(totalValueWidth) << "-"
                      << RESET << std::endl;
        }
        
        std::cout << std::string(priceWidth + qtyWidth + totalQtyWidth + totalValueWidth, '=') << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error displaying order book: " << e.what() << std::endl;
    }
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
    
    // Create 200 random buy orders for deep liquidity
    for (int i = 0; i < 200; i++) {
        auto order = generateRandomOrder(rng, true, Side::BUY);
        book.addOrder(order, logFill);
    }
    
    // Create 200 random sell orders for deep liquidity
    for (int i = 0; i < 200; i++) {
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
    
    // Order tracking counters
    uint64_t ordersReceived = 400; // Start at 400 since we added 400 initial orders
    uint64_t ordersFilled = 0;
    
    // Simulation loop - runs until user interrupts
    std::cout << "\nStarting infinite simulation (Ctrl+C to exit)...\n" << std::endl;
    
    int step = 0;
    int maxSteps = 10000; // Increase maximum steps (effectively infinite)
    
    while (step < maxSteps) {
        try {
            step++;
            
            // Update market price with random walk
            updateMarketPrice(rng);
            
            // Update high/low prices
            {
                std::lock_guard<std::mutex> lock(dataMutex);
                highPrice = std::max(highPrice, CryptoMarket::currentMidPrice);
                lowPrice = std::min(lowPrice, CryptoMarket::currentMidPrice);
            }
            
            // Clear screen for better visualization
            clearScreen();
            
            // Print order book first at top of screen so it stays in a fixed position
            printOrderBook(book, 10);
            
            // Show market statistics with mutex protection below the order book
            {
                std::lock_guard<std::mutex> lock(consoleMutex);
                std::cout << std::string(60, '=') << std::endl;
                std::cout << "===== ETH/USD MARKET STATISTICS =====" << std::endl;
                std::cout << "Step: " << step << " of " << maxSteps << std::endl;
                std::cout << "Current Mid Price: $" << std::fixed << std::setprecision(2) 
                          << CryptoMarket::currentMidPrice << std::endl;
                std::cout << "24h Range: $" << lowPrice << " - $" << highPrice << std::endl;
                std::cout << "Trades: " << tradeCount << " | Volume: " 
                          << std::setprecision(3) << totalVolume << " ETH" << std::endl;
                std::cout << "Orders Received: " << ordersReceived << " | Orders Filled: " << ordersFilled 
                          << " | Fill Rate: " << std::fixed << std::setprecision(2) 
                          << (ordersReceived > 0 ? (static_cast<double>(ordersFilled) / ordersReceived) * 100.0 : 0.0) << "%" << std::endl;
                std::cout << std::string(60, '-') << std::endl;
                std::cout << "RECENT MARKET ACTIVITY:" << std::endl;
            }
            
            // Add large number of orders per step (15-30) for fast market simulation
            std::uniform_int_distribution<> numOrdersDist(15, 30);
            int numOrders = numOrdersDist(rng);
            
            // Bias toward even more orders in volatile markets
            double volatilityFactor;
            {
                std::lock_guard<std::mutex> lock(dataMutex);
                volatilityFactor = std::abs(CryptoMarket::currentMidPrice - CryptoMarket::BASE_PRICE) 
                                     / CryptoMarket::PRICE_VOLATILITY;
            }
            
            if (volatilityFactor > 1.0 && rand() % 3 == 0) {
                numOrders += 10;  // Add many more orders in volatile periods
            }
            
            // Update orders received counter
            ordersReceived += numOrders;
            
            for (int i = 0; i < numOrders; i++) {
                auto order = generateRandomOrder(rng);
                
                // Only log a sample of orders to reduce console output and increase performance
                if (i % 10 == 0) { // Log only 1 out of every 10 orders
                    std::lock_guard<std::mutex> lock(consoleMutex);
                    std::cout << "New " << (order->getSide() == Side::BUY ? "BUY" : "SELL")
                              << " order: " << std::fixed << std::setprecision(3) 
                              << order->getQuantity() << " ETH @ $" 
                              << std::setprecision(2) << order->getPrice() << std::endl;
                }
                
                // Add the order and track fills through callback
                book.addOrder(order, [&tradeCount, &totalVolume, &ordersFilled](const std::string& symbol, 
                                                               double price, 
                                                               double quantity, 
                                                               double side) {
                    // Update trading statistics with mutex protection
                    {
                        std::lock_guard<std::mutex> lock(consoleMutex);
                        tradeCount++;
                        totalVolume += quantity;
                        ordersFilled++; // Increment fill counter
                    }
                    
                    // Log only some trades to reduce console output and increase performance
                    // Use the price as a pseudo-random value for sampling trades to log
                    if (static_cast<int>(price * 10) % 5 == 0) { // Log roughly 20% of trades
                        std::lock_guard<std::mutex> lock(consoleMutex);
                        std::cout << "TRADE: " << symbol << " " 
                                  << (side > 0 ? "BUY" : "SELL") << " " 
                                  << std::fixed << std::setprecision(3) << quantity << " ETH @ " 
                                  << std::setprecision(2) << price << " USD" << std::endl;
                    }
                });
            }
            
            // Cancel orders every step (not just occasionally)
            // Try to cancel many random old orders (5-15 per step)
            if (nextOrderId > 10) {
                std::uniform_int_distribution<> numCancelsDist(5, 15);
                int numCancels = numCancelsDist(rng);
                
                for (int i = 0; i < numCancels; i++) {
                    std::uniform_int_distribution<> idDist(1, nextOrderId-1);
                    Order::OrderID idToCancel = idDist(rng);
                    book.cancelOrder(idToCancel);
                }
            }
            
            // Use a moderate delay between steps (80ms) - fast but not too fast
            std::this_thread::sleep_for(std::chrono::milliseconds(80));
        }
        catch (const std::exception& e) {
            std::cerr << "Error in simulation loop: " << e.what() << std::endl;
            break;
        }
    }
    
    return 0;
}