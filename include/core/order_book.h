#pragma once

#include "market_data_types.h"
#include <map>
#include <memory>
#include <vector>
#include <atomic>

namespace market_data {

// Order book level representing price and aggregate quantity
struct BookLevel {
    Price price;
    std::atomic<Quantity> total_quantity{0};
    std::vector<std::shared_ptr<Order>> orders;
    
    BookLevel(Price p) : price(p) {}
};

// High-performance order book implementation
class OrderBook {
private:
    Symbol symbol_;
    
    // Use maps for efficient price-level operations
    // In production, could use custom containers for better cache performance
    std::map<Price, std::unique_ptr<BookLevel>, std::greater<Price>> bid_levels_; // Descending order
    std::map<Price, std::unique_ptr<BookLevel>, std::less<Price>> ask_levels_;    // Ascending order
    
    // Order lookup for O(1) access
    std::map<OrderId, std::shared_ptr<Order>> orders_;
    
    // Statistics
    std::atomic<uint64_t> total_orders_{0};
    std::atomic<uint64_t> total_trades_{0};
    
public:
    explicit OrderBook(const Symbol& symbol) : symbol_(symbol) {}
    
    // Add new order to the book
    bool addOrder(std::shared_ptr<Order> order);
    
    // Cancel existing order
    bool cancelOrder(OrderId order_id);
    
    // Modify existing order
    bool modifyOrder(OrderId order_id, Price new_price, Quantity new_quantity);
    
    // Get best bid price
    Price getBestBid() const;
    
    // Get best ask price  
    Price getBestAsk() const;
    
    // Get spread
    Price getSpread() const;
    
    // Get total quantity at price level
    Quantity getQuantityAtLevel(Side side, Price price) const;
    
    // Get market depth (top N levels)
    std::vector<std::pair<Price, Quantity>> getMarketDepth(Side side, size_t levels = 5) const;
    
    // Clear all orders
    void clear();
    
    // Statistics
    uint64_t getTotalOrders() const { return total_orders_.load(); }
    uint64_t getTotalTrades() const { return total_trades_.load(); }
    
    // Get current quote
    Quote getCurrentQuote() const;
    
private:
    // Helper methods
    void removeOrderFromLevel(const std::shared_ptr<Order>& order);
    void addOrderToLevel(const std::shared_ptr<Order>& order);
    std::vector<Trade> matchOrders(std::shared_ptr<Order> incoming_order);
};

} // namespace market_data