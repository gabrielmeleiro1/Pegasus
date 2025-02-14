#ifndef ORDER_H
#define ORDER_H

#include <string>
#include <cstdint>

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

    double         getFilledQty() const { return m_filledQty; }
    bool           isActive()     const { return m_isActive; }

    // Setters / updaters
    void setQuantity(double newQuantity)   { m_quantity = newQuantity; }
    void fillQuantity(double fillAmount)   { m_filledQty += fillAmount; }
    void deactivate()                      { m_isActive = false; }

private:
    OrderID       m_id;
    Side          m_side;
    OrderType     m_type;
    std::string   m_symbol;

    double        m_price;      ///< Limit price
    double        m_quantity;   ///< Initial quantity (remaining quantity can be derived as m_quantity - m_filledQty)
    double        m_filledQty;  ///< How much has been filled so far

    double        m_stopPrice;  ///< For STOP_LIMIT; 0 if not used.

    bool          m_isActive;   ///< Simple flag to indicate if order is still live.
};

#endif // ORDER_H
