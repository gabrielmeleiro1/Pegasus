#ifndef LIMIT_H
#define LIMIT_H

#include <list>
#include <algorithm>
#include <memory>
#include <atomic>
#include <mutex>
#include "order.h"

/**
 * @brief Represents all orders at a given price level (a "limit").
 * 
 * Stores a FIFO list of orders at `m_price`.
 * Add orders using addOrder() (end of list),
 * and remove orders (by pointer) using removeOrder().
 */
class Limit {
public:
    using OrderPtr = std::shared_ptr<Order>;
    
    explicit Limit(double price);

    // Add an order at the back (FIFO)
    void addOrder(OrderPtr order);

    // Remove the specified order (if it exists) from this limit
    void removeOrder(OrderPtr order);

    // Accessors
    double getPrice() const { return m_price; }
    double getTotalVolume() const { return m_totalVolumeAtomic.load(std::memory_order_acquire); }

    bool empty() const { 
        std::lock_guard<std::mutex> lock(m_ordersMutex);
        return m_orders.empty(); 
    }

    // Access to the FIFO list of orders
    // (e.g., the front is the oldest)
    OrderPtr frontOrder() const;
    OrderPtr backOrder()  const;

private:
    double m_price;       ///< Price of this limit
    double m_totalVolume; ///< Cache of total volume (use atomic version for thread safety)
    std::atomic<double> m_totalVolumeAtomic; ///< Atomic version for thread safety

    // Orders in FIFO order
    // In production, you might store custom linked-list nodes or iterators for O(1) removal.
    mutable std::mutex m_ordersMutex = {}; ///< Mutex for protecting the orders list
    std::list<OrderPtr> m_orders;

    /**
     * @brief Adjusts m_totalVolume to reflect partial fills, cancels, etc.
     * 
     * Uses atomic operations for thread safety.
     */
    void updateTotalVolume(double change) { 
        // Atomic add operation
        double old_value = m_totalVolumeAtomic.load(std::memory_order_relaxed);
        double new_value;
        do {
            new_value = old_value + change;
        } while (!m_totalVolumeAtomic.compare_exchange_weak(old_value, new_value, 
                                                          std::memory_order_release,
                                                          std::memory_order_relaxed));
        // Update the cached value
        m_totalVolume = new_value;
    }
};

#endif // LIMIT_H
