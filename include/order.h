#ifndef ORDER_H
#define ORDER_H

#include <string>
#include <cstdint>
#include <atomic>

enum class Side {
    BUY,
    SELL
};

enum class OrderType {
    MARKET,
    LIMIT,
    STOP_LIMIT
};

/**
 * @brief
 */
class Order {
public:
    using OrderID = std::uint64_t;

    /**
     * @brief Construct a new Order object.
     * 
     * @param id         Unique identifier for this order.
     * @param side       BUY or SELL.
     * @param type       MARKET, LIMIT, or STOP_LIMIT.
     * @param symbol     Symbol (e.g. "AAPL", "BTCUSD").
     * @param price      Limit price (if type == LIMIT or STOP_LIMIT). 
     * @param quantity   Quantity of the order.
     * @param stopPrice  Trigger price (only meaningful if type == STOP_LIMIT).
     */
    Order(OrderID id,
          Side side,
          OrderType type,
          const std::string& symbol,
          double price,
          double quantity,
          double stopPrice = 0.0);

    // Getters
    OrderID        getID()       const { return m_id; }
    Side           getSide()     const { return m_side; }
    OrderType      getType()     const { return m_type; }
    const std::string& getSymbol()   const { return m_symbol; }
    double         getPrice()    const { return m_price; }
    double         getStopPrice()const { return m_stopPrice; }
    double         getQuantity() const { return m_quantity; }

    double         getFilledQty() const { return m_filledQtyAtomic.load(std::memory_order_acquire); }
    bool           isActive()     const { return m_isActiveAtomic.load(std::memory_order_acquire); }

    // Setters / updaters with atomic operations for thread safety
    void setQuantity(double newQuantity)   { m_quantity = newQuantity; }
    
    // Thread-safe fill quantity operation
    void fillQuantity(double fillAmount)   { 
        // Use atomic operation for updating filled quantity
        double current = m_filledQty;
        double desired = current + fillAmount;
        // Keep trying until we successfully update the value
        while (!m_filledQtyAtomic.compare_exchange_weak(current, desired)) {
            // If compare_exchange_weak fails, current is updated with the latest value
            // Recalculate desired based on the updated current
            desired = current + fillAmount;
        }
        m_filledQty = desired; // Update the regular member for consistency
    }
    
    // Thread-safe deactivation
    void deactivate() { 
        m_isActiveAtomic.store(false, std::memory_order_release);
        m_isActive = false; 
    }

private:
    OrderID       m_id;
    Side          m_side;
    OrderType     m_type;
    std::string   m_symbol;

    double        m_price;      ///< Limit price
    double        m_quantity;   ///< Initial quantity (remaining quantity can be derived as m_quantity - m_filledQty)
    double        m_filledQty;  ///< How much has been filled so far (cached value, see atomic version)
    std::atomic<double> m_filledQtyAtomic;  ///< Atomic version for thread safety

    double        m_stopPrice;  ///< For STOP_LIMIT; 0 if not used.

    bool          m_isActive;   ///< Simple flag to indicate if order is still live (cached value, see atomic version)
    std::atomic<bool> m_isActiveAtomic;   ///< Atomic version for thread safety
};

#endif // ORDER_H
