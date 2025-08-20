#include <gtest/gtest.h>
#include "engine/market_data_engine.h"
#include <chrono>
#include <thread>

using namespace market_data;

class TestEventHandler : public MarketDataEventHandler {
public:
    std::atomic<int> orders_added{0};
    std::atomic<int> orders_cancelled{0};
    std::atomic<int> orders_modified{0};
    std::atomic<int> trades_received{0};
    std::atomic<int> quotes_received{0};
    
    void onOrderAdded(const Order& order) override {
        orders_added.fetch_add(1);
    }
    
    void onOrderCancelled(OrderId order_id) override {
        orders_cancelled.fetch_add(1);
    }
    
    void onOrderModified(const Order& order) override {
        orders_modified.fetch_add(1);
    }
    
    void onTrade(const Trade& trade) override {
        trades_received.fetch_add(1);
    }
    
    void onQuoteUpdate(const Quote& quote) override {
        quotes_received.fetch_add(1);
    }
};

class MarketDataEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        engine = std::make_unique<MarketDataEngine>();
        handler = std::make_shared<TestEventHandler>();
        engine->addEventHandler(handler);
        engine->start();
        
        // Give the engine a moment to start
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    void TearDown() override {
        engine->stop();
    }

    std::unique_ptr<MarketDataEngine> engine;
    std::shared_ptr<TestEventHandler> handler;
};

TEST_F(MarketDataEngineTest, BasicOrderSubmission) {
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
    auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(now);
    
    Order order(1, "AAPL", Side::BUY, OrderType::LIMIT, 15000, 100, timestamp);
    
    EXPECT_TRUE(engine->submitOrder(order));
    
    // Wait for processing
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    EXPECT_EQ(handler->orders_added.load(), 1);
    EXPECT_EQ(handler->quotes_received.load(), 1);  // Quote should be generated
}

TEST_F(MarketDataEngineTest, OrderCancellation) {
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
    auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(now);
    
    Order order(1, "AAPL", Side::BUY, OrderType::LIMIT, 15000, 100, timestamp);
    
    EXPECT_TRUE(engine->submitOrder(order));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    EXPECT_TRUE(engine->cancelOrder("AAPL", 1));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    EXPECT_EQ(handler->orders_added.load(), 1);
    EXPECT_EQ(handler->orders_cancelled.load(), 1);
    EXPECT_GE(handler->quotes_received.load(), 2);  // At least 2 quotes (add + cancel)
}

TEST_F(MarketDataEngineTest, OrderModification) {
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
    auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(now);
    
    Order order(1, "AAPL", Side::BUY, OrderType::LIMIT, 15000, 100, timestamp);
    
    EXPECT_TRUE(engine->submitOrder(order));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    EXPECT_TRUE(engine->modifyOrder("AAPL", 1, 15100, 200));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    EXPECT_EQ(handler->orders_added.load(), 1);
    EXPECT_EQ(handler->orders_modified.load(), 1);
    EXPECT_GE(handler->quotes_received.load(), 2);  // At least 2 quotes (add + modify)
}

TEST_F(MarketDataEngineTest, MultipleOrders) {
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
    auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(now);
    
    constexpr int NUM_ORDERS = 100;
    
    for (int i = 0; i < NUM_ORDERS; ++i) {
        Order order(i, "AAPL", Side::BUY, OrderType::LIMIT, 15000 + i, 100, timestamp);
        EXPECT_TRUE(engine->submitOrder(order));
    }
    
    // Wait for all orders to be processed
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    EXPECT_EQ(handler->orders_added.load(), NUM_ORDERS);
    EXPECT_GE(handler->quotes_received.load(), NUM_ORDERS);
}

TEST_F(MarketDataEngineTest, OrderBookAccess) {
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
    auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(now);
    
    Order order(1, "AAPL", Side::BUY, OrderType::LIMIT, 15000, 100, timestamp);
    
    EXPECT_TRUE(engine->submitOrder(order));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    OrderBook* book = engine->getOrderBook("AAPL");
    ASSERT_NE(book, nullptr);
    EXPECT_EQ(book->getBestBid(), 15000);
    
    Quote quote = engine->getCurrentQuote("AAPL");
    EXPECT_EQ(quote.symbol, "AAPL");
    EXPECT_EQ(quote.bid_price, 15000);
    EXPECT_EQ(quote.bid_quantity, 100);
}

TEST_F(MarketDataEngineTest, PerformanceStatistics) {
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
    auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(now);
    
    constexpr int NUM_ORDERS = 10;
    
    for (int i = 0; i < NUM_ORDERS; ++i) {
        Order order(i, "AAPL", Side::BUY, OrderType::LIMIT, 15000 + i, 100, timestamp);
        EXPECT_TRUE(engine->submitOrder(order));
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    EXPECT_EQ(engine->getMessagesProcessed(), NUM_ORDERS);
    EXPECT_EQ(engine->getProcessingErrors(), 0);
}