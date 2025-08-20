#pragma once

#include <cstdint>
#include <string>
#include <chrono>

namespace market_data {

using Price = std::int64_t;  // Fixed-point price (multiply by 10000 for 4 decimal places)
using Quantity = std::uint64_t;
using OrderId = std::uint64_t;
using Symbol = std::string;
using Timestamp = std::chrono::nanoseconds;

// Order side enumeration
enum class Side : std::uint8_t {
    BUY = 0,
    SELL = 1
};

// Order type enumeration  
enum class OrderType : std::uint8_t {
    MARKET = 0,
    LIMIT = 1,
    STOP = 2
};

// Market data message types
enum class MessageType : std::uint8_t {
    NEW_ORDER = 0,
    CANCEL_ORDER = 1,
    MODIFY_ORDER = 2,
    TRADE = 3,
    QUOTE_UPDATE = 4
};

// Basic order structure
struct Order {
    OrderId id;
    Symbol symbol;
    Side side;
    OrderType type;
    Price price;
    Quantity quantity;
    Timestamp timestamp;
    
    Order() = default;
    Order(OrderId id_, const Symbol& symbol_, Side side_, OrderType type_, 
          Price price_, Quantity quantity_, Timestamp timestamp_)
        : id(id_), symbol(symbol_), side(side_), type(type_), 
          price(price_), quantity(quantity_), timestamp(timestamp_) {}
};

// Trade execution structure
struct Trade {
    OrderId buy_order_id;
    OrderId sell_order_id;
    Symbol symbol;
    Price price;
    Quantity quantity;
    Timestamp timestamp;
    
    Trade() = default;
    Trade(OrderId buy_id, OrderId sell_id, const Symbol& symbol_, 
          Price price_, Quantity quantity_, Timestamp timestamp_)
        : buy_order_id(buy_id), sell_order_id(sell_id), symbol(symbol_),
          price(price_), quantity(quantity_), timestamp(timestamp_) {}
};

// Quote structure for market data
struct Quote {
    Symbol symbol;
    Price bid_price;
    Quantity bid_quantity;
    Price ask_price;
    Quantity ask_quantity;
    Timestamp timestamp;
    
    Quote() = default;
    Quote(const Symbol& symbol_, Price bid_p, Quantity bid_q, 
          Price ask_p, Quantity ask_q, Timestamp timestamp_)
        : symbol(symbol_), bid_price(bid_p), bid_quantity(bid_q),
          ask_price(ask_p), ask_quantity(ask_q), timestamp(timestamp_) {}
};

} // namespace market_data