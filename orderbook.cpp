#include "orderbook.h"
#include <algorithm>
#include <cassert>
#include <limits>

OrderBook::OrderBook(const std::string& symbol)
    : m_symbol(symbol)
{
}

OrderBook::~OrderBook()
{
    // Clean up all limits
    for (auto& [price, limit] : m_bids) {
        delete limit;
    }
    
    for (auto& [price, limit] : m_asks) {
        delete limit;
    }
    
    // Orders are now managed by shared_ptr
}

bool OrderBook::addOrder(OrderPtr order, 
                         const std::function<void(const std::string&, double, double, double)>& fillCallback)
{
    // Validate order
    if (!order || !order->isActive()) {
        return false;
    }
    
    // Make sure order ID is unique
    if (m_orders.find(order->getID()) != m_orders.end()) {
        return false;
    }
    
    // For market orders, immediately try to match
    if (order->getType() == OrderType::MARKET) {
        // We don't add market orders to the book if they don't execute fully
        // In a real exchange, this might return "rejected" status
        return matchOrder(order, fillCallback);
    }
    
    // For limit orders, add to the book
    addOrderToBook(order);
    return true;
}

bool OrderBook::cancelOrder(Order::OrderID orderId)
{
    auto it = m_orders.find(orderId);
    if (it == m_orders.end()) {
        return false;
    }
    
    OrderPtr order = it->second;
    
    // Deactivate the order
    order->deactivate();
    
    // Remove from the book
    return removeOrderFromBook(order);
}

bool OrderBook::matchOrder(OrderPtr order, 
                        const std::function<void(const std::string&, double, double, double)>& fillCallback)
{
    // Basic validations
    if (!order || !order->isActive()) {
        return false;
    }
    
    // Keep track of how much of the order has been filled
    double remainingQty = order->getQuantity() - order->getFilledQty();
    bool orderFullyFilled = false;
    
    if (order->getSide() == Side::BUY) {
        // BUY order matches against ASKs
        while (remainingQty > 0 && !m_asks.empty()) {
            // Get the best price on the ask side
            auto it = m_asks.begin();
            auto limit = it->second;
            
            // For a limit order, check if the price is acceptable
            if (order->getType() == OrderType::LIMIT) {
                // For a BUY order, make sure the ask price is <= our limit price
                if (limit->getPrice() > order->getPrice()) {
                    break;  // No more matching prices
                }
            }
            
            // Get the first order at this limit
            auto matchingOrder = limit->frontOrder();
            if (!matchingOrder) {
                // This should not happen if the book is properly maintained
                m_asks.erase(it);
                continue;
            }
            
            // Calculate the fill quantity (minimum of remaining quantities)
            double matchingOrderRemaining = matchingOrder->getQuantity() - matchingOrder->getFilledQty();
            double fillQty = std::min(remainingQty, matchingOrderRemaining);
            
            // Execute the fill at the resting order's price
            double fillPrice = limit->getPrice();
            
            // Update both orders with fill information
            order->fillQuantity(fillQty);
            matchingOrder->fillQuantity(fillQty);
            
            // Invoke the callback with fill details if provided
            if (fillCallback) {
                fillCallback(m_symbol, fillPrice, fillQty, order->getSide() == Side::BUY ? 1.0 : -1.0);
            }
            
            // Update remaining quantity
            remainingQty -= fillQty;
            
            // If the matching order is now fully filled, remove it
            if (matchingOrder->getFilledQty() >= matchingOrder->getQuantity()) {
                matchingOrder->deactivate();
                removeOrderFromBook(matchingOrder);
            }
            
            // If the limit is now empty, remove it
            if (limit->empty()) {
                m_asks.erase(it);
            }
            
            // Check if our order is fully filled
            if (remainingQty <= 0) {
                orderFullyFilled = true;
                break;
            }
        }
    } else {
        // SELL order matches against BIDs
        while (remainingQty > 0 && !m_bids.empty()) {
            // Get the best price on the bid side
            auto it = m_bids.begin();
            auto limit = it->second;
            
            // For a limit order, check if the price is acceptable
            if (order->getType() == OrderType::LIMIT) {
                // For a SELL order, make sure the bid price is >= our limit price
                if (limit->getPrice() < order->getPrice()) {
                    break;  // No more matching prices
                }
            }
            
            // Get the first order at this limit
            auto matchingOrder = limit->frontOrder();
            if (!matchingOrder) {
                // This should not happen if the book is properly maintained
                m_bids.erase(it);
                continue;
            }
            
            // Calculate the fill quantity (minimum of remaining quantities)
            double matchingOrderRemaining = matchingOrder->getQuantity() - matchingOrder->getFilledQty();
            double fillQty = std::min(remainingQty, matchingOrderRemaining);
            
            // Execute the fill at the resting order's price
            double fillPrice = limit->getPrice();
            
            // Update both orders with fill information
            order->fillQuantity(fillQty);
            matchingOrder->fillQuantity(fillQty);
            
            // Invoke the callback with fill details if provided
            if (fillCallback) {
                fillCallback(m_symbol, fillPrice, fillQty, order->getSide() == Side::BUY ? 1.0 : -1.0);
            }
            
            // Update remaining quantity
            remainingQty -= fillQty;
            
            // If the matching order is now fully filled, remove it
            if (matchingOrder->getFilledQty() >= matchingOrder->getQuantity()) {
                matchingOrder->deactivate();
                removeOrderFromBook(matchingOrder);
            }
            
            // If the limit is now empty, remove it
            if (limit->empty()) {
                m_bids.erase(it);
            }
            
            // Check if our order is fully filled
            if (remainingQty <= 0) {
                orderFullyFilled = true;
                break;
            }
        }
    }
    
    // If order is fully filled, we're done
    if (orderFullyFilled || order->getFilledQty() >= order->getQuantity()) {
        order->deactivate();
        // Don't add to the book, just return success
        return true;
    }
    
    // If this is a limit order and there's remaining quantity, add it to the book
    if (order->getType() == OrderType::LIMIT) {
        addOrderToBook(order);
    } else if (order->getType() == OrderType::MARKET) {
        // Market orders that don't fully fill are unusual
        // In a real exchange, we'd reject them or have special handling
        // Here we'll just mark them inactive and not add to the book
        order->deactivate();
    }
    
    return true;
}

double OrderBook::getBestBid() const
{
    if (m_bids.empty()) {
        return 0.0;
    }
    return m_bids.begin()->first;
}

double OrderBook::getBestAsk() const
{
    if (m_asks.empty()) {
        return std::numeric_limits<double>::max();
    }
    return m_asks.begin()->first;
}

void OrderBook::addOrderToBook(OrderPtr order)
{
    double price = order->getPrice();
    LimitPtr limit;
    
    // Handle buy side
    if (order->getSide() == Side::BUY) {
        auto it = m_bids.find(price);
        if (it == m_bids.end()) {
            limit = new Limit(price);
            m_bids[price] = limit;
        } else {
            limit = it->second;
        }
    } 
    // Handle sell side
    else {
        auto it = m_asks.find(price);
        if (it == m_asks.end()) {
            limit = new Limit(price);
            m_asks[price] = limit;
        } else {
            limit = it->second;
        }
    }
    
    // Add the order to the limit
    limit->addOrder(order);
    
    // Add to our ID -> order map for quick lookup
    m_orders[order->getID()] = order;
}

bool OrderBook::removeOrderFromBook(OrderPtr order)
{
    // First, find the order in our ID map
    auto it = m_orders.find(order->getID());
    if (it == m_orders.end()) {
        return false;
    }
    
    // Remove from ID map
    m_orders.erase(it);
    
    // Get the price limit based on side
    double price = order->getPrice();
    
    if (order->getSide() == Side::BUY) {
        auto limitIt = m_bids.find(price);
        if (limitIt == m_bids.end()) {
            // This should not happen if book is properly maintained
            return false;
        }
        
        LimitPtr limit = limitIt->second;
        
        // Remove the order from the limit
        limit->removeOrder(order);
        
        // If limit is now empty, remove it
        if (limit->empty()) {
            delete limit;
            m_bids.erase(limitIt);
        }
    } else {
        auto limitIt = m_asks.find(price);
        if (limitIt == m_asks.end()) {
            // This should not happen if book is properly maintained
            return false;
        }
        
        LimitPtr limit = limitIt->second;
        
        // Remove the order from the limit
        limit->removeOrder(order);
        
        // If limit is now empty, remove it
        if (limit->empty()) {
            delete limit;
            m_asks.erase(limitIt);
        }
    }
    
    return true;
}