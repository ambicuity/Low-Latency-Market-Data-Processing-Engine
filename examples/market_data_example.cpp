#include "engine/market_data_engine.h"
#include "utils/performance_stats.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <iomanip>

using namespace market_data;

// Example event handler that logs market events
class LoggingEventHandler : public MarketDataEventHandler {
private:
    bool verbose_;
    
public:
    explicit LoggingEventHandler(bool verbose = false) : verbose_(verbose) {}
    
    void onOrderAdded(const Order& order) override {
        if (verbose_) {
            std::cout << "Order Added: ID=" << order.id 
                     << ", Symbol=" << order.symbol
                     << ", Side=" << (order.side == Side::BUY ? "BUY" : "SELL")
                     << ", Price=" << std::fixed << std::setprecision(2) << (order.price / 100.0)
                     << ", Quantity=" << order.quantity << std::endl;
        }
    }
    
    void onOrderCancelled(OrderId order_id) override {
        if (verbose_) {
            std::cout << "Order Cancelled: ID=" << order_id << std::endl;
        }
    }
    
    void onOrderModified(const Order& order) override {
        if (verbose_) {
            std::cout << "Order Modified: ID=" << order.id
                     << ", New Price=" << std::fixed << std::setprecision(2) << (order.price / 100.0)
                     << ", New Quantity=" << order.quantity << std::endl;
        }
    }
    
    void onTrade(const Trade& trade) override {
        if (verbose_) {
            std::cout << "Trade Executed: Symbol=" << trade.symbol
                     << ", Price=" << std::fixed << std::setprecision(2) << (trade.price / 100.0)
                     << ", Quantity=" << trade.quantity << std::endl;
        }
    }
    
    void onQuoteUpdate(const Quote& quote) override {
        if (!verbose_) {
            std::cout << "Quote Update: " << quote.symbol 
                     << " | Bid: " << std::fixed << std::setprecision(2) << (quote.bid_price / 100.0)
                     << "x" << quote.bid_quantity
                     << " | Ask: " << std::fixed << std::setprecision(2) << (quote.ask_price / 100.0)
                     << "x" << quote.ask_quantity << std::endl;
        }
    }
};

void demonstrateBasicFunctionality() {
    std::cout << "\n=== Basic Functionality Demo ===" << std::endl;
    
    MarketDataEngine engine;
    auto handler = std::make_shared<LoggingEventHandler>(true);
    engine.addEventHandler(handler);
    
    engine.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
    auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(now);
    
    // Add some orders
    Order buy_order(1, "AAPL", Side::BUY, OrderType::LIMIT, 15000, 100, timestamp);  // $150.00
    Order sell_order(2, "AAPL", Side::SELL, OrderType::LIMIT, 15050, 200, timestamp); // $150.50
    Order buy_order2(3, "AAPL", Side::BUY, OrderType::LIMIT, 14990, 150, timestamp);  // $149.90
    
    engine.submitOrder(buy_order);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    engine.submitOrder(sell_order);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    engine.submitOrder(buy_order2);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    // Get current quote
    Quote current_quote = engine.getCurrentQuote("AAPL");
    std::cout << "\nCurrent Market Quote:" << std::endl;
    std::cout << "  " << current_quote.symbol 
             << " Bid: $" << std::fixed << std::setprecision(2) << (current_quote.bid_price / 100.0)
             << " x " << current_quote.bid_quantity
             << ", Ask: $" << std::fixed << std::setprecision(2) << (current_quote.ask_price / 100.0) 
             << " x " << current_quote.ask_quantity << std::endl;
    
    // Get market depth
    OrderBook* book = engine.getOrderBook("AAPL");
    if (book) {
        auto bid_depth = book->getMarketDepth(Side::BUY, 5);
        auto ask_depth = book->getMarketDepth(Side::SELL, 5);
        
        std::cout << "\nMarket Depth:" << std::endl;
        std::cout << "Bids:" << std::endl;
        for (const auto& level : bid_depth) {
            std::cout << "  $" << std::fixed << std::setprecision(2) << (level.first / 100.0)
                     << " x " << level.second << std::endl;
        }
        
        std::cout << "Asks:" << std::endl;
        for (const auto& level : ask_depth) {
            std::cout << "  $" << std::fixed << std::setprecision(2) << (level.first / 100.0)
                     << " x " << level.second << std::endl;
        }
        
        std::cout << "Spread: $" << std::fixed << std::setprecision(2) 
                 << (book->getSpread() / 100.0) << std::endl;
    }
    
    // Cancel an order
    std::cout << "\nCancelling order 1..." << std::endl;
    engine.cancelOrder("AAPL", 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    // Modify an order
    std::cout << "\nModifying order 3..." << std::endl;
    engine.modifyOrder("AAPL", 3, 15010, 300);  // $150.10, 300 shares
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    engine.stop();
}

void demonstratePerformance() {
    std::cout << "\n=== Performance Demo ===" << std::endl;
    
    MarketDataEngine engine;
    auto handler = std::make_shared<LoggingEventHandler>(false);  // Non-verbose
    engine.addEventHandler(handler);
    
    engine.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    PerformanceStats order_submission_stats;
    const int NUM_ORDERS = 10000;
    
    std::cout << "Submitting " << NUM_ORDERS << " orders..." << std::endl;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < NUM_ORDERS; ++i) {
        LatencyTimer timer;
        timer.start();
        
        auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
        auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(now);
        
        Side side = (i % 2) ? Side::BUY : Side::SELL;
        Price price = (side == Side::BUY) ? 15000 - (i % 50) : 15050 + (i % 50);
        
        Order order(i + 1, "AAPL", side, OrderType::LIMIT, price, 100, timestamp);
        engine.submitOrder(order);
        
        order_submission_stats.recordLatency(timer.elapsed());
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto total_duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    // Wait for processing to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    std::cout << "Order submission completed in " << total_duration.count() 
             << " microseconds" << std::endl;
    std::cout << "Throughput: " << (NUM_ORDERS * 1000000.0) / total_duration.count() 
             << " orders/second" << std::endl;
    
    order_submission_stats.printSummary("Order Submission");
    
    // Engine statistics
    std::cout << "Engine processed " << engine.getMessagesProcessed() 
             << " messages with " << engine.getProcessingErrors() << " errors" << std::endl;
    
    // Final quote
    Quote final_quote = engine.getCurrentQuote("AAPL");
    std::cout << "Final Quote: " << final_quote.symbol 
             << " Bid: $" << std::fixed << std::setprecision(2) << (final_quote.bid_price / 100.0)
             << " x " << final_quote.bid_quantity
             << ", Ask: $" << std::fixed << std::setprecision(2) << (final_quote.ask_price / 100.0) 
             << " x " << final_quote.ask_quantity << std::endl;
    
    engine.stop();
}

int main() {
    std::cout << "Low-Latency Market Data Processing Engine Demo" << std::endl;
    std::cout << "=============================================" << std::endl;
    
    try {
        demonstrateBasicFunctionality();
        demonstratePerformance();
        
        std::cout << "\nDemo completed successfully!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}