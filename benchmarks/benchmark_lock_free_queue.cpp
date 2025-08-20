#include <benchmark/benchmark.h>
#include "utils/lock_free_queue.h"
#include <thread>
#include <vector>

using namespace market_data;

static void BenchmarkLockFreeQueuePush(benchmark::State& state) {
    LockFreeQueue<int> queue;
    int value = 42;
    
    for (auto _ : state) {
        queue.push(value);
    }
}
BENCHMARK(BenchmarkLockFreeQueuePush);

static void BenchmarkLockFreeQueuePop(benchmark::State& state) {
    LockFreeQueue<int> queue;
    
    // Pre-fill queue
    for (int i = 0; i < state.range(0); ++i) {
        queue.push(i);
    }
    
    int value;
    for (auto _ : state) {
        if (!queue.try_pop(value)) {
            // Refill if empty
            for (int i = 0; i < 1000; ++i) {
                queue.push(i);
            }
            queue.try_pop(value);
        }
    }
}
BENCHMARK(BenchmarkLockFreeQueuePop)->Range(1000, 10000);

static void BenchmarkRingBufferPush(benchmark::State& state) {
    RingBuffer<int, 1024> buffer;
    int value = 42;
    
    for (auto _ : state) {
        while (!buffer.try_push(value)) {
            // If buffer full, pop one item to make space
            int dummy;
            buffer.try_pop(dummy);
        }
    }
}
BENCHMARK(BenchmarkRingBufferPush);

static void BenchmarkRingBufferPop(benchmark::State& state) {
    RingBuffer<int, 1024> buffer;
    
    // Pre-fill buffer
    for (int i = 0; i < 512; ++i) {
        buffer.try_push(i);
    }
    
    int value;
    for (auto _ : state) {
        if (!buffer.try_pop(value)) {
            // Refill if empty
            for (int i = 0; i < 512; ++i) {
                buffer.try_push(i);
            }
            buffer.try_pop(value);
        }
    }
}
BENCHMARK(BenchmarkRingBufferPop);

static void BenchmarkRingBufferPushPop(benchmark::State& state) {
    RingBuffer<int, 1024> buffer;
    int push_value = 42;
    int pop_value;
    
    for (auto _ : state) {
        buffer.try_push(push_value);
        buffer.try_pop(pop_value);
        benchmark::DoNotOptimize(pop_value);
    }
}
BENCHMARK(BenchmarkRingBufferPushPop);

// Concurrent access benchmarks
static void BenchmarkRingBufferConcurrentAccess(benchmark::State& state) {
    RingBuffer<int, 4096> buffer;
    std::atomic<bool> stop{false};
    
    // Producer thread
    std::thread producer([&buffer, &stop]() {
        int value = 0;
        while (!stop.load()) {
            if (buffer.try_push(value)) {
                ++value;
            }
            std::this_thread::yield();
        }
    });
    
    // Consumer (benchmark thread)
    int pop_value;
    for (auto _ : state) {
        if (buffer.try_pop(pop_value)) {
            benchmark::DoNotOptimize(pop_value);
        }
    }
    
    stop.store(true);
    producer.join();
}
BENCHMARK(BenchmarkRingBufferConcurrentAccess);

// Test different ring buffer sizes
template<size_t Size>
static void BenchmarkRingBufferSizePushPop(benchmark::State& state) {
    RingBuffer<int, Size> buffer;
    int push_value = 42;
    int pop_value;
    
    for (auto _ : state) {
        buffer.try_push(push_value);
        buffer.try_pop(pop_value);
        benchmark::DoNotOptimize(pop_value);
    }
}

BENCHMARK_TEMPLATE(BenchmarkRingBufferSizePushPop, 64);
BENCHMARK_TEMPLATE(BenchmarkRingBufferSizePushPop, 256);
BENCHMARK_TEMPLATE(BenchmarkRingBufferSizePushPop, 1024);
BENCHMARK_TEMPLATE(BenchmarkRingBufferSizePushPop, 4096);