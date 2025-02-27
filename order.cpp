#include "order.h"

Order::Order(OrderID id,
             Side side,
             OrderType type,
             const std::string& symbol,
             double price,
             double quantity,
             double stopPrice)
    : m_id(id),
      m_side(side),
      m_type(type),
      m_symbol(symbol),
      m_price(price),
      m_quantity(quantity),
      m_filledQty(0.0),
      m_filledQtyAtomic(0.0),
      m_stopPrice(stopPrice),
      m_isActive(true),
      m_isActiveAtomic(true)
{
    // Add additional validation
    // like price > 0, quantity > 0, etc.)
}
