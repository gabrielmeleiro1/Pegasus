#include "limit.h"
#include <iterator>   // for std::find (if needed)
#include <cassert>    // for assert

Limit::Limit(double price)
    : m_price(price),
      m_totalVolume(0.0),
      m_totalVolumeAtomic(0.0)
{
}

void Limit::addOrder(OrderPtr order)
{
    if (!order) {
        return;  // Safety check
    }
    
    // Insert at the back of the FIFO list with mutex protection
    {
        std::lock_guard<std::mutex> lock(m_ordersMutex);
        m_orders.push_back(order);
    }

    // Increase total volume by the order's available quantity
    double remainingQty = order->getQuantity() - order->getFilledQty();
    updateTotalVolume(remainingQty);
}

void Limit::removeOrder(OrderPtr order)
{
    if (!order) {
        return;  // Safety check
    }
    
    bool orderFound = false;
    {
        std::lock_guard<std::mutex> lock(m_ordersMutex);
        // We find the order in O(n) time in std::list.
        // For a learning project, this is okay; in production HFT, you'd optimize this.
        auto it = std::find(m_orders.begin(), m_orders.end(), order);
        if (it != m_orders.end()) {
            // Erase it from the list
            m_orders.erase(it);
            orderFound = true;
        }
    }
    
    if (orderFound) {
        // Decrease total volume - only consider unfilled quantity
        double remainingQty = order->getQuantity() - order->getFilledQty();
        if (remainingQty > 0) {
            updateTotalVolume(-remainingQty);
        }
    }
}

Limit::OrderPtr Limit::frontOrder() const
{
    std::lock_guard<std::mutex> lock(m_ordersMutex);
    if (m_orders.empty()) {
        return nullptr;
    }
    return m_orders.front();
}

Limit::OrderPtr Limit::backOrder() const
{
    std::lock_guard<std::mutex> lock(m_ordersMutex);
    if (m_orders.empty()) {
        return nullptr;
    }
    return m_orders.back();
}
