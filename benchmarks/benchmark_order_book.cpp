#include <benchmark/benchmark.h>
#include "core/order_book.h"
#include <memory>
#include <random>
#include <chrono>

using namespace market_data;

class OrderBookBenchmark : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State& state) override {
        book = std::make_unique<OrderBook>("AAPL");
        
        // Random number generator for realistic testing
        gen.seed(42);  // Fixed seed for reproducible results
        price_dist = std::uniform_int_distribution<Price>(14000, 16000);
        quantity_dist = std::uniform_int_distribution<Quantity>(100, 1000);
    }

    void TearDown(const benchmark::State& state) override {
        book.reset();
    }

protected:
    std::unique_ptr<OrderBook> book;
    std::mt19937 gen;
    std::uniform_int_distribution<Price> price_dist;
    std::uniform_int_distribution<Quantity> quantity_dist;
    OrderId next_order_id = 1;
    
    std::shared_ptr<Order> createRandomOrder(Side side) {
        auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
        auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(now);
        
        return std::make_shared<Order>(
            next_order_id++,
            "AAPL",
            side,
            OrderType::LIMIT,
            price_dist(gen),
            quantity_dist(gen),
            timestamp
        );
    }
};

BENCHMARK_F(OrderBookBenchmark, AddSingleOrder)(benchmark::State& state) {
    for (auto _ : state) {
        auto order = createRandomOrder(Side::BUY);
        book->addOrder(order);
        
        // Prevent accumulation affecting subsequent iterations
        if ((next_order_id % 1000) == 0) {
            book->clear();
            next_order_id = 1;
        }
    }
}

BENCHMARK_F(OrderBookBenchmark, AddMultipleOrders)(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        book->clear();
        next_order_id = 1;
        state.ResumeTiming();
        
        // Add batch of orders
        for (int i = 0; i < state.range(0); ++i) {
            auto order = createRandomOrder((i % 2) ? Side::BUY : Side::SELL);
            book->addOrder(order);
        }
    }
}

BENCHMARK_F(OrderBookBenchmark, GetBestBidAsk)(benchmark::State& state) {
    // Setup: Add some orders
    for (int i = 0; i < 100; ++i) {
        auto order = createRandomOrder((i % 2) ? Side::BUY : Side::SELL);
        book->addOrder(order);
    }
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(book->getBestBid());
        benchmark::DoNotOptimize(book->getBestAsk());
    }
}

BENCHMARK_F(OrderBookBenchmark, GetMarketDepth)(benchmark::State& state) {
    // Setup: Add many orders at different levels
    for (int i = 0; i < 1000; ++i) {
        auto order = createRandomOrder((i % 2) ? Side::BUY : Side::SELL);
        book->addOrder(order);
    }
    
    for (auto _ : state) {
        auto bid_depth = book->getMarketDepth(Side::BUY, 10);
        auto ask_depth = book->getMarketDepth(Side::SELL, 10);
        benchmark::DoNotOptimize(bid_depth);
        benchmark::DoNotOptimize(ask_depth);
    }
}

BENCHMARK_F(OrderBookBenchmark, CancelOrder)(benchmark::State& state) {
    std::vector<OrderId> order_ids;
    
    for (auto _ : state) {
        state.PauseTiming();
        
        // Setup: Add orders to cancel
        order_ids.clear();
        for (int i = 0; i < 100; ++i) {
            auto order = createRandomOrder(Side::BUY);
            order_ids.push_back(order->id);
            book->addOrder(order);
        }
        
        state.ResumeTiming();
        
        // Cancel a random order
        if (!order_ids.empty()) {
            OrderId id = order_ids[order_ids.size() / 2];
            book->cancelOrder(id);
        }
    }
}

BENCHMARK_F(OrderBookBenchmark, ModifyOrder)(benchmark::State& state) {
    std::vector<OrderId> order_ids;
    
    for (auto _ : state) {
        state.PauseTiming();
        
        // Setup: Add orders to modify
        order_ids.clear();
        for (int i = 0; i < 100; ++i) {
            auto order = createRandomOrder(Side::BUY);
            order_ids.push_back(order->id);
            book->addOrder(order);
        }
        
        state.ResumeTiming();
        
        // Modify a random order
        if (!order_ids.empty()) {
            OrderId id = order_ids[order_ids.size() / 2];
            book->modifyOrder(id, price_dist(gen), quantity_dist(gen));
        }
    }
}

BENCHMARK_F(OrderBookBenchmark, GetCurrentQuote)(benchmark::State& state) {
    // Setup: Add some orders
    for (int i = 0; i < 100; ++i) {
        auto order = createRandomOrder((i % 2) ? Side::BUY : Side::SELL);
        book->addOrder(order);
    }
    
    for (auto _ : state) {
        auto quote = book->getCurrentQuote();
        benchmark::DoNotOptimize(quote);
    }
}

// Register benchmarks with different parameters
BENCHMARK_REGISTER_F(OrderBookBenchmark, AddMultipleOrders)
    ->RangeMultiplier(2)->Range(8, 1024);