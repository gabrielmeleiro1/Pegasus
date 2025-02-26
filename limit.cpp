#include "limit.h"
#include <iterator>   // for std::find (if needed)
#include <cassert>    // for assert

Limit::Limit(double price)
    : m_price(price),
      m_totalVolume(0.0)
{
}

void Limit::addOrder(Order* order)
{
    if (!order) {
        return;  // Safety check
    }
    
    // Insert at the back of the FIFO list
    m_orders.push_back(order);

    // Increase total volume by the order's available quantity
    double remainingQty = order->getQuantity() - order->getFilledQty();
    updateTotalVolume(remainingQty);
}

void Limit::removeOrder(Order* order)
{
    if (!order) {
        return;  // Safety check
    }
    
    // We find the order in O(n) time in std::list.
    // For a learning project, this is okay; in production HFT, you'd optimize this.
    auto it = std::find(m_orders.begin(), m_orders.end(), order);
    if (it != m_orders.end()) {
        // Decrease total volume - only consider unfilled quantity
        double remainingQty = order->getQuantity() - order->getFilledQty();
        if (remainingQty > 0) {
            updateTotalVolume(-remainingQty);
        }

        // Erase it from the list
        m_orders.erase(it);
    }
}

Order* Limit::frontOrder() const
{
    if (m_orders.empty()) {
        return nullptr;
    }
    return m_orders.front();
}

Order* Limit::backOrder() const
{
    if (m_orders.empty()) {
        return nullptr;
    }
    return m_orders.back();
}
