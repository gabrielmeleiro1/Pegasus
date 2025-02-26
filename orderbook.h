#ifndef ORDERBOOK_H
#define ORDERBOOK_H

#include <map>
#include <unordered_map>
#include <vector>
#include <functional>
#include "limit.h"
#include "order.h"

/**
 * @brief OrderBook maintains a set of buy and sell orders, organized by price level.
 * 
 * Provides functionality to add/cancel orders and match incoming orders against
 * existing resting orders.
 */
class OrderBook {
public:
    using OrderPtr = Order*;
    using LimitPtr = Limit*;
    using OrderMap = std::unordered_map<Order::OrderID, OrderPtr>;
    
    explicit OrderBook(const std::string& symbol);
    ~OrderBook();
    
    // Delete copy/move constructors and assignment operators
    OrderBook(const OrderBook&) = delete;
    OrderBook& operator=(const OrderBook&) = delete;
    OrderBook(OrderBook&&) = delete;
    OrderBook& operator=(OrderBook&&) = delete;

    /**
     * @brief Add a new order to the book
     * 
     * @param order The order to add
     * @return true if added successfully, false otherwise
     */
    bool addOrder(OrderPtr order);
    
    /**
     * @brief Cancel an existing order
     * 
     * @param orderId ID of the order to cancel
     * @return true if cancelled successfully, false if order not found
     */
    bool cancelOrder(Order::OrderID orderId);
    
    /**
     * @brief Match a new order against existing orders in the book
     * 
     * For market orders, continues crossing until filled or no more matches.
     * For limit orders, crosses against eligible orders and adds the remainder to the book.
     * 
     * @param order The order to match
     * @param fillCallback Function called with details of each fill (may be called multiple times)
     * @return true if order fully processed, false otherwise
     */
    bool matchOrder(OrderPtr order, 
                    std::function<void(OrderPtr, OrderPtr, double, double)> fillCallback);
    
    /**
     * @brief Get current best bid price
     * 
     * @return Best bid price or 0.0 if no bids
     */
    double getBestBid() const;
    
    /**
     * @brief Get current best ask price
     * 
     * @return Best ask price or 0.0 if no asks
     */
    double getBestAsk() const;
    
    /**
     * @brief Get symbol associated with this order book
     */
    const std::string& getSymbol() const { return m_symbol; }

private:
    std::string m_symbol;
    
    // Maps price -> limit level
    std::map<double, LimitPtr, std::greater<double>> m_bids; // Sorted high to low
    std::map<double, LimitPtr> m_asks;                       // Sorted low to high
    
    // Maps order ID -> order for quick lookup
    OrderMap m_orders;
    
    // Helper methods
    void addOrderToBook(OrderPtr order);
    bool removeOrderFromBook(OrderPtr order);
};

#endif // ORDERBOOK_H