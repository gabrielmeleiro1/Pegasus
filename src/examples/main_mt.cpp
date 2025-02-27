#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <random>
#include <iomanip>
#include "orderbookmanager.h"
#include "order.h"

// Global counter for order IDs
std::atomic<uint64_t> nextOrderId(1);

// Helper function to create random orders
std::shared_ptr<Order> createRandomOrder(const std::string& symbol, std::mt19937& rng) {
    std::uniform_int_distribution<> sideDist(0, 1);
    std::uniform_int_distribution<> typeDist(0, 1); // We'll stick to LIMIT and MARKET orders
    std::uniform_real_distribution<> priceDist(90.0, 110.0);
    std::uniform_real_distribution<> qtyDist(1.0, 10.0);
    
    Side side = sideDist(rng) == 0 ? Side::BUY : Side::SELL;
    OrderType type = typeDist(rng) == 0 ? OrderType::LIMIT : OrderType::MARKET;
    double price = std::round(priceDist(rng) * 100) / 100.0;  // Round to 2 decimal places
    double quantity = std::round(qtyDist(rng) * 100) / 100.0; // Round to 2 decimal places
    
    // Note: Symbol is now passed directly in the constructor
    return std::make_shared<Order>(nextOrderId++, side, type, symbol, price, quantity);
}

// Callback for order fills
void logFill(const std::string& symbol, double price, double quantity, double side) {
    std::cout << "FILL: " << symbol << " " 
              << (side > 0 ? "BUY" : "SELL") << " " 
              << std::fixed << std::setprecision(2) << quantity << " @ " 
              << price << std::endl;
}

int main() {
    std::cout << "Starting multi-threaded order book test..." << std::endl;
    
    // Create an OrderBookManager
    OrderBookManager manager(logFill);
    
    // Define symbols to use
    std::vector<std::string> symbols = {"AAPL", "MSFT", "GOOG", "AMZN", "FB"};
    
    // Random number generator
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution<> symbolDist(0, symbols.size() - 1);
    
    // Pre-create order books for all symbols to avoid race conditions
    for (const auto& symbol : symbols) {
        auto firstOrder = createRandomOrder(symbol, rng);
        manager.processOrder(firstOrder);
        // Small sleep to ensure thread is fully initialized
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Timing variables
    auto startTime = std::chrono::high_resolution_clock::now();
    const int numOrders = 1000;  // Reduced for testing
    
    // Create and process orders
    std::cout << "Processing " << numOrders << " random orders across " << symbols.size() << " symbols..." << std::endl;
    
    for (int i = 0; i < numOrders; ++i) {
        // Randomly select a symbol
        std::string symbol = symbols[symbolDist(rng)];
        
        // Create a random order for that symbol
        auto order = createRandomOrder(symbol, rng);
        
        // Process the order
        manager.processOrder(order);
        
        // Occasionally cancel an order (5% chance)
        std::uniform_int_distribution<> cancelDist(1, 100);
        if (cancelDist(rng) <= 5 && order->getID() > 1) {
            // Cancel a random older order
            std::uniform_int_distribution<> orderIdDist(1, order->getID() - 1);
            Order::OrderID idToCancel = orderIdDist(rng);
            manager.cancelOrder(idToCancel, symbol);
        }
        
        // Print progress every 1000 orders
        if ((i + 1) % 1000 == 0) {
            std::cout << "Processed " << (i + 1) << " orders..." << std::endl;
        }
    }
    
    // Calculate and print timing information
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    
    std::cout << "Processed " << numOrders << " orders in " << duration << " ms" << std::endl;
    std::cout << "Average orders per second: " << (numOrders * 1000.0 / duration) << std::endl;
    
    std::cout << "Shutting down manager..." << std::endl;
    
    // Shutdown the manager
    manager.shutdown();
    
    std::cout << "Test completed successfully." << std::endl;
    return 0;
}