#include "engine/market_data_engine.h"
#include <algorithm>
#include <chrono>

namespace market_data {

void MarketDataEngine::start() {
    if (running_.load()) {
        return; // Already running
    }
    
    running_.store(true);
    processing_thread_ = std::thread(&MarketDataEngine::processMessages, this);
}

void MarketDataEngine::stop() {
    if (!running_.load()) {
        return; // Not running
    }
    
    running_.store(false);
    if (processing_thread_.joinable()) {
        processing_thread_.join();
    }
}

void MarketDataEngine::addEventHandler(std::shared_ptr<MarketDataEventHandler> handler) {
    if (handler) {
        event_handlers_.push_back(handler);
    }
}

bool MarketDataEngine::submitOrder(const Order& order) {
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
    MarketDataMessage message(MessageType::NEW_ORDER, 
                            std::chrono::duration_cast<std::chrono::nanoseconds>(now));
    message.order = std::make_shared<Order>(order);
    
    return message_queue_.try_push(message);
}

bool MarketDataEngine::cancelOrder(const Symbol& symbol, OrderId order_id) {
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
    MarketDataMessage message(MessageType::CANCEL_ORDER, 
                            std::chrono::duration_cast<std::chrono::nanoseconds>(now));
    message.order = std::make_shared<Order>();
    message.order->id = order_id;
    message.order->symbol = symbol;
    
    return message_queue_.try_push(message);
}

bool MarketDataEngine::modifyOrder(const Symbol& symbol, OrderId order_id, 
                                  Price new_price, Quantity new_quantity) {
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
    MarketDataMessage message(MessageType::MODIFY_ORDER, 
                            std::chrono::duration_cast<std::chrono::nanoseconds>(now));
    message.order = std::make_shared<Order>();
    message.order->id = order_id;
    message.order->symbol = symbol;
    message.order->price = new_price;
    message.order->quantity = new_quantity;
    
    return message_queue_.try_push(message);
}

OrderBook* MarketDataEngine::getOrderBook(const Symbol& symbol) {
    auto it = order_books_.find(symbol);
    return (it != order_books_.end()) ? it->second.get() : nullptr;
}

Quote MarketDataEngine::getCurrentQuote(const Symbol& symbol) {
    OrderBook* book = getOrderBook(symbol);
    if (book) {
        return book->getCurrentQuote();
    }
    
    // Return empty quote if book doesn't exist
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
    return Quote(symbol, 0, 0, 0, 0, 
                std::chrono::duration_cast<std::chrono::nanoseconds>(now));
}

void MarketDataEngine::processMessages() {
    MarketDataMessage message(MessageType::NEW_ORDER, Timestamp{});
    
    while (running_.load()) {
        if (message_queue_.try_pop(message)) {
            processMessage(message);
            messages_processed_.fetch_add(1);
        } else {
            // No messages, yield to avoid busy waiting
            std::this_thread::yield();
        }
    }
    
    // Process remaining messages before shutdown
    while (message_queue_.try_pop(message)) {
        processMessage(message);
        messages_processed_.fetch_add(1);
    }
}

void MarketDataEngine::processMessage(const MarketDataMessage& message) {
    try {
        switch (message.type) {
            case MessageType::NEW_ORDER: {
                if (message.order) {
                    OrderBook* book = ensureOrderBook(message.order->symbol);
                    if (book && book->addOrder(message.order)) {
                        notifyOrderAdded(*message.order);
                        
                        // Generate quote update
                        Quote quote = book->getCurrentQuote();
                        notifyQuoteUpdate(quote);
                    }
                }
                break;
            }
            
            case MessageType::CANCEL_ORDER: {
                if (message.order) {
                    OrderBook* book = getOrderBook(message.order->symbol);
                    if (book && book->cancelOrder(message.order->id)) {
                        notifyOrderCancelled(message.order->id);
                        
                        // Generate quote update
                        Quote quote = book->getCurrentQuote();
                        notifyQuoteUpdate(quote);
                    }
                }
                break;
            }
            
            case MessageType::MODIFY_ORDER: {
                if (message.order) {
                    OrderBook* book = getOrderBook(message.order->symbol);
                    if (book && book->modifyOrder(message.order->id, 
                                                 message.order->price, 
                                                 message.order->quantity)) {
                        notifyOrderModified(*message.order);
                        
                        // Generate quote update
                        Quote quote = book->getCurrentQuote();
                        notifyQuoteUpdate(quote);
                    }
                }
                break;
            }
            
            case MessageType::TRADE: {
                if (message.trade) {
                    notifyTrade(*message.trade);
                }
                break;
            }
            
            case MessageType::QUOTE_UPDATE: {
                if (message.quote) {
                    notifyQuoteUpdate(*message.quote);
                }
                break;
            }
        }
    } catch (const std::exception&) {
        processing_errors_.fetch_add(1);
    }
}

OrderBook* MarketDataEngine::ensureOrderBook(const Symbol& symbol) {
    auto it = order_books_.find(symbol);
    if (it == order_books_.end()) {
        order_books_[symbol] = std::make_unique<OrderBook>(symbol);
        return order_books_[symbol].get();
    }
    return it->second.get();
}

void MarketDataEngine::notifyOrderAdded(const Order& order) {
    for (auto& handler : event_handlers_) {
        handler->onOrderAdded(order);
    }
}

void MarketDataEngine::notifyOrderCancelled(OrderId order_id) {
    for (auto& handler : event_handlers_) {
        handler->onOrderCancelled(order_id);
    }
}

void MarketDataEngine::notifyOrderModified(const Order& order) {
    for (auto& handler : event_handlers_) {
        handler->onOrderModified(order);
    }
}

void MarketDataEngine::notifyTrade(const Trade& trade) {
    for (auto& handler : event_handlers_) {
        handler->onTrade(trade);
    }
}

void MarketDataEngine::notifyQuoteUpdate(const Quote& quote) {
    for (auto& handler : event_handlers_) {
        handler->onQuoteUpdate(quote);
    }
}

} // namespace market_data