#include <gtest/gtest.h>
#include "utils/lock_free_queue.h"
#include <thread>
#include <vector>
#include <atomic>

using namespace market_data;

TEST(LockFreeQueueTest, BasicPushPop) {
    LockFreeQueue<int> queue;
    
    EXPECT_TRUE(queue.empty());
    
    queue.push(42);
    EXPECT_FALSE(queue.empty());
    
    int value;
    EXPECT_TRUE(queue.try_pop(value));
    EXPECT_EQ(value, 42);
    EXPECT_TRUE(queue.empty());
}

TEST(LockFreeQueueTest, MultipleElements) {
    LockFreeQueue<int> queue;
    
    for (int i = 0; i < 10; ++i) {
        queue.push(i);
    }
    
    for (int i = 0; i < 10; ++i) {
        int value;
        EXPECT_TRUE(queue.try_pop(value));
        EXPECT_EQ(value, i);
    }
    
    EXPECT_TRUE(queue.empty());
}

TEST(LockFreeQueueTest, ConcurrentProducerConsumer) {
    LockFreeQueue<int> queue;
    constexpr int NUM_ITEMS = 10000;
    std::atomic<int> items_consumed{0};
    
    // Producer thread
    std::thread producer([&queue]() {
        for (int i = 0; i < NUM_ITEMS; ++i) {
            queue.push(i);
        }
    });
    
    // Consumer thread
    std::thread consumer([&queue, &items_consumed]() {
        int value;
        while (items_consumed.load() < NUM_ITEMS) {
            if (queue.try_pop(value)) {
                items_consumed.fetch_add(1);
            } else {
                std::this_thread::yield();
            }
        }
    });
    
    producer.join();
    consumer.join();
    
    EXPECT_EQ(items_consumed.load(), NUM_ITEMS);
}

TEST(RingBufferTest, BasicOperations) {
    RingBuffer<int, 8> buffer;
    
    EXPECT_TRUE(buffer.empty());
    EXPECT_FALSE(buffer.full());
    EXPECT_EQ(buffer.size(), 0);
    
    // Push some items
    for (int i = 0; i < 7; ++i) {
        EXPECT_TRUE(buffer.try_push(i));
    }
    
    EXPECT_FALSE(buffer.empty());
    EXPECT_TRUE(buffer.full());  // 7 items in buffer of size 8 (one slot reserved)
    EXPECT_EQ(buffer.size(), 7);
    
    // Try to push when full
    EXPECT_FALSE(buffer.try_push(999));
    
    // Pop items
    for (int i = 0; i < 7; ++i) {
        int value;
        EXPECT_TRUE(buffer.try_pop(value));
        EXPECT_EQ(value, i);
    }
    
    EXPECT_TRUE(buffer.empty());
    
    // Try to pop when empty
    int value;
    EXPECT_FALSE(buffer.try_pop(value));
}

TEST(RingBufferTest, ConcurrentAccess) {
    RingBuffer<int, 1024> buffer;
    constexpr int NUM_ITEMS = 10000;
    std::atomic<int> items_pushed{0};
    std::atomic<int> items_popped{0};
    
    // Producer thread
    std::thread producer([&buffer, &items_pushed]() {
        for (int i = 0; i < NUM_ITEMS; ++i) {
            while (!buffer.try_push(i)) {
                std::this_thread::yield();
            }
            items_pushed.fetch_add(1);
        }
    });
    
    // Consumer thread
    std::thread consumer([&buffer, &items_popped]() {
        int value;
        while (items_popped.load() < NUM_ITEMS) {
            if (buffer.try_pop(value)) {
                items_popped.fetch_add(1);
            } else {
                std::this_thread::yield();
            }
        }
    });
    
    producer.join();
    consumer.join();
    
    EXPECT_EQ(items_pushed.load(), NUM_ITEMS);
    EXPECT_EQ(items_popped.load(), NUM_ITEMS);
}

TEST(RingBufferTest, WrapAround) {
    RingBuffer<int, 4> buffer;  // Small buffer to test wrap-around
    
    // Fill buffer
    for (int i = 0; i < 3; ++i) {
        EXPECT_TRUE(buffer.try_push(i));
    }
    
    // Pop one item
    int value;
    EXPECT_TRUE(buffer.try_pop(value));
    EXPECT_EQ(value, 0);
    
    // Push another item (should wrap around)
    EXPECT_TRUE(buffer.try_push(3));
    
    // Pop remaining items
    for (int expected = 1; expected <= 3; ++expected) {
        EXPECT_TRUE(buffer.try_pop(value));
        EXPECT_EQ(value, expected);
    }
    
    EXPECT_TRUE(buffer.empty());
}