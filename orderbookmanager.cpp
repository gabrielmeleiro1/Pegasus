#include "orderbookmanager.h"
#include <iostream>

OrderBookManager::OrderBookManager(const std::function<void(const std::string&, double, double, double)>& fillCallback)
    : m_running(true), m_fillCallback(fillCallback)
{
}

OrderBookManager::~OrderBookManager()
{
    shutdown();
}

void OrderBookManager::shutdown()
{
    // First, set the running flag to false
    m_running = false;
    
    // Make a copy of the symbols to avoid potential issues during shutdown
    std::vector<std::string> symbols;
    {
        std::lock_guard<std::mutex> lock(m_mapMutex);
        for (const auto& [symbol, _] : m_symbolThreads) {
            symbols.push_back(symbol);
        }
    }
    
    // Create shutdown actions for all symbol threads
    for (const auto& symbol : symbols) {
        // Safely check if the mutex and condvar exist for this symbol
        {
            std::lock_guard<std::mutex> lock(m_mapMutex);
            if (m_queueMutexes.find(symbol) == m_queueMutexes.end() ||
                m_queueCondVars.find(symbol) == m_queueCondVars.end() ||
                m_orderQueues.find(symbol) == m_orderQueues.end()) {
                continue;
            }
            
            // Signal the thread to shut down
            {
                std::lock_guard<std::mutex> queueLock(*m_queueMutexes[symbol]);
                m_orderQueues[symbol].push(OrderAction{OrderActionType::SHUTDOWN, nullptr, nullptr, 0});
            }
            m_queueCondVars[symbol]->notify_one();
        }
    }
    
    // Join all threads
    for (const auto& symbol : symbols) {
        std::thread* threadPtr = nullptr;
        {
            std::lock_guard<std::mutex> lock(m_mapMutex);
            auto it = m_symbolThreads.find(symbol);
            if (it != m_symbolThreads.end()) {
                threadPtr = &(it->second);
            }
        }
        
        if (threadPtr && threadPtr->joinable()) {
            std::cout << "Joining thread for symbol: " << symbol << std::endl;
            threadPtr->join();
        }
    }
    
    // Clear all maps
    {
        std::lock_guard<std::mutex> lock(m_mapMutex);
        m_symbolThreads.clear();
        m_orderBooks.clear();
        m_orderQueues.clear();
        m_queueMutexes.clear();
        m_queueCondVars.clear();
    }
}

bool OrderBookManager::processOrder(std::shared_ptr<Order> order)
{
    if (!m_running) {
        return false;
    }
    
    const std::string& symbol = order->getSymbol();
    
    // Ensure we have an OrderBook and thread for this symbol
    {
        std::lock_guard<std::mutex> lock(m_mapMutex);
        if (m_symbolThreads.find(symbol) == m_symbolThreads.end()) {
            // Create necessary structures for this symbol
            m_orderBooks[symbol] = std::make_unique<OrderBook>(symbol);
            m_queueMutexes[symbol] = std::make_unique<std::mutex>();
            m_queueCondVars[symbol] = std::make_unique<std::condition_variable>();
            
            // Start a new thread for this symbol
            m_symbolThreads[symbol] = std::thread(&OrderBookManager::symbolThreadFunction, this, symbol);
        }
    }
    
    // Queue the order action
    {
        std::lock_guard<std::mutex> queueLock(*m_queueMutexes[symbol]);
        m_orderQueues[symbol].push(OrderAction{OrderActionType::ADD, order, m_fillCallback, 0});
    }
    
    // Notify the symbol thread
    m_queueCondVars[symbol]->notify_one();
    
    return true;
}

bool OrderBookManager::cancelOrder(Order::OrderID orderId, const std::string& symbol)
{
    if (!m_running) {
        return false;
    }
    
    // Ensure we have an OrderBook and thread for this symbol
    {
        std::lock_guard<std::mutex> lock(m_mapMutex);
        if (m_symbolThreads.find(symbol) == m_symbolThreads.end()) {
            // No OrderBook for this symbol, nothing to cancel
            return false;
        }
    }
    
    // Queue the cancel action
    {
        std::lock_guard<std::mutex> queueLock(*m_queueMutexes[symbol]);
        m_orderQueues[symbol].push(OrderAction{OrderActionType::CANCEL, nullptr, nullptr, orderId});
    }
    
    // Notify the symbol thread
    m_queueCondVars[symbol]->notify_one();
    
    return true;
}

std::string OrderBookManager::getOrderBookState(const std::string& symbol)
{
    std::lock_guard<std::mutex> lock(m_mapMutex);
    
    auto it = m_orderBooks.find(symbol);
    if (it == m_orderBooks.end()) {
        return "No order book found for symbol: " + symbol;
    }
    
    // Since this is just a debugging method, we'll temporarily use a simple string
    // In a real system, you'd want to get a proper snapshot without blocking the processing thread
    return "Order book for " + symbol + " exists";
}

OrderBook* OrderBookManager::getOrCreateOrderBook(const std::string& symbol)
{
    std::lock_guard<std::mutex> lock(m_mapMutex);
    
    auto it = m_orderBooks.find(symbol);
    if (it == m_orderBooks.end()) {
        m_orderBooks[symbol] = std::make_unique<OrderBook>(symbol);
        m_queueMutexes[symbol] = std::make_unique<std::mutex>();
        m_queueCondVars[symbol] = std::make_unique<std::condition_variable>();
        
        // Start a new thread for this symbol
        m_symbolThreads[symbol] = std::thread(&OrderBookManager::symbolThreadFunction, this, symbol);
    }
    
    return m_orderBooks[symbol].get();
}

void OrderBookManager::symbolThreadFunction(const std::string& symbol)
{
    std::cout << "Started thread for symbol: " << symbol << std::endl;
    
    // Get the OrderBook for this symbol
    OrderBook* orderBook = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_mapMutex);
        orderBook = m_orderBooks[symbol].get();
    }
    
    if (!orderBook) {
        std::cerr << "Error: OrderBook for symbol " << symbol << " not found!" << std::endl;
        return;
    }
    
    bool running = true;
    
    while (running && m_running.load()) {
        OrderAction action;
        bool hasAction = false;
        
        // Wait for an action
        {
            std::unique_lock<std::mutex> lock(*m_queueMutexes[symbol]);
            m_queueCondVars[symbol]->wait(lock, [this, &symbol]() {
                return !m_orderQueues[symbol].empty() || !m_running.load();
            });
            
            if (!m_running.load()) {
                break;
            }
            
            if (!m_orderQueues[symbol].empty()) {
                action = m_orderQueues[symbol].front();
                m_orderQueues[symbol].pop();
                hasAction = true;
            }
        }
        
        // Process the action if we have one
        if (hasAction) {
            switch (action.type) {
                case OrderActionType::ADD:
                    orderBook->addOrder(action.order, action.fillCallback);
                    break;
                    
                case OrderActionType::CANCEL:
                    orderBook->cancelOrder(action.orderId);
                    break;
                    
                case OrderActionType::SHUTDOWN:
                    running = false;
                    break;
            }
        }
    }
    
    std::cout << "Shutting down thread for symbol: " << symbol << std::endl;
}