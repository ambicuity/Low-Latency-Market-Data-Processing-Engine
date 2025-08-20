#include "utils/performance_stats.h"
#include <iostream>
#include <iomanip>

namespace market_data {

void PerformanceStats::printSummary(const std::string& operation_name) const {
    std::cout << "\n=== " << operation_name << " Performance Summary ===" << std::endl;
    std::cout << std::fixed << std::setprecision(2);
    
    std::cout << "Operations: " << getOperationCount() << std::endl;
    std::cout << "Throughput: " << getThroughput() << " ops/sec" << std::endl;
    
    if (!latencies_us_.empty()) {
        std::cout << "\nLatency Statistics (microseconds):" << std::endl;
        std::cout << "  Average: " << getAverageLatency() << " μs" << std::endl;
        std::cout << "  Minimum: " << getMinLatency() << " μs" << std::endl;
        std::cout << "  Maximum: " << getMaxLatency() << " μs" << std::endl;
        std::cout << "  Std Dev: " << getStandardDeviation() << " μs" << std::endl;
        
        std::cout << "\nPercentiles:" << std::endl;
        std::cout << "  50th: " << getPercentile(50.0) << " μs" << std::endl;
        std::cout << "  90th: " << getPercentile(90.0) << " μs" << std::endl;
        std::cout << "  95th: " << getPercentile(95.0) << " μs" << std::endl;
        std::cout << "  99th: " << getPercentile(99.0) << " μs" << std::endl;
        std::cout << "  99.9th: " << getPercentile(99.9) << " μs" << std::endl;
    }
    
    std::cout << std::endl;
}

} // namespace market_data