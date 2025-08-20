#include <benchmark/benchmark.h>
#include "engine/market_data_engine.h"
#include <memory>
#include <chrono>
#include <thread>

using namespace market_data;

class EngineBenchmark : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State& state) override {
        engine = std::make_unique<MarketDataEngine>();
        engine->start();
        
        // Give the engine a moment to start
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        next_order_id = 1;
    }

    void TearDown(const benchmark::State& state) override {
        engine->stop();
        engine.reset();
    }

protected:
    std::unique_ptr<MarketDataEngine> engine;
    OrderId next_order_id = 1;
    
    Order createOrder(const std::string& symbol, Side side, Price price, Quantity quantity) {
        auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
        auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(now);
        return Order(next_order_id++, symbol, side, OrderType::LIMIT, price, quantity, timestamp);
    }
};

BENCHMARK_F(EngineBenchmark, SubmitSingleOrder)(benchmark::State& state) {
    for (auto _ : state) {
        Order order = createOrder("AAPL", Side::BUY, 15000, 100);
        bool success = engine->submitOrder(order);
        benchmark::DoNotOptimize(success);
    }
    
    // Allow processing to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

BENCHMARK_F(EngineBenchmark, SubmitOrderBatch)(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        std::vector<Order> orders;
        orders.reserve(state.range(0));
        
        for (int i = 0; i < state.range(0); ++i) {
            orders.emplace_back(createOrder("AAPL", (i % 2) ? Side::BUY : Side::SELL, 
                                          15000 + (i % 100), 100));
        }
        state.ResumeTiming();
        
        for (const auto& order : orders) {
            engine->submitOrder(order);
        }
    }
    
    // Allow processing to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

BENCHMARK_F(EngineBenchmark, OrderCancellation)(benchmark::State& state) {
    std::vector<OrderId> order_ids;
    
    // Pre-fill with orders to cancel
    for (int i = 0; i < 1000; ++i) {
        Order order = createOrder("AAPL", Side::BUY, 15000 + i, 100);
        order_ids.push_back(order.id);
        engine->submitOrder(order);
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    size_t index = 0;
    for (auto _ : state) {
        if (index < order_ids.size()) {
            bool success = engine->cancelOrder("AAPL", order_ids[index++]);
            benchmark::DoNotOptimize(success);
        }
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

BENCHMARK_F(EngineBenchmark, OrderModification)(benchmark::State& state) {
    std::vector<OrderId> order_ids;
    
    // Pre-fill with orders to modify
    for (int i = 0; i < 1000; ++i) {
        Order order = createOrder("AAPL", Side::BUY, 15000 + i, 100);
        order_ids.push_back(order.id);
        engine->submitOrder(order);
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    size_t index = 0;
    for (auto _ : state) {
        if (index < order_ids.size()) {
            bool success = engine->modifyOrder("AAPL", order_ids[index++], 15500, 200);
            benchmark::DoNotOptimize(success);
        }
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

BENCHMARK_F(EngineBenchmark, GetCurrentQuote)(benchmark::State& state) {
    // Setup: Add some orders
    for (int i = 0; i < 100; ++i) {
        Order order = createOrder("AAPL", (i % 2) ? Side::BUY : Side::SELL, 
                                15000 + (i % 50), 100);
        engine->submitOrder(order);
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    for (auto _ : state) {
        Quote quote = engine->getCurrentQuote("AAPL");
        benchmark::DoNotOptimize(quote);
    }
}

BENCHMARK_F(EngineBenchmark, MultiSymbolProcessing)(benchmark::State& state) {
    const std::vector<std::string> symbols = {"AAPL", "GOOGL", "MSFT", "TSLA", "AMZN"};
    
    for (auto _ : state) {
        for (const auto& symbol : symbols) {
            Order order = createOrder(symbol, Side::BUY, 15000, 100);
            engine->submitOrder(order);
        }
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
}

BENCHMARK_F(EngineBenchmark, HighFrequencyTrading)(benchmark::State& state) {
    // Simulate high-frequency trading scenario
    Price base_price = 15000;
    
    for (auto _ : state) {
        // Submit rapid-fire orders
        for (int i = 0; i < 10; ++i) {
            Order buy_order = createOrder("AAPL", Side::BUY, base_price - i, 100);
            Order sell_order = createOrder("AAPL", Side::SELL, base_price + i + 10, 100);
            
            engine->submitOrder(buy_order);
            engine->submitOrder(sell_order);
        }
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

// Register benchmarks with different parameters
BENCHMARK_REGISTER_F(EngineBenchmark, SubmitOrderBatch)
    ->RangeMultiplier(2)->Range(8, 512);