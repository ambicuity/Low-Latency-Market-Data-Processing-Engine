#pragma once

#include <chrono>
#include <vector>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <string>

namespace market_data {

// High-resolution timer for latency measurement
class LatencyTimer {
private:
    std::chrono::high_resolution_clock::time_point start_time_;
    
public:
    void start() {
        start_time_ = std::chrono::high_resolution_clock::now();
    }
    
    std::chrono::nanoseconds elapsed() const {
        auto end_time = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time_);
    }
    
    double elapsedMicroseconds() const {
        return elapsed().count() / 1000.0;
    }
    
    double elapsedMilliseconds() const {
        return elapsed().count() / 1000000.0;
    }
};

// Performance statistics collector
class PerformanceStats {
private:
    std::vector<double> latencies_us_;
    std::chrono::high_resolution_clock::time_point start_time_;
    size_t operation_count_{0};
    
public:
    PerformanceStats() {
        start_time_ = std::chrono::high_resolution_clock::now();
        latencies_us_.reserve(1000000); // Pre-allocate for performance
    }
    
    void recordLatency(std::chrono::nanoseconds latency) {
        latencies_us_.push_back(latency.count() / 1000.0);
        ++operation_count_;
    }
    
    void recordLatency(double latency_us) {
        latencies_us_.push_back(latency_us);
        ++operation_count_;
    }
    
    size_t getOperationCount() const { return operation_count_; }
    
    double getAverageLatency() const {
        if (latencies_us_.empty()) return 0.0;
        return std::accumulate(latencies_us_.begin(), latencies_us_.end(), 0.0) / latencies_us_.size();
    }
    
    double getMinLatency() const {
        if (latencies_us_.empty()) return 0.0;
        return *std::min_element(latencies_us_.begin(), latencies_us_.end());
    }
    
    double getMaxLatency() const {
        if (latencies_us_.empty()) return 0.0;
        return *std::max_element(latencies_us_.begin(), latencies_us_.end());
    }
    
    double getPercentile(double percentile) const {
        if (latencies_us_.empty()) return 0.0;
        
        std::vector<double> sorted_latencies = latencies_us_;
        std::sort(sorted_latencies.begin(), sorted_latencies.end());
        
        size_t index = static_cast<size_t>((percentile / 100.0) * (sorted_latencies.size() - 1));
        return sorted_latencies[index];
    }
    
    double getStandardDeviation() const {
        if (latencies_us_.size() < 2) return 0.0;
        
        double mean = getAverageLatency();
        double sum_sq_diff = 0.0;
        
        for (double latency : latencies_us_) {
            double diff = latency - mean;
            sum_sq_diff += diff * diff;
        }
        
        return std::sqrt(sum_sq_diff / (latencies_us_.size() - 1));
    }
    
    double getThroughput() const {
        auto elapsed = std::chrono::high_resolution_clock::now() - start_time_;
        double elapsed_seconds = std::chrono::duration<double>(elapsed).count();
        return operation_count_ / elapsed_seconds;
    }
    
    void clear() {
        latencies_us_.clear();
        operation_count_ = 0;
        start_time_ = std::chrono::high_resolution_clock::now();
    }
    
    // Print summary statistics
    void printSummary(const std::string& operation_name = "Operation") const;
};

// RAII latency measurement helper
class ScopedLatencyMeasurement {
private:
    LatencyTimer timer_;
    PerformanceStats& stats_;
    
public:
    explicit ScopedLatencyMeasurement(PerformanceStats& stats) : stats_(stats) {
        timer_.start();
    }
    
    ~ScopedLatencyMeasurement() {
        stats_.recordLatency(timer_.elapsed());
    }
};

#define MEASURE_LATENCY(stats) ScopedLatencyMeasurement _measure(stats)

} // namespace market_data