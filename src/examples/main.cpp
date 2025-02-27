#include <iostream>
#include <iomanip>
#include <vector>
#include <memory>
#include <limits>
#include "orderbook.h"

// Simple fill handler for logging trades
void logFill(Order* takerOrder, Order* makerOrder, double quantity, double price) {
    std::cout << "FILL: "
              << (takerOrder->getSide() == Side::BUY ? "BUY" : "SELL")
              << " order " << takerOrder->getID()
              << " matched with "
              << (makerOrder->getSide() == Side::BUY ? "BUY" : "SELL")
              << " order " << makerOrder->getID()
              << " for " << quantity
              << " @ $" << std::fixed << std::setprecision(2) << price
              << std::endl;
}

// Helper to print order book state
void printOrderBook(const OrderBook& book) {
    std::cout << "\n--- " << book.getSymbol() << " Order Book ---\n";
    
    // Print best bid/ask
    double bestBid = book.getBestBid();
    double bestAsk = book.getBestAsk();
    
    std::cout << "Best Bid: ";
    if (bestBid > 0.0) {
        std::cout << "$" << std::fixed << std::setprecision(2) << bestBid;
    } else {
        std::cout << "None";
    }
    
    std::cout << " | Best Ask: ";
    if (bestAsk < std::numeric_limits<double>::max()) {
        std::cout << "$" << std::fixed << std::setprecision(2) << bestAsk;
    } else {
        std::cout << "None";
    }
    
    std::cout << "\nSpread: ";
    if (bestBid > 0.0 && bestAsk < std::numeric_limits<double>::max()) {
        std::cout << "$" << std::fixed << std::setprecision(2) << (bestAsk - bestBid);
    } else {
        std::cout << "N/A";
    }
    
    std::cout << "\n--------------------------\n\n";
}

int main() {
    try {
        // Create order book for AAPL
        OrderBook book("AAPL");
    
        // Keep track of order pointers for cleanup
        std::vector<std::unique_ptr<Order>> orders;
        
        // Helper to create orders
        auto createOrder = [&orders](Order::OrderID id, Side side, OrderType type, 
                                  const std::string& symbol, double price, double quantity) {
            auto order = std::make_unique<Order>(id, side, type, symbol, price, quantity, 0.0);
            Order* ptr = order.get();
            orders.push_back(std::move(order));
            return ptr;
        };
        
        std::cout << "ORDER BOOK TEST\n";
        std::cout << "===============\n\n";
        
        // Test 1: Add limit orders to both sides
        std::cout << "Test 1: Adding limit orders\n";
        
        // Add buy orders
        book.addOrder(createOrder(1, Side::BUY, OrderType::LIMIT, "AAPL", 150.00, 100));
        book.addOrder(createOrder(2, Side::BUY, OrderType::LIMIT, "AAPL", 149.50, 200));
        book.addOrder(createOrder(3, Side::BUY, OrderType::LIMIT, "AAPL", 150.00, 50));  // Same price level
        
        // Add sell orders
        book.addOrder(createOrder(4, Side::SELL, OrderType::LIMIT, "AAPL", 150.50, 150));
        book.addOrder(createOrder(5, Side::SELL, OrderType::LIMIT, "AAPL", 151.00, 100));
        
        printOrderBook(book);
        
        // Test 2: Add a matching limit order that crosses
        std::cout << "Test 2: Adding a matching limit order\n";
        auto order6 = createOrder(6, Side::BUY, OrderType::LIMIT, "AAPL", 151.00, 120);
        book.matchOrder(order6, logFill);
        
        printOrderBook(book);
        
        // Test 3: Cancel an order
        std::cout << "Test 3: Cancelling an order\n";
        book.cancelOrder(1);  // Cancel first BUY order
        
        printOrderBook(book);
        
        // Test 4: Simple market order to buy remaining quantity
        std::cout << "Test 4: Simple market order\n";
        auto order7 = createOrder(7, Side::BUY, OrderType::MARKET, "AAPL", 0.0, 30);
        book.matchOrder(order7, logFill);
        
        printOrderBook(book);
        
        // Test 5: Add orders at multiple price levels to test book depth
        std::cout << "Test 5: Adding orders at multiple price levels\n";
        
        // Add more buy orders at different price levels
        book.addOrder(createOrder(8, Side::BUY, OrderType::LIMIT, "AAPL", 148.00, 200));
        book.addOrder(createOrder(9, Side::BUY, OrderType::LIMIT, "AAPL", 147.50, 300));
        book.addOrder(createOrder(10, Side::BUY, OrderType::LIMIT, "AAPL", 147.00, 200));
        
        // Add more sell orders at different price levels
        book.addOrder(createOrder(11, Side::SELL, OrderType::LIMIT, "AAPL", 152.00, 150));
        book.addOrder(createOrder(12, Side::SELL, OrderType::LIMIT, "AAPL", 153.00, 200));
        book.addOrder(createOrder(13, Side::SELL, OrderType::LIMIT, "AAPL", 152.50, 100));
        
        printOrderBook(book);
        
        // Test 6: Large market order that sweeps through multiple levels
        std::cout << "Test 6: Large market order sweeping multiple levels\n";
        // Use more reasonable quantity
        auto order14 = createOrder(14, Side::BUY, OrderType::MARKET, "AAPL", 0.0, 100);
        book.matchOrder(order14, logFill);
        
        printOrderBook(book);
        
        std::cout << "Tests completed successfully.\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown error occurred" << std::endl;
        return 1;
    }
}