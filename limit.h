#ifndef LIMIT_H
#define LIMIT_H

#include <list>
#include <algorithm>
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
    explicit Limit(double price);

    // Add an order at the back (FIFO)
    void addOrder(Order* order);

    // Remove the specified order (if it exists) from this limit
    void removeOrder(Order* order);

    // Accessors
    double getPrice() const { return m_price; }
    double getTotalVolume() const { return m_totalVolume; }

    bool empty() const { return m_orders.empty(); }

    // Access to the FIFO list of orders
    // (e.g., the front is the oldest)
    Order* frontOrder() const;
    Order* backOrder()  const;

private:
    double         m_price;       ///< Price of this limit
    double         m_totalVolume; ///< Sum of (quantity - filledQty) for all active orders

    // Orders in FIFO order
    // In production, you might store custom linked-list nodes or iterators for O(1) removal.
    std::list<Order*> m_orders;

    /**
     * @brief Adjusts m_totalVolume to reflect partial fills, cancels, etc.
     * 
     * In practice, you may update this on the fly in the matching engine, 
     * or every time an order changes quantity/fills.
     */
    void updateTotalVolume(double change) { 
        m_totalVolume += change;
    }
};

#endif // LIMIT_H
