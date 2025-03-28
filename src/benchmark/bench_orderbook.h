#ifndef BENCH_ORDERBOOK_H
#define BENCH_ORDERBOOK_H

#include "orderbook.h"
#include "order.h"
#include <unordered_map>
#include <memory>
#include <functional>
#include <string>
#include <mutex>

namespace pegasus {

/**
 * @brief Simplified OrderBookManager for benchmarking purposes
 * 
 * This class provides a minimal interface for managing multiple order books
 * without the threading complexity of the full OrderBookManager.
 */
class BenchOrderBookManager {
public:
    BenchOrderBookManager() {}
    
    // Create a new order book for a symbol
    void createOrderBook(const std::string& symbol) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_orderBooks.find(symbol) == m_orderBooks.end()) {
            m_orderBooks[symbol] = std::make_unique<OrderBook>(symbol);
        }
    }
    
    // Add an order to the appropriate order book
    bool addOrder(std::shared_ptr<Order> order, 
                  const std::function<void(const std::string&, double, double, double)>& fillCallback) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_orderBooks.find(order->getSymbol());
        if (it == m_orderBooks.end()) {
            createOrderBook(order->getSymbol());
            it = m_orderBooks.find(order->getSymbol());
        }
        
        return it->second->addOrder(order, fillCallback);
    }
    
    // Cancel an order in the appropriate order book
    bool cancelOrder(Order::OrderID orderId) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        // Try to cancel the order in each order book
        for (auto& [_, book] : m_orderBooks) {
            if (book->cancelOrder(orderId)) {
                return true;
            }
        }
        
        return false;
    }
    
    // Get the order book for a specific symbol
    OrderBook* getOrderBook(const std::string& symbol) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_orderBooks.find(symbol);
        if (it != m_orderBooks.end()) {
            return it->second.get();
        }
        return nullptr;
    }

private:
    // Map of symbols to their OrderBook instances
    std::unordered_map<std::string, std::unique_ptr<OrderBook>> m_orderBooks;
    
    // Mutex for thread safety
    std::mutex m_mutex;
};

} // namespace pegasus

#endif // BENCH_ORDERBOOK_H