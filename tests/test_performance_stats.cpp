#include <gtest/gtest.h>
#include "utils/performance_stats.h"
#include <thread>
#include <chrono>

using namespace market_data;

TEST(PerformanceStatsTest, BasicLatencyMeasurement) {
    LatencyTimer timer;
    timer.start();
    
    // Simulate some work
    std::this_thread::sleep_for(std::chrono::microseconds(100));
    
    auto elapsed = timer.elapsed();
    EXPECT_GT(elapsed.count(), 90000);  // At least 90 microseconds (allowing for variance)
    EXPECT_LT(elapsed.count(), 200000); // Less than 200 microseconds
    
    double elapsed_us = timer.elapsedMicroseconds();
    EXPECT_GT(elapsed_us, 90.0);
    EXPECT_LT(elapsed_us, 200.0);
}

TEST(PerformanceStatsTest, StatsCollection) {
    PerformanceStats stats;
    
    EXPECT_EQ(stats.getOperationCount(), 0);
    EXPECT_EQ(stats.getAverageLatency(), 0.0);
    
    // Add some latencies
    stats.recordLatency(100.0);  // 100 microseconds
    stats.recordLatency(200.0);  // 200 microseconds
    stats.recordLatency(150.0);  // 150 microseconds
    
    EXPECT_EQ(stats.getOperationCount(), 3);
    EXPECT_DOUBLE_EQ(stats.getAverageLatency(), 150.0);
    EXPECT_DOUBLE_EQ(stats.getMinLatency(), 100.0);
    EXPECT_DOUBLE_EQ(stats.getMaxLatency(), 200.0);
}

TEST(PerformanceStatsTest, Percentiles) {
    PerformanceStats stats;
    
    // Add latencies from 1 to 100 microseconds
    for (int i = 1; i <= 100; ++i) {
        stats.recordLatency(static_cast<double>(i));
    }
    
    EXPECT_DOUBLE_EQ(stats.getPercentile(50.0), 50.0);  // Median
    EXPECT_DOUBLE_EQ(stats.getPercentile(90.0), 90.0);  // 90th percentile
    EXPECT_DOUBLE_EQ(stats.getPercentile(95.0), 95.0);  // 95th percentile
    EXPECT_DOUBLE_EQ(stats.getPercentile(99.0), 99.0);  // 99th percentile
}

TEST(PerformanceStatsTest, StandardDeviation) {
    PerformanceStats stats;
    
    // Add identical latencies (standard deviation should be 0)
    for (int i = 0; i < 10; ++i) {
        stats.recordLatency(100.0);
    }
    
    EXPECT_DOUBLE_EQ(stats.getStandardDeviation(), 0.0);
    
    // Clear and add varied latencies
    stats.clear();
    stats.recordLatency(90.0);
    stats.recordLatency(100.0);
    stats.recordLatency(110.0);
    
    double stddev = stats.getStandardDeviation();
    EXPECT_GT(stddev, 8.0);   // Should be around 10
    EXPECT_LT(stddev, 12.0);
}

TEST(PerformanceStatsTest, ThroughputMeasurement) {
    PerformanceStats stats;
    
    // Record some operations
    for (int i = 0; i < 1000; ++i) {
        stats.recordLatency(10.0);
    }
    
    // Wait a bit to ensure elapsed time > 0
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    double throughput = stats.getThroughput();
    EXPECT_GT(throughput, 0.0);
    EXPECT_LT(throughput, 1000000.0);  // Should be reasonable
}

TEST(PerformanceStatsTest, ScopedMeasurement) {
    PerformanceStats stats;
    
    {
        MEASURE_LATENCY(stats);
        std::this_thread::sleep_for(std::chrono::microseconds(50));
    }  // Destructor should record latency
    
    EXPECT_EQ(stats.getOperationCount(), 1);
    EXPECT_GT(stats.getAverageLatency(), 40.0);  // Should be around 50 microseconds
    EXPECT_LT(stats.getAverageLatency(), 100.0);
}

TEST(PerformanceStatsTest, ClearStats) {
    PerformanceStats stats;
    
    stats.recordLatency(100.0);
    stats.recordLatency(200.0);
    
    EXPECT_EQ(stats.getOperationCount(), 2);
    EXPECT_GT(stats.getAverageLatency(), 0.0);
    
    stats.clear();
    
    EXPECT_EQ(stats.getOperationCount(), 0);
    EXPECT_EQ(stats.getAverageLatency(), 0.0);
}