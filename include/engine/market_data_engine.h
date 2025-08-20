#pragma once

#include "core/market_data_types.h"
#include "core/order_book.h"
#include "utils/lock_free_queue.h"
#include <unordered_map>
#include <thread>
#include <atomic>
#include <functional>
#include <memory>

namespace market_data {

// Market data message wrapper
struct MarketDataMessage {
    MessageType type;
    std::shared_ptr<Order> order;
    std::shared_ptr<Trade> trade;
    std::shared_ptr<Quote> quote;
    Timestamp timestamp;
    
    MarketDataMessage() : type(MessageType::NEW_ORDER), timestamp(Timestamp{}) {}
    MarketDataMessage(MessageType t, Timestamp ts) : type(t), timestamp(ts) {}
};

// Event handler interface
class MarketDataEventHandler {
public:
    virtual ~MarketDataEventHandler() = default;
    virtual void onOrderAdded(const Order& order) {}
    virtual void onOrderCancelled(OrderId order_id) {}
    virtual void onOrderModified(const Order& order) {}
    virtual void onTrade(const Trade& trade) {}
    virtual void onQuoteUpdate(const Quote& quote) {}
};

// High-performance market data processing engine
class MarketDataEngine {
private:
    // Order books for different symbols
    std::unordered_map<Symbol, std::unique_ptr<OrderBook>> order_books_;
    
    // Message queue for processing
    static constexpr size_t QUEUE_SIZE = 65536;  // Power of 2
    RingBuffer<MarketDataMessage, QUEUE_SIZE> message_queue_;
    
    // Processing thread
    std::thread processing_thread_;
    std::atomic<bool> running_{false};
    
    // Event handlers
    std::vector<std::shared_ptr<MarketDataEventHandler>> event_handlers_;
    
    // Performance statistics
    std::atomic<uint64_t> messages_processed_{0};
    std::atomic<uint64_t> processing_errors_{0};
    
public:
    MarketDataEngine() = default;
    ~MarketDataEngine() { stop(); }
    
    // Non-copyable
    MarketDataEngine(const MarketDataEngine&) = delete;
    MarketDataEngine& operator=(const MarketDataEngine&) = delete;
    
    // Start the processing engine
    void start();
    
    // Stop the processing engine
    void stop();
    
    // Add event handler
    void addEventHandler(std::shared_ptr<MarketDataEventHandler> handler);
    
    // Submit new order
    bool submitOrder(const Order& order);
    
    // Cancel order
    bool cancelOrder(const Symbol& symbol, OrderId order_id);
    
    // Modify order
    bool modifyOrder(const Symbol& symbol, OrderId order_id, Price new_price, Quantity new_quantity);
    
    // Get order book for symbol
    OrderBook* getOrderBook(const Symbol& symbol);
    
    // Get current quote for symbol
    Quote getCurrentQuote(const Symbol& symbol);
    
    // Performance statistics
    uint64_t getMessagesProcessed() const { return messages_processed_.load(); }
    uint64_t getProcessingErrors() const { return processing_errors_.load(); }
    
private:
    // Main processing loop
    void processMessages();
    
    // Process individual message
    void processMessage(const MarketDataMessage& message);
    
    // Ensure order book exists for symbol
    OrderBook* ensureOrderBook(const Symbol& symbol);
    
    // Notify event handlers
    void notifyOrderAdded(const Order& order);
    void notifyOrderCancelled(OrderId order_id);
    void notifyOrderModified(const Order& order);
    void notifyTrade(const Trade& trade);
    void notifyQuoteUpdate(const Quote& quote);
};

} // namespace market_data