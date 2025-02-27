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

// Simple thread-safe function to process orders sequentially
void processOrders(const std::string& symbol, int numOrders) {
    OrderBook orderBook(symbol);
    
    {
        std::lock_guard<std::mutex> lock(coutMutex);
        std::cout << "Thread " << std::this_thread::get_id() << " processing " 
                  << numOrders << " orders for " << symbol << std::endl;
    }
    
    for (int i = 0; i < numOrders; i++) {
        auto order = createRandomOrder(symbol, i + 1000);
        orderBook.addOrder(order);
        
        if (i % 100 == 0) {
            std::lock_guard<std::mutex> lock(coutMutex);
            std::cout << "Thread " << std::this_thread::get_id() << " processed " 
                      << i << " orders for " << symbol << std::endl;
        }
    }
    
    {
        std::lock_guard<std::mutex> lock(coutMutex);
        std::cout << "Thread " << std::this_thread::get_id() << " completed " 
                  << numOrders << " orders for " << symbol << std::endl;
    }
}

int main() {
    std::cout << "Starting basic multi-threaded test..." << std::endl;
    
    std::vector<std::string> symbols = {"AAPL", "MSFT", "GOOG", "AMZN", "FB"};
    const int ordersPerSymbol = 100;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Launch one thread per symbol
    std::vector<std::thread> threads;
    for (const auto& symbol : symbols) {
        threads.emplace_back(processOrders, symbol, ordersPerSymbol);
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Stagger starts
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    
    std::cout << "All threads completed processing " << (symbols.size() * ordersPerSymbol) 
              << " orders in " << duration << " ms" << std::endl;
    
    return 0;
}