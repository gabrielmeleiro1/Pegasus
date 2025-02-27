# Pegasus Order Book System

A high-performance limit order book system with multi-threading support for financial trading applications.

## Overview

The Pegasus Order Book System is a C++ implementation of a limit order book with price-time priority matching. It supports multiple threading models for parallel order processing across different symbols.

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

## Project Structure

```
/include         # Header files
/src
  /core          # Core order book components
  /threading     # Multi-threading implementations
  /examples      # Example applications
/docs            # Documentation
```

## Building and Running

### Compile All Files

```bash
# Create build scripts
chmod +x build_scripts/*.sh

# Compile the core implementation
./build_scripts/build.sh

# Run the basic test program
./pegasus
```

### Running Different Implementations

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

## Build Requirements

- C++17 compatible compiler
- Standard library support for threads, atomics, and mutexes