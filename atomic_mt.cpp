#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <random>
#include <iomanip>
#include <mutex>
#include "orderbook.h"
#include "order.h"

// Global counter for order IDs
std::atomic<uint64_t> nextOrderId(1);
std::mutex coutMutex;  // Mutex for thread-safe console output

// Helper function to create random orders
std::shared_ptr<Order> createRandomOrder(const std::string& symbol, unsigned int seed) {
    std::mt19937 rng(seed);
    std::uniform_int_distribution<> sideDist(0, 1);
    std::uniform_int_distribution<> typeDist(0, 1); // We'll stick to LIMIT and MARKET orders
    std::uniform_real_distribution<> priceDist(90.0, 110.0);
    std::uniform_real_distribution<> qtyDist(1.0, 10.0);
    
    Side side = sideDist(rng) == 0 ? Side::BUY : Side::SELL;
    OrderType type = typeDist(rng) == 0 ? OrderType::LIMIT : OrderType::MARKET;
    double price = std::round(priceDist(rng) * 100) / 100.0;  // Round to 2 decimal places
    double quantity = std::round(qtyDist(rng) * 100) / 100.0; // Round to 2 decimal places
    
    return std::make_shared<Order>(nextOrderId++, side, type, symbol, price, quantity);
}

// Thread-safe callback for order fills
void logFill(const std::string& symbol, double price, double quantity, double side) {
    std::lock_guard<std::mutex> lock(coutMutex);
    std::cout << "FILL: " << symbol << " " 
              << (side > 0 ? "BUY" : "SELL") << " " 
              << std::fixed << std::setprecision(2) << quantity << " @ " 
              << price << std::endl;
}

// Function to process orders for a symbol
void processSymbolOrders(const std::string& symbol, int numOrders) {
    // Create an order book for this symbol
    OrderBook orderBook(symbol);
    
    {
        std::lock_guard<std::mutex> lock(coutMutex);
        std::cout << "Processing " << numOrders << " orders for symbol: " << symbol << std::endl;
    }
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Process orders
    for (int i = 0; i < numOrders; ++i) {
        // Use deterministic seed for reproducible tests and easier debugging
        auto order = createRandomOrder(symbol, i + 1000);
        
        // Create a thread-local callback to avoid capturing external references
        auto callback = [symbol](const std::string&, double price, double qty, double side) {
            std::lock_guard<std::mutex> lock(coutMutex);
            std::cout << "FILL: " << symbol << " " 
                    << (side > 0 ? "BUY" : "SELL") << " " 
                    << std::fixed << std::setprecision(2) << qty << " @ " 
                    << price << std::endl;
        };
        
        orderBook.addOrder(order, callback);
        
        // Occasionally cancel an order (5% chance)
        if (i % 20 == 0 && i > 0) {  // Simple deterministic approach
            Order::OrderID idToCancel = static_cast<Order::OrderID>(i-10);
            orderBook.cancelOrder(idToCancel);
        }
        
        // Small sleep to reduce contention between threads
        if (i % 100 == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    
    {
        std::lock_guard<std::mutex> lock(coutMutex);
        std::cout << "Symbol " << symbol << " processed " << numOrders 
                  << " orders in " << duration << " ms" 
                  << " (" << (numOrders * 1000.0 / (duration + 1)) << " orders/sec)" << std::endl;
    }
}

int main() {
    std::cout << "Starting multi-threaded order book test with atomic operations..." << std::endl;
    
    // Define symbols to use
    std::vector<std::string> symbols = {"AAPL", "MSFT", "GOOG", "AMZN", "FB"};
    
    // Number of orders to process per symbol
    const int ordersPerSymbol = 200;  // Reduced for testing
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Create threads for each symbol
    std::vector<std::thread> threads;
    for (const auto& symbol : symbols) {
        threads.emplace_back(processSymbolOrders, symbol, ordersPerSymbol);
        // Small delay to stagger thread startup
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Join all threads
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    
    // Calculate total orders processed
    int totalOrders = symbols.size() * ordersPerSymbol;
    
    std::cout << "\nProcessed " << totalOrders << " orders across " 
              << symbols.size() << " symbols in " << duration << " ms" << std::endl;
    std::cout << "Average throughput: " << (totalOrders * 1000.0 / (duration + 1)) 
              << " orders per second" << std::endl;
    
    std::cout << "Test completed successfully." << std::endl;
    return 0;
}