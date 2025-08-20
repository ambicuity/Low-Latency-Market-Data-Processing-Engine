#include <gtest/gtest.h>
#include "core/order_book.h"
#include <chrono>

using namespace market_data;

class OrderBookTest : public ::testing::Test {
protected:
    void SetUp() override {
        book = std::make_unique<OrderBook>("AAPL");
    }

    std::unique_ptr<OrderBook> book;
};

TEST_F(OrderBookTest, BasicOrderAddition) {
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
    auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(now);
    
    auto order = std::make_shared<Order>(1, "AAPL", Side::BUY, OrderType::LIMIT, 15000, 100, timestamp);
    
    EXPECT_TRUE(book->addOrder(order));
    EXPECT_EQ(book->getBestBid(), 15000);
    EXPECT_EQ(book->getTotalOrders(), 1);
}

TEST_F(OrderBookTest, MultipleBidLevels) {
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
    auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(now);
    
    // Add multiple bid orders at different prices
    auto order1 = std::make_shared<Order>(1, "AAPL", Side::BUY, OrderType::LIMIT, 15000, 100, timestamp);
    auto order2 = std::make_shared<Order>(2, "AAPL", Side::BUY, OrderType::LIMIT, 14900, 200, timestamp);
    auto order3 = std::make_shared<Order>(3, "AAPL", Side::BUY, OrderType::LIMIT, 15100, 150, timestamp);
    
    book->addOrder(order1);
    book->addOrder(order2);
    book->addOrder(order3);
    
    // Best bid should be the highest price
    EXPECT_EQ(book->getBestBid(), 15100);
    EXPECT_EQ(book->getTotalOrders(), 3);
    
    // Check market depth
    auto depth = book->getMarketDepth(Side::BUY, 3);
    EXPECT_EQ(depth.size(), 3);
    EXPECT_EQ(depth[0].first, 15100);  // Best bid first
    EXPECT_EQ(depth[0].second, 150);
    EXPECT_EQ(depth[1].first, 15000);
    EXPECT_EQ(depth[1].second, 100);
    EXPECT_EQ(depth[2].first, 14900);
    EXPECT_EQ(depth[2].second, 200);
}

TEST_F(OrderBookTest, MultipleAskLevels) {
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
    auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(now);
    
    // Add multiple ask orders at different prices
    auto order1 = std::make_shared<Order>(1, "AAPL", Side::SELL, OrderType::LIMIT, 15200, 100, timestamp);
    auto order2 = std::make_shared<Order>(2, "AAPL", Side::SELL, OrderType::LIMIT, 15300, 200, timestamp);
    auto order3 = std::make_shared<Order>(3, "AAPL", Side::SELL, OrderType::LIMIT, 15100, 150, timestamp);
    
    book->addOrder(order1);
    book->addOrder(order2);
    book->addOrder(order3);
    
    // Best ask should be the lowest price
    EXPECT_EQ(book->getBestAsk(), 15100);
    EXPECT_EQ(book->getTotalOrders(), 3);
    
    // Check market depth
    auto depth = book->getMarketDepth(Side::SELL, 3);
    EXPECT_EQ(depth.size(), 3);
    EXPECT_EQ(depth[0].first, 15100);  // Best ask first
    EXPECT_EQ(depth[0].second, 150);
    EXPECT_EQ(depth[1].first, 15200);
    EXPECT_EQ(depth[1].second, 100);
    EXPECT_EQ(depth[2].first, 15300);
    EXPECT_EQ(depth[2].second, 200);
}

TEST_F(OrderBookTest, OrderCancellation) {
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
    auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(now);
    
    auto order1 = std::make_shared<Order>(1, "AAPL", Side::BUY, OrderType::LIMIT, 15000, 100, timestamp);
    auto order2 = std::make_shared<Order>(2, "AAPL", Side::BUY, OrderType::LIMIT, 14900, 200, timestamp);
    
    book->addOrder(order1);
    book->addOrder(order2);
    
    EXPECT_EQ(book->getBestBid(), 15000);
    EXPECT_EQ(book->getTotalOrders(), 2);
    
    // Cancel the best bid
    EXPECT_TRUE(book->cancelOrder(1));
    EXPECT_EQ(book->getBestBid(), 14900);
    
    // Cancel non-existent order
    EXPECT_FALSE(book->cancelOrder(999));
}

TEST_F(OrderBookTest, OrderModification) {
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
    auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(now);
    
    auto order = std::make_shared<Order>(1, "AAPL", Side::BUY, OrderType::LIMIT, 15000, 100, timestamp);
    book->addOrder(order);
    
    EXPECT_EQ(book->getBestBid(), 15000);
    EXPECT_EQ(book->getQuantityAtLevel(Side::BUY, 15000), 100);
    
    // Modify order price and quantity
    EXPECT_TRUE(book->modifyOrder(1, 15100, 200));
    EXPECT_EQ(book->getBestBid(), 15100);
    EXPECT_EQ(book->getQuantityAtLevel(Side::BUY, 15100), 200);
    EXPECT_EQ(book->getQuantityAtLevel(Side::BUY, 15000), 0);  // Old level should be empty
}

TEST_F(OrderBookTest, SpreadCalculation) {
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
    auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(now);
    
    // Add bid and ask
    auto bid = std::make_shared<Order>(1, "AAPL", Side::BUY, OrderType::LIMIT, 15000, 100, timestamp);
    auto ask = std::make_shared<Order>(2, "AAPL", Side::SELL, OrderType::LIMIT, 15050, 100, timestamp);
    
    book->addOrder(bid);
    book->addOrder(ask);
    
    EXPECT_EQ(book->getSpread(), 50);
}

TEST_F(OrderBookTest, CurrentQuote) {
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
    auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(now);
    
    auto bid = std::make_shared<Order>(1, "AAPL", Side::BUY, OrderType::LIMIT, 15000, 100, timestamp);
    auto ask = std::make_shared<Order>(2, "AAPL", Side::SELL, OrderType::LIMIT, 15050, 200, timestamp);
    
    book->addOrder(bid);
    book->addOrder(ask);
    
    Quote quote = book->getCurrentQuote();
    EXPECT_EQ(quote.symbol, "AAPL");
    EXPECT_EQ(quote.bid_price, 15000);
    EXPECT_EQ(quote.bid_quantity, 100);
    EXPECT_EQ(quote.ask_price, 15050);
    EXPECT_EQ(quote.ask_quantity, 200);
}