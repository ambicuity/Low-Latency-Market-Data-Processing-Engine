#include "core/order_book.h"
#include <algorithm>
#include <limits>

namespace market_data {

bool OrderBook::addOrder(std::shared_ptr<Order> order) {
    if (!order || orders_.find(order->id) != orders_.end()) {
        return false; // Order already exists or invalid
    }
    
    orders_[order->id] = order;
    addOrderToLevel(order);
    total_orders_.fetch_add(1);
    
    return true;
}

bool OrderBook::cancelOrder(OrderId order_id) {
    auto it = orders_.find(order_id);
    if (it == orders_.end()) {
        return false; // Order not found
    }
    
    removeOrderFromLevel(it->second);
    orders_.erase(it);
    
    return true;
}

bool OrderBook::modifyOrder(OrderId order_id, Price new_price, Quantity new_quantity) {
    auto it = orders_.find(order_id);
    if (it == orders_.end()) {
        return false;
    }
    
    auto order = it->second;
    
    // Remove from current level
    removeOrderFromLevel(order);
    
    // Update order
    order->price = new_price;
    order->quantity = new_quantity;
    
    // Add to new level
    addOrderToLevel(order);
    
    return true;
}

Price OrderBook::getBestBid() const {
    if (bid_levels_.empty()) {
        return 0;
    }
    return bid_levels_.begin()->first;
}

Price OrderBook::getBestAsk() const {
    if (ask_levels_.empty()) {
        return std::numeric_limits<Price>::max();
    }
    return ask_levels_.begin()->first;
}

Price OrderBook::getSpread() const {
    Price best_bid = getBestBid();
    Price best_ask = getBestAsk();
    
    if (best_bid == 0 || best_ask == std::numeric_limits<Price>::max()) {
        return std::numeric_limits<Price>::max();
    }
    
    return best_ask - best_bid;
}

Quantity OrderBook::getQuantityAtLevel(Side side, Price price) const {
    if (side == Side::BUY) {
        auto it = bid_levels_.find(price);
        return (it != bid_levels_.end()) ? it->second->total_quantity.load() : 0;
    } else {
        auto it = ask_levels_.find(price);
        return (it != ask_levels_.end()) ? it->second->total_quantity.load() : 0;
    }
}

std::vector<std::pair<Price, Quantity>> OrderBook::getMarketDepth(Side side, size_t levels) const {
    std::vector<std::pair<Price, Quantity>> depth;
    depth.reserve(levels);
    
    if (side == Side::BUY) {
        size_t count = 0;
        for (const auto& level : bid_levels_) {
            if (count >= levels) break;
            depth.emplace_back(level.first, level.second->total_quantity.load());
            ++count;
        }
    } else {
        size_t count = 0;
        for (const auto& level : ask_levels_) {
            if (count >= levels) break;
            depth.emplace_back(level.first, level.second->total_quantity.load());
            ++count;
        }
    }
    
    return depth;
}

void OrderBook::clear() {
    orders_.clear();
    bid_levels_.clear();
    ask_levels_.clear();
    total_orders_.store(0);
    total_trades_.store(0);
}

Quote OrderBook::getCurrentQuote() const {
    Price best_bid = getBestBid();
    Price best_ask = getBestAsk();
    Quantity bid_qty = (best_bid > 0) ? getQuantityAtLevel(Side::BUY, best_bid) : 0;
    Quantity ask_qty = (best_ask < std::numeric_limits<Price>::max()) ? getQuantityAtLevel(Side::SELL, best_ask) : 0;
    
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
    return Quote(symbol_, best_bid, bid_qty, best_ask, ask_qty, 
                 std::chrono::duration_cast<std::chrono::nanoseconds>(now));
}

void OrderBook::addOrderToLevel(const std::shared_ptr<Order>& order) {
    if (order->side == Side::BUY) {
        auto& level = bid_levels_[order->price];
        if (!level) {
            level = std::make_unique<BookLevel>(order->price);
        }
        level->orders.push_back(order);
        level->total_quantity.fetch_add(order->quantity);
    } else {
        auto& level = ask_levels_[order->price];
        if (!level) {
            level = std::make_unique<BookLevel>(order->price);
        }
        level->orders.push_back(order);
        level->total_quantity.fetch_add(order->quantity);
    }
}

void OrderBook::removeOrderFromLevel(const std::shared_ptr<Order>& order) {
    if (order->side == Side::BUY) {
        auto level_it = bid_levels_.find(order->price);
        if (level_it != bid_levels_.end()) {
            auto& level = level_it->second;
            auto& orders = level->orders;
            
            // Remove order from level
            orders.erase(std::remove(orders.begin(), orders.end(), order), orders.end());
            level->total_quantity.fetch_sub(order->quantity);
            
            // Remove empty level
            if (orders.empty()) {
                bid_levels_.erase(level_it);
            }
        }
    } else {
        auto level_it = ask_levels_.find(order->price);
        if (level_it != ask_levels_.end()) {
            auto& level = level_it->second;
            auto& orders = level->orders;
            
            // Remove order from level
            orders.erase(std::remove(orders.begin(), orders.end(), order), orders.end());
            level->total_quantity.fetch_sub(order->quantity);
            
            // Remove empty level
            if (orders.empty()) {
                ask_levels_.erase(level_it);
            }
        }
    }
}

} // namespace market_data