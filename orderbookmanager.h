#ifndef ORDERBOOKMANAGER_H
#define ORDERBOOKMANAGER_H

#include "orderbook.h"
#include <unordered_map>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <atomic>
#include <memory>
#include <functional>

/**
 * @brief Enum defining the types of actions that can be performed on orders
 */
enum class OrderActionType
{
    ADD,
    CANCEL,
    SHUTDOWN
};

/**
 * @brief Structure representing an action to be performed on an order
 */
struct OrderAction
{
    OrderActionType type;
    std::shared_ptr<Order> order;
    std::function<void(const std::string&, double, double, double)> fillCallback;
    Order::OrderID orderId;
};

/**
 * @brief Manager class that handles multiple order books for different symbols
 * 
 * The OrderBookManager creates and manages independent threads for each symbol,
 * allowing parallel processing of orders for different symbols.
 */
class OrderBookManager
{
public:
    /**
     * @brief Constructor
     * 
     * @param fillCallback Callback function to be called when an order is filled
     */
    OrderBookManager(const std::function<void(const std::string&, double, double, double)>& fillCallback);
    
    /**
     * @brief Destructor, ensures all threads are properly shut down
     */
    ~OrderBookManager();
    
    /**
     * @brief Processes an order by routing it to the appropriate symbol thread
     * 
     * @param order Shared pointer to the order to process
     * @return bool True if the order was successfully queued
     */
    bool processOrder(std::shared_ptr<Order> order);
    
    /**
     * @brief Cancels an order for a specific symbol
     * 
     * @param orderId ID of the order to cancel
     * @param symbol Symbol of the order to cancel
     * @return bool True if the cancel request was successfully queued
     */
    bool cancelOrder(Order::OrderID orderId, const std::string& symbol);
    
    /**
     * @brief Gets a snapshot of the current state of the order book for a symbol
     * 
     * @param symbol Symbol to get the order book state for
     * @return std::string String representation of the order book state
     */
    std::string getOrderBookState(const std::string& symbol);
    
    /**
     * @brief Shuts down all symbol threads and the manager
     */
    void shutdown();

private:
    /**
     * @brief Main thread function for processing orders for a specific symbol
     * 
     * @param symbol Symbol this thread is responsible for
     */
    void symbolThreadFunction(const std::string& symbol);
    
    /**
     * @brief Gets or creates an OrderBook for a symbol
     * 
     * @param symbol Symbol to get the OrderBook for
     * @return OrderBook* Pointer to the OrderBook
     */
    OrderBook* getOrCreateOrderBook(const std::string& symbol);

    // Map of symbols to their OrderBook instances
    std::unordered_map<std::string, std::unique_ptr<OrderBook>> m_orderBooks;
    
    // Map of symbols to their processing threads
    std::unordered_map<std::string, std::thread> m_symbolThreads;
    
    // Map of symbols to their order action queues
    std::unordered_map<std::string, std::queue<OrderAction>> m_orderQueues;
    
    // Map of symbols to their queue mutex
    std::unordered_map<std::string, std::unique_ptr<std::mutex>> m_queueMutexes;
    
    // Map of symbols to their condition variables
    std::unordered_map<std::string, std::unique_ptr<std::condition_variable>> m_queueCondVars;
    
    // Mutex protecting the maps
    std::mutex m_mapMutex;
    
    // Flag indicating whether the manager is running
    std::atomic<bool> m_running;
    
    // Callback function called when an order is filled
    std::function<void(const std::string&, double, double, double)> m_fillCallback;
};

#endif // ORDERBOOKMANAGER_H