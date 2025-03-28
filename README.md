# Pegasus Order Book System

A high-performance limit order book system with multi-threading support for financial trading applications.

## Overview

The Pegasus Order Book System is a C++ implementation of a limit order book with price-time priority matching. It supports multiple threading models for parallel order processing across different symbols and includes comprehensive benchmarking tools to analyze performance.

### Key Features

- Price-time priority matching engine
- Support for limit, market, and stop-limit orders
- Multiple multi-threading implementations:
  - Thread-per-symbol
  - Thread pool
  - Basic threading
  - Atomic operations
- ETH/USD cryptocurrency order book simulation
- Thread-safe operations with atomic counters and mutex protection
- Performance benchmarking infrastructure
- Interactive CLI interface

## Project Structure

```
/include         # Header files
  benchmark.h    # Benchmarking infrastructure
  limit.h        # Price level management
  order.h        # Order definitions
  orderbook.h    # Order book implementation
  orderbookmanager.h  # Multi-threaded manager
  threadpool.h   # Thread pool implementation

/src
  /benchmark     # Benchmarking implementation
    bench_orderbook.h       # Simplified order book for benchmarking
    benchmark_runner.cpp    # Command-line benchmark tool
    memory_usage.h          # Cross-platform memory monitoring

  /core          # Core order book components
    limit.cpp    # Price level implementations
    order.cpp    # Order class implementation
    orderbook.cpp          # Matching engine implementation
    orderbookmanager.cpp   # Multi-threaded order manager

  /threading     # Multi-threading implementations
    atomic_mt.cpp    # Atomic operations example
    basic_mt.cpp     # Basic multi-threading
    final_mt.cpp     # Full multi-threaded implementation
    simple_mt.cpp    # Thread pool implementation

  /examples      # Example applications
    crypto_book.cpp  # ETH/USD simulation
    main.cpp         # Single-threaded example
    main_mt.cpp      # Multi-threaded example

/build_scripts   # Build automation
  build.sh              # Basic build
  build_mt.sh           # Multi-threaded build
  build_benchmark.sh    # Benchmarking tool build
  build_crypto.sh       # Crypto simulation build
  build_basic_mt.sh     # Basic threading build
  build_simple_mt.sh    # Thread pool build
  build_final_mt.sh     # Final implementation build

pegasus_cli.sh   # Interactive CLI interface
/docs            # Documentation
```

## Interactive CLI Interface

The system includes an interactive command-line interface for easy operation. Launch it with:

```bash
./pegasus_cli.sh
```

The CLI provides:
- Interactive menu for building and running different implementations
- Performance metrics visualization
- Project information and documentation
- Benchmark configuration and execution

## Building and Running

### Using the CLI (Recommended)

The easiest way to build and run is through the CLI interface:

```bash
chmod +x pegasus_cli.sh
./pegasus_cli.sh
```

### Manual Builds

#### Basic Order Book
```bash
./build_scripts/build.sh
./pegasus
```

#### Multi-threading with OrderBookManager
```bash
./build_scripts/build_mt.sh
./pegasus_mt
```

#### Thread Pool Implementation
```bash
./build_scripts/build_simple_mt.sh
./pegasus_simple_mt
```

#### Basic Multi-threading
```bash
./build_scripts/build_basic_mt.sh
./pegasus_basic_mt
```

#### ETH/USD Simulation
```bash
./build_scripts/build_crypto.sh
./crypto_book
```

## Performance Benchmarking

The system includes a comprehensive benchmarking infrastructure to measure performance across different configurations.

### Running Benchmarks

```bash
# Build the benchmarking tool
./build_scripts/build_benchmark.sh

# Run all benchmarks with default parameters
./pegasus_benchmark --benchmark=all

# Run specific benchmark types
./pegasus_benchmark --benchmark=add
./pegasus_benchmark --benchmark=cancel
./pegasus_benchmark --benchmark=match
./pegasus_benchmark --benchmark=mixed

# Configure benchmark parameters
./pegasus_benchmark --threads=4 --symbols=10 --operations=100000 --benchmark=all

# Run the complete benchmark suite with multiple configurations
./pegasus_benchmark --benchmark=suite
```

### Benchmark Features

- Throughput measurements (orders/second)
- Latency metrics (average, median, P95, P99)
- Memory usage tracking
- Multi-threaded scaling tests
- Multi-symbol performance analysis
- CSV export for data analysis

## Build Requirements

- C++17 compatible compiler
- Standard library support for threads, atomics, and mutexes
- Platform support for memory usage reporting (Linux, macOS, Windows)

## Performance Optimization

The codebase is designed for high performance with optimizations including:
- Lock-free operations where possible
- Memory-efficient data structures
- Cache-friendly design
- Multiple threading models for different workloads
- Fine-grained locking to minimize contention

## Contributing

Contributions welcome! Areas for improvement include:
- Additional order types
- Further performance optimizations
- Extended benchmarking capabilities
- Integration with external market data sources
- GUI interfaces