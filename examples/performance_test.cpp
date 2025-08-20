#include "engine/market_data_engine.h"
#include "utils/performance_stats.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <random>
#include <iomanip>

using namespace market_data;

class PerformanceTestHandler : public MarketDataEventHandler {
private:
    std::atomic<uint64_t> orders_processed_{0};
    std::atomic<uint64_t> quotes_received_{0};
    
public:
    void onOrderAdded(const Order& order) override {
        orders_processed_.fetch_add(1);
    }
    
    void onOrderCancelled(OrderId order_id) override {
        orders_processed_.fetch_add(1);
    }
    
    void onOrderModified(const Order& order) override {
        orders_processed_.fetch_add(1);
    }
    
    void onQuoteUpdate(const Quote& quote) override {
        quotes_received_.fetch_add(1);
    }
    
    uint64_t getOrdersProcessed() const { return orders_processed_.load(); }
    uint64_t getQuotesReceived() const { return quotes_received_.load(); }
    
    void reset() {
        orders_processed_.store(0);
        quotes_received_.store(0);
    }
};

void runLatencyTest(MarketDataEngine& engine, int num_orders) {
    std::cout << "\n=== Latency Test (" << num_orders << " orders) ===" << std::endl;
    
    PerformanceStats latency_stats;
    std::mt19937 gen(42);
    std::uniform_int_distribution<Price> price_dist(14500, 15500);  // $145-$155
    std::uniform_int_distribution<Quantity> qty_dist(100, 1000);
    
    const std::vector<std::string> symbols = {"AAPL", "GOOGL", "MSFT", "TSLA", "AMZN"};
    std::uniform_int_distribution<size_t> symbol_dist(0, symbols.size() - 1);
    
    for (int i = 0; i < num_orders; ++i) {
        LatencyTimer timer;
        timer.start();
        
        auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
        auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(now);
        
        const std::string& symbol = symbols[symbol_dist(gen)];
        Side side = (i % 2) ? Side::BUY : Side::SELL;
        Price price = price_dist(gen);
        Quantity quantity = qty_dist(gen);
        
        Order order(i + 1, symbol, side, OrderType::LIMIT, price, quantity, timestamp);
        bool success = engine.submitOrder(order);
        
        if (success) {
            latency_stats.recordLatency(timer.elapsed());
        }
    }
    
    latency_stats.printSummary("Order Submission Latency");
}

void runThroughputTest(MarketDataEngine& engine, int duration_seconds) {
    std::cout << "\n=== Throughput Test (" << duration_seconds << " seconds) ===" << std::endl;
    
    auto handler = std::make_shared<PerformanceTestHandler>();
    engine.addEventHandler(handler);
    
    std::mt19937 gen(42);
    std::uniform_int_distribution<Price> price_dist(14500, 15500);
    std::uniform_int_distribution<Quantity> qty_dist(100, 1000);
    
    const std::vector<std::string> symbols = {"AAPL", "GOOGL", "MSFT", "TSLA", "AMZN", 
                                             "NVDA", "META", "NFLX", "AMD", "INTC"};
    std::uniform_int_distribution<size_t> symbol_dist(0, symbols.size() - 1);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    auto end_time = start_time + std::chrono::seconds(duration_seconds);
    
    uint64_t orders_submitted = 0;
    OrderId order_id = 1;
    
    while (std::chrono::high_resolution_clock::now() < end_time) {
        auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
        auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(now);
        
        const std::string& symbol = symbols[symbol_dist(gen)];
        Side side = (orders_submitted % 2) ? Side::BUY : Side::SELL;
        Price price = price_dist(gen);
        Quantity quantity = qty_dist(gen);
        
        Order order(order_id++, symbol, side, OrderType::LIMIT, price, quantity, timestamp);
        
        if (engine.submitOrder(order)) {
            ++orders_submitted;
        }
        
        // Add some variety - occasionally cancel or modify orders
        if (orders_submitted > 100 && (orders_submitted % 10) == 0) {
            // Cancel a random previous order
            OrderId cancel_id = order_id - (gen() % 50) - 1;
            engine.cancelOrder(symbol, cancel_id);
        }
        
        if (orders_submitted > 200 && (orders_submitted % 15) == 0) {
            // Modify a random previous order
            OrderId modify_id = order_id - (gen() % 100) - 1;
            engine.modifyOrder(symbol, modify_id, price_dist(gen), qty_dist(gen));
        }
    }
    
    auto actual_end = std::chrono::high_resolution_clock::now();
    auto actual_duration = std::chrono::duration<double>(actual_end - start_time);
    
    // Wait for processing to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    double throughput = orders_submitted / actual_duration.count();
    
    std::cout << "Orders submitted: " << orders_submitted << std::endl;
    std::cout << "Actual test duration: " << std::fixed << std::setprecision(2) 
             << actual_duration.count() << " seconds" << std::endl;
    std::cout << "Throughput: " << std::fixed << std::setprecision(0) 
             << throughput << " orders/second" << std::endl;
    
    std::cout << "Engine statistics:" << std::endl;
    std::cout << "  Messages processed: " << engine.getMessagesProcessed() << std::endl;
    std::cout << "  Processing errors: " << engine.getProcessingErrors() << std::endl;
    std::cout << "  Handler orders processed: " << handler->getOrdersProcessed() << std::endl;
    std::cout << "  Handler quotes received: " << handler->getQuotesReceived() << std::endl;
}

void runMemoryStressTest(MarketDataEngine& engine, int num_symbols, int orders_per_symbol) {
    std::cout << "\n=== Memory Stress Test (" << num_symbols << " symbols, " 
             << orders_per_symbol << " orders each) ===" << std::endl;
    
    std::vector<std::string> symbols;
    for (int i = 0; i < num_symbols; ++i) {
        symbols.push_back("SYM" + std::to_string(i));
    }
    
    std::mt19937 gen(42);
    std::uniform_int_distribution<Price> price_dist(10000, 20000);
    std::uniform_int_distribution<Quantity> qty_dist(100, 1000);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    OrderId order_id = 1;
    for (const auto& symbol : symbols) {
        for (int i = 0; i < orders_per_symbol; ++i) {
            auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
            auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(now);
            
            Side side = (i % 2) ? Side::BUY : Side::SELL;
            Price base_price = price_dist(gen);
            Price price = (side == Side::BUY) ? base_price - (i % 100) : base_price + (i % 100);
            
            Order order(order_id++, symbol, side, OrderType::LIMIT, price, qty_dist(gen), timestamp);
            engine.submitOrder(order);
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double>(end_time - start_time);
    
    // Wait for processing to complete
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    int total_orders = num_symbols * orders_per_symbol;
    double throughput = total_orders / duration.count();
    
    std::cout << "Total orders: " << total_orders << std::endl;
    std::cout << "Time taken: " << std::fixed << std::setprecision(2) 
             << duration.count() << " seconds" << std::endl;
    std::cout << "Throughput: " << std::fixed << std::setprecision(0) 
             << throughput << " orders/second" << std::endl;
    
    // Check some order books
    std::cout << "Sample order book statistics:" << std::endl;
    for (int i = 0; i < std::min(5, num_symbols); ++i) {
        OrderBook* book = engine.getOrderBook(symbols[i]);
        if (book) {
            std::cout << "  " << symbols[i] << ": " 
                     << book->getTotalOrders() << " orders, "
                     << "spread = $" << std::fixed << std::setprecision(2) 
                     << (book->getSpread() / 100.0) << std::endl;
        }
    }
}

int main() {
    std::cout << "Market Data Engine Performance Test Suite" << std::endl;
    std::cout << "=========================================" << std::endl;
    
    try {
        MarketDataEngine engine;
        engine.start();
        
        // Give the engine time to start
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Run various performance tests
        runLatencyTest(engine, 1000);
        runLatencyTest(engine, 10000);
        
        runThroughputTest(engine, 5);
        
        runMemoryStressTest(engine, 10, 1000);
        runMemoryStressTest(engine, 100, 100);
        
        engine.stop();
        
        std::cout << "\nPerformance tests completed successfully!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}