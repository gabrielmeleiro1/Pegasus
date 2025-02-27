#include <iostream>
#include <string>
#include <vector>
#include <atomic>
#include <mutex>
#include <thread>
#include <chrono>
#include "orderbook.h"

// Thread-safe order counter
std::atomic<int> orderCount{0};

// Used to ensure console output doesn't get garbled
std::mutex consoleMutex;

// Function to run in each thread
void processSymbolOrders(const std::string& symbol) {
    // Create an order book for this symbol
    OrderBook book(symbol);
    
    // Simple set of test orders with predictable IDs
    const int numOrders = 10;
    
    {
        std::lock_guard<std::mutex> lock(consoleMutex);
        std::cout << "Thread for " << symbol << " started" << std::endl;
    }
    
    // Add a set of buy orders at different price levels
    for (int i = 0; i < numOrders; i++) {
        auto id = orderCount.fetch_add(1) + 1;
        auto order = std::make_shared<Order>(id, Side::BUY, OrderType::LIMIT, symbol, 100.0 - i, 1.0);
        book.addOrder(order);
    }
    
    // Add a set of sell orders at different price levels
    for (int i = 0; i < numOrders; i++) {
        auto id = orderCount.fetch_add(1) + 1;
        auto order = std::make_shared<Order>(id, Side::SELL, OrderType::LIMIT, symbol, 101.0 + i, 1.0);
        book.addOrder(order);
    }
    
    // Add a matching order that should execute
    {
        auto id = orderCount.fetch_add(1) + 1;
        auto order = std::make_shared<Order>(id, Side::BUY, OrderType::LIMIT, symbol, 101.0, 1.0);
        
        book.addOrder(order, [&](const std::string& sym, double price, double qty, double side) {
            std::lock_guard<std::mutex> lock(consoleMutex);
            std::cout << "FILL for " << sym << ": " 
                      << (side > 0 ? "BUY" : "SELL") << " " 
                      << qty << " @ " << price << std::endl;
        });
    }
    
    {
        std::lock_guard<std::mutex> lock(consoleMutex);
        std::cout << "Thread for " << symbol << " completed" << std::endl;
    }
}

int main() {
    std::cout << "Starting multi-threaded order book demo" << std::endl;
    
    // Define symbols
    std::vector<std::string> symbols = {"AAPL", "MSFT", "GOOG", "AMZN", "FB"};
    
    // Create threads
    std::vector<std::thread> threads;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    for (const auto& symbol : symbols) {
        threads.emplace_back(processSymbolOrders, symbol);
    }
    
    // Wait for all threads to finish
    for (auto& thread : threads) {
        thread.join();
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    
    std::cout << "All threads completed in " << duration << " ms" << std::endl;
    std::cout << "Processed " << orderCount.load() << " orders across " 
              << symbols.size() << " symbols" << std::endl;
    
    return 0;
}