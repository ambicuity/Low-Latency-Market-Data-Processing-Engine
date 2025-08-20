# Low-Latency Market Data Processing Engine

A high-performance C++ market data processing engine designed for low-latency environments, focusing on efficient data structures and algorithms to handle real-time market data.

## Features

- **Ultra-Low Latency**: Lock-free data structures and optimized algorithms for minimal processing delays
- **High Throughput**: Capable of processing hundreds of thousands of orders per second
- **Real-Time Order Book Management**: Efficient order book implementation with O(log n) insertion/deletion
- **Lock-Free Message Queues**: SPSC and ring buffer implementations for high-performance inter-thread communication
- **Memory Efficiency**: Custom memory management with object pooling and cache-friendly data layouts
- **Multi-Symbol Support**: Handle multiple trading instruments simultaneously
- **Event-Driven Architecture**: Extensible event handling system for market data callbacks
- **Comprehensive Testing**: Unit tests and performance benchmarks included
- **Performance Monitoring**: Built-in latency measurement and throughput statistics

## Architecture

### Core Components

1. **MarketDataEngine**: Main processing engine that coordinates all operations
2. **OrderBook**: High-performance order book with efficient price-level management
3. **LockFreeQueue**: Lock-free message queues for inter-thread communication
4. **PerformanceStats**: Latency measurement and performance analysis tools

### Data Structures

- **Order**: Represents a trading order with all relevant fields
- **Trade**: Represents an executed trade
- **Quote**: Market quote with bid/ask prices and quantities
- **BookLevel**: Price level in the order book with aggregated quantities

## Performance Characteristics

- **Latency**: Sub-microsecond order processing in optimal conditions
- **Throughput**: 500K+ orders per second on modern hardware
- **Memory Footprint**: Minimal dynamic allocation during steady-state operation
- **Scalability**: Efficient handling of hundreds of symbols simultaneously

## Building

### Prerequisites

- CMake 3.14 or higher
- C++17 compatible compiler (GCC 8+, Clang 8+, MSVC 2019+)
- (Optional) Google Test for running unit tests
- (Optional) Google Benchmark for performance testing

### Build Instructions

```bash
# Clone the repository
git clone https://github.com/ambicuity/Low-Latency-Market-Data-Processing-Engine.git
cd Low-Latency-Market-Data-Processing-Engine

# Create build directory
mkdir build && cd build

# Configure and build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

# Run tests (optional)
make test

# Run benchmarks (optional)
./benchmarks/market_data_benchmarks
```

## Usage

### Basic Example

```cpp
#include "engine/market_data_engine.h"

using namespace market_data;

// Create custom event handler
class MyEventHandler : public MarketDataEventHandler {
public:
    void onQuoteUpdate(const Quote& quote) override {
        std::cout << quote.symbol << " Bid: " << quote.bid_price 
                  << " Ask: " << quote.ask_price << std::endl;
    }
};

int main() {
    // Create and start engine
    MarketDataEngine engine;
    engine.addEventHandler(std::make_shared<MyEventHandler>());
    engine.start();
    
    // Submit orders
    auto timestamp = std::chrono::high_resolution_clock::now().time_since_epoch();
    Order order(1, "AAPL", Side::BUY, OrderType::LIMIT, 15000, 100, 
                std::chrono::duration_cast<std::chrono::nanoseconds>(timestamp));
    
    engine.submitOrder(order);
    
    // Get current quote
    Quote quote = engine.getCurrentQuote("AAPL");
    
    engine.stop();
    return 0;
}
```

### Running Examples

```bash
# Basic functionality demonstration
./examples/market_data_example

# Performance testing
./examples/performance_test
```

## API Reference

### MarketDataEngine

Main engine class for processing market data.

**Key Methods:**
- `start()`: Start the processing engine
- `stop()`: Stop the processing engine
- `submitOrder(const Order& order)`: Submit a new order
- `cancelOrder(const Symbol& symbol, OrderId order_id)`: Cancel an existing order
- `modifyOrder(const Symbol& symbol, OrderId order_id, Price new_price, Quantity new_quantity)`: Modify an existing order
- `getCurrentQuote(const Symbol& symbol)`: Get current market quote
- `getOrderBook(const Symbol& symbol)`: Access order book for a symbol

### OrderBook

High-performance order book implementation.

**Key Methods:**
- `addOrder(std::shared_ptr<Order> order)`: Add order to book
- `cancelOrder(OrderId order_id)`: Remove order from book
- `getBestBid()`: Get best bid price
- `getBestAsk()`: Get best ask price
- `getMarketDepth(Side side, size_t levels)`: Get market depth
- `getCurrentQuote()`: Get current quote

### Performance Utilities

**LatencyTimer**: High-resolution latency measurement
```cpp
LatencyTimer timer;
timer.start();
// ... operation to measure ...
auto latency = timer.elapsed();
```

**PerformanceStats**: Statistical analysis of performance metrics
```cpp
PerformanceStats stats;
stats.recordLatency(latency);
stats.printSummary("Operation");
```

## Performance Optimization Tips

1. **CPU Affinity**: Pin processing thread to dedicated CPU core
2. **Memory Pre-allocation**: Pre-allocate memory pools for orders and messages
3. **Compiler Optimizations**: Use `-O3 -march=native -flto` for maximum performance
4. **NUMA Awareness**: Consider NUMA topology for multi-socket systems
5. **Huge Pages**: Use huge pages for large memory allocations
6. **Real-Time Kernel**: Consider using RT kernel for consistent latencies

## Testing

### Unit Tests

Run comprehensive unit tests:
```bash
./tests/market_data_tests
```

### Benchmarks

Run performance benchmarks:
```bash
./benchmarks/market_data_benchmarks
```

### Performance Testing

Run performance test suite:
```bash
./examples/performance_test
```

## Project Structure

```
├── include/
│   ├── core/           # Core data structures
│   ├── engine/         # Main processing engine
│   └── utils/          # Utility classes
├── src/
│   ├── core/           # Core implementations
│   ├── engine/         # Engine implementations
│   └── utils/          # Utility implementations
├── tests/              # Unit tests
├── benchmarks/         # Performance benchmarks
├── examples/           # Example applications
└── CMakeLists.txt      # Build configuration
```

## Contributing

1. Fork the repository
2. Create a feature branch
3. Add tests for new functionality
4. Ensure all tests pass
5. Submit a pull request

## License

This project is open source. See LICENSE file for details.

## Performance Results

Typical performance characteristics on modern hardware (Intel i7-10700K, 32GB RAM):

- **Order Submission Latency**: ~200-500 nanoseconds (95th percentile)
- **Quote Update Latency**: ~100-300 nanoseconds (95th percentile)
- **Throughput**: 500,000+ orders per second
- **Memory Usage**: ~50MB for 100K active orders across 100 symbols

## Acknowledgments

This engine demonstrates high-performance systems programming concepts and is suitable for educational and development purposes. For production trading systems, additional features like network handling, persistence, risk management, and regulatory compliance would be required.