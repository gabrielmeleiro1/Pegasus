#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <chrono>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <functional>
#include <random>
#include <atomic>
#include <thread>
#include <mutex>
#include <map>
#include <algorithm>
#include <numeric>
#include <memory>
#include "orderbook.h"
#include "../src/benchmark/bench_orderbook.h"
#include "../src/benchmark/memory_usage.h"

namespace pegasus {

/**
 * @brief Performance benchmarking infrastructure for Pegasus order book system
 * 
 * Provides functionality to measure throughput, latency, and memory usage
 * for various order book operations across different implementations and configurations.
 */
class OrderBookBenchmark {
public:
    // Define benchmark operation types
    enum class OperationType {
        ADD_ORDER,
        CANCEL_ORDER,
        MATCH_ORDER,
        MIXED_WORKLOAD
    };

    // Benchmark result structure
    struct BenchmarkResult {
        std::string name;                   // Name of the benchmark
        OperationType type;                 // Type of operation benchmarked
        size_t operationCount;              // Total number of operations performed
        size_t threadCount;                 // Number of threads used
        size_t symbolCount;                 // Number of symbols used
        double durationSec;                 // Total duration in seconds
        double operationsPerSecond;         // Throughput in operations/sec
        double averageLatencyNs;            // Average latency in nanoseconds
        double p50LatencyNs;                // 50th percentile latency (median)
        double p95LatencyNs;                // 95th percentile latency
        double p99LatencyNs;                // 99th percentile latency
        double maxLatencyNs;                // Maximum observed latency
        size_t memoryUsageBytes;            // Memory usage in bytes
        std::vector<double> latencies;      // Individual operation latencies

        // Print result in human-readable format
        void print() const {
            std::cout << "=============================" << std::endl;
            std::cout << "Benchmark: " << name << std::endl;
            std::cout << "=============================" << std::endl;
            std::cout << "Configuration:" << std::endl;
            std::cout << "  - Threads: " << threadCount << std::endl;
            std::cout << "  - Symbols: " << symbolCount << std::endl;
            std::cout << "  - Operations: " << operationCount << std::endl;
            std::cout << "  - Duration: " << std::fixed << std::setprecision(2) << durationSec << " sec" << std::endl;
            
            std::cout << "Throughput:" << std::endl;
            std::cout << "  - " << std::fixed << std::setprecision(2) << operationsPerSecond 
                      << " ops/sec" << std::endl;
            
            std::cout << "Latency:" << std::endl;
            std::cout << "  - Average: " << std::fixed << std::setprecision(2) << averageLatencyNs 
                      << " ns" << std::endl;
            std::cout << "  - Median (P50): " << std::fixed << std::setprecision(2) << p50LatencyNs 
                      << " ns" << std::endl;
            std::cout << "  - P95: " << std::fixed << std::setprecision(2) << p95LatencyNs 
                      << " ns" << std::endl;
            std::cout << "  - P99: " << std::fixed << std::setprecision(2) << p99LatencyNs 
                      << " ns" << std::endl;
            std::cout << "  - Max: " << std::fixed << std::setprecision(2) << maxLatencyNs 
                      << " ns" << std::endl;
            
            std::cout << "Memory:" << std::endl;
            std::cout << "  - Total: " << (memoryUsageBytes / 1024.0 / 1024.0) 
                      << " MB" << std::endl;
            if (operationCount > 0) {
                std::cout << "  - Per operation: " << (memoryUsageBytes / static_cast<double>(operationCount)) 
                          << " bytes" << std::endl;
            }
            std::cout << "=============================" << std::endl;
        }
        
        // Save result to CSV file
        void saveToCSV(const std::string& filename) const {
            bool fileExists = std::ifstream(filename).good();
            
            std::ofstream file;
            file.open(filename, std::ios::app);
            
            // Write header if file doesn't exist
            if (!fileExists) {
                file << "Name,OperationType,OperationCount,ThreadCount,SymbolCount,Duration(s),"
                     << "Throughput(ops/s),AvgLatency(ns),P50Latency(ns),P95Latency(ns),"
                     << "P99Latency(ns),MaxLatency(ns),MemoryUsage(bytes)\n";
            }
            
            // Write result data
            file << name << ","
                 << static_cast<int>(type) << ","
                 << operationCount << ","
                 << threadCount << ","
                 << symbolCount << ","
                 << durationSec << ","
                 << operationsPerSecond << ","
                 << averageLatencyNs << ","
                 << p50LatencyNs << ","
                 << p95LatencyNs << ","
                 << p99LatencyNs << ","
                 << maxLatencyNs << ","
                 << memoryUsageBytes << "\n";
            
            file.close();
        }
    };

    OrderBookBenchmark() 
        : m_symbolCount(1),
          m_threadCount(1),
          m_operationCount(100000),
          m_rng(std::random_device{}())
    {
        // Default initialization
        m_symbols = {"BTC/USD"};
    }

    // Set number of symbols to benchmark
    void setSymbolCount(size_t count) {
        m_symbolCount = count;
        m_symbols.clear();
        
        // Generate symbol names
        static const std::vector<std::string> baseAssets = {
            "BTC", "ETH", "SOL", "ADA", "DOT", "AVAX", "MATIC", "LINK", "XRP", "DOGE",
            "SHIB", "LTC", "UNI", "ATOM", "ETC", "XLM", "ALGO", "MANA", "SAND", "AXS"
        };
        
        static const std::vector<std::string> quoteAssets = {
            "USD", "USDT", "USDC", "EUR", "BTC", "ETH"
        };
        
        // Generate specified number of symbols
        for (size_t i = 0; i < count && i < baseAssets.size() * quoteAssets.size(); ++i) {
            size_t baseIdx = i % baseAssets.size();
            size_t quoteIdx = (i / baseAssets.size()) % quoteAssets.size();
            
            // Don't create pairs with same base and quote
            if (baseAssets[baseIdx] == quoteAssets[quoteIdx]) {
                quoteIdx = (quoteIdx + 1) % quoteAssets.size();
            }
            
            m_symbols.push_back(baseAssets[baseIdx] + "/" + quoteAssets[quoteIdx]);
        }
    }
    
    // Set number of threads to use
    void setThreadCount(size_t count) {
        m_threadCount = count;
    }
    
    // Set number of operations per benchmark
    void setOperationCount(size_t count) {
        m_operationCount = count;
    }

    // Generate test orders
    std::vector<std::shared_ptr<Order>> generateOrders(size_t count, bool randomSides = true) {
        std::vector<std::shared_ptr<Order>> orders;
        orders.reserve(count);
        
        std::uniform_int_distribution<> symbolDist(0, m_symbols.size() - 1);
        std::uniform_int_distribution<> sideDist(0, 1);
        std::uniform_real_distribution<> priceDist(100.0, 10000.0);
        std::uniform_real_distribution<> qtyDist(0.1, 10.0);
        
        static std::atomic<Order::OrderID> nextOrderId{1};
        
        for (size_t i = 0; i < count; ++i) {
            std::string symbol = m_symbols[symbolDist(m_rng)];
            Side side = randomSides ? (sideDist(m_rng) == 0 ? Side::BUY : Side::SELL) 
                                   : (i % 2 == 0 ? Side::BUY : Side::SELL);
            double price = std::round(priceDist(m_rng) * 100) / 100; // Round to 2 decimals
            double quantity = std::round(qtyDist(m_rng) * 1000) / 1000; // Round to 3 decimals
            
            orders.push_back(std::make_shared<Order>(
                nextOrderId++, side, OrderType::LIMIT, symbol, price, quantity
            ));
        }
        
        return orders;
    }

    // Run order addition benchmark
    BenchmarkResult benchmarkOrderAddition(size_t warmupCount = 1000) {
        BenchmarkResult result;
        result.name = "Order Addition Benchmark";
        result.type = OperationType::ADD_ORDER;
        result.operationCount = m_operationCount;
        result.threadCount = m_threadCount;
        result.symbolCount = m_symbolCount;
        
        // Create book manager
        BenchOrderBookManager manager;
        
        // Initialize order books for all symbols
        for (const auto& symbol : m_symbols) {
            manager.createOrderBook(symbol);
        }
        
        // Generate test orders
        auto orders = generateOrders(m_operationCount + warmupCount);
        
        // Warm up with a few orders to initialize data structures
        for (size_t i = 0; i < warmupCount; ++i) {
            manager.addOrder(orders[i], nullptr);
        }
        
        // Start memory measurement
        size_t startMemory = getCurrentMemoryUsage();
        
        // Prepare latency data
        std::vector<double> latencies;
        latencies.reserve(m_operationCount);
        
        // Start timing
        auto startTime = std::chrono::high_resolution_clock::now();
        
        if (m_threadCount == 1) {
            // Single-threaded benchmark
            for (size_t i = warmupCount; i < orders.size(); ++i) {
                auto start = std::chrono::high_resolution_clock::now();
                
                manager.addOrder(orders[i], nullptr);
                
                auto end = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
                latencies.push_back(static_cast<double>(duration));
            }
        } else {
            // Multi-threaded benchmark
            std::vector<std::thread> threads;
            std::vector<std::vector<double>> threadLatencies(m_threadCount);
            std::atomic<size_t> nextOrderIndex{warmupCount};
            
            for (size_t t = 0; t < m_threadCount; ++t) {
                threads.emplace_back([&, t]() {
                    threadLatencies[t].reserve(m_operationCount / m_threadCount);
                    
                    while (true) {
                        size_t i = nextOrderIndex.fetch_add(1, std::memory_order_relaxed);
                        if (i >= orders.size()) break;
                        
                        auto start = std::chrono::high_resolution_clock::now();
                        
                        manager.addOrder(orders[i], nullptr);
                        
                        auto end = std::chrono::high_resolution_clock::now();
                        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
                        threadLatencies[t].push_back(static_cast<double>(duration));
                    }
                });
            }
            
            // Wait for all threads to finish
            for (auto& thread : threads) {
                thread.join();
            }
            
            // Combine latencies from all threads
            for (const auto& threadLatency : threadLatencies) {
                latencies.insert(latencies.end(), threadLatency.begin(), threadLatency.end());
            }
        }
        
        // End timing
        auto endTime = std::chrono::high_resolution_clock::now();
        
        // End memory measurement
        size_t endMemory = getCurrentMemoryUsage();
        
        // Calculate results
        result.durationSec = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count() / 1000000.0;
        result.operationsPerSecond = m_operationCount / result.durationSec;
        result.memoryUsageBytes = endMemory > startMemory ? endMemory - startMemory : 0;
        result.latencies = std::move(latencies);
        
        // Calculate latency statistics
        calculateLatencyStatistics(result);
        
        return result;
    }
    
    // Run order cancellation benchmark
    BenchmarkResult benchmarkOrderCancellation() {
        BenchmarkResult result;
        result.name = "Order Cancellation Benchmark";
        result.type = OperationType::CANCEL_ORDER;
        result.operationCount = m_operationCount;
        result.threadCount = m_threadCount;
        result.symbolCount = m_symbolCount;
        
        // Create book manager
        BenchOrderBookManager manager;
        
        // Initialize order books for all symbols
        for (const auto& symbol : m_symbols) {
            manager.createOrderBook(symbol);
        }
        
        // Generate test orders
        auto orders = generateOrders(m_operationCount * 2); // 2x orders since we'll cancel half
        
        // Add all orders first
        for (size_t i = 0; i < orders.size(); ++i) {
            manager.addOrder(orders[i], nullptr);
        }
        
        // Start memory measurement
        size_t startMemory = getCurrentMemoryUsage();
        
        // Prepare latency data
        std::vector<double> latencies;
        latencies.reserve(m_operationCount);
        
        // Start timing
        auto startTime = std::chrono::high_resolution_clock::now();
        
        if (m_threadCount == 1) {
            // Single-threaded benchmark
            for (size_t i = 0; i < m_operationCount; ++i) {
                auto start = std::chrono::high_resolution_clock::now();
                
                manager.cancelOrder(orders[i]->getID());
                
                auto end = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
                latencies.push_back(static_cast<double>(duration));
            }
        } else {
            // Multi-threaded benchmark
            std::vector<std::thread> threads;
            std::vector<std::vector<double>> threadLatencies(m_threadCount);
            std::atomic<size_t> nextOrderIndex{0};
            
            for (size_t t = 0; t < m_threadCount; ++t) {
                threads.emplace_back([&, t]() {
                    threadLatencies[t].reserve(m_operationCount / m_threadCount);
                    
                    while (true) {
                        size_t i = nextOrderIndex.fetch_add(1, std::memory_order_relaxed);
                        if (i >= m_operationCount) break;
                        
                        auto start = std::chrono::high_resolution_clock::now();
                        
                        manager.cancelOrder(orders[i]->getID());
                        
                        auto end = std::chrono::high_resolution_clock::now();
                        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
                        threadLatencies[t].push_back(static_cast<double>(duration));
                    }
                });
            }
            
            // Wait for all threads to finish
            for (auto& thread : threads) {
                thread.join();
            }
            
            // Combine latencies from all threads
            for (const auto& threadLatency : threadLatencies) {
                latencies.insert(latencies.end(), threadLatency.begin(), threadLatency.end());
            }
        }
        
        // End timing
        auto endTime = std::chrono::high_resolution_clock::now();
        
        // End memory measurement
        size_t endMemory = getCurrentMemoryUsage();
        
        // Calculate results
        result.durationSec = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count() / 1000000.0;
        result.operationsPerSecond = m_operationCount / result.durationSec;
        result.memoryUsageBytes = startMemory > endMemory ? startMemory - endMemory : 0; // Memory should decrease
        result.latencies = std::move(latencies);
        
        // Calculate latency statistics
        calculateLatencyStatistics(result);
        
        return result;
    }
    
    // Run order matching benchmark
    BenchmarkResult benchmarkOrderMatching() {
        BenchmarkResult result;
        result.name = "Order Matching Benchmark";
        result.type = OperationType::MATCH_ORDER;
        result.operationCount = m_operationCount;
        result.threadCount = m_threadCount;
        result.symbolCount = m_symbolCount;
        
        // Create book manager
        BenchOrderBookManager manager;
        
        // Initialize order books for all symbols
        for (const auto& symbol : m_symbols) {
            manager.createOrderBook(symbol);
        }
        
        // Generate resting orders (all buys to ensure they don't match each other)
        auto restingOrders = generateOrders(m_operationCount);
        for (auto& order : restingOrders) {
            // We can't modify orders directly, so create a new one with the same properties but as BUY
            auto newOrder = std::make_shared<Order>(
                order->getID(), 
                Side::BUY, 
                order->getType(), 
                order->getSymbol(), 
                order->getPrice(), 
                order->getQuantity()
            );
            manager.addOrder(newOrder, nullptr);
        }
        
        // Generate matching orders (all sells to match against the resting buys)
        auto matchingOrders = generateOrders(m_operationCount, false);
        for (size_t i = 0; i < matchingOrders.size(); ++i) {
            // Find a corresponding resting order to match against
            size_t matchIndex = i % restingOrders.size();
            
            // Create a new SELL order with properties that will match the resting BUY
            auto newOrder = std::make_shared<Order>(
                matchingOrders[i]->getID(),
                Side::SELL,
                matchingOrders[i]->getType(),
                restingOrders[matchIndex]->getSymbol(),
                restingOrders[matchIndex]->getPrice(), // Match price to ensure crossing
                matchingOrders[i]->getQuantity()
            );
            
            // Replace the original order
            matchingOrders[i] = newOrder;
        }
        
        // Start memory measurement
        size_t startMemory = getCurrentMemoryUsage();
        
        // Prepare latency data
        std::vector<double> latencies;
        latencies.reserve(m_operationCount);
        
        // Start timing
        auto startTime = std::chrono::high_resolution_clock::now();
        
        if (m_threadCount == 1) {
            // Single-threaded benchmark
            for (size_t i = 0; i < m_operationCount; ++i) {
                auto start = std::chrono::high_resolution_clock::now();
                
                // Count fills
                int fillCount = 0;
                manager.addOrder(matchingOrders[i], [&fillCount](const std::string&, double, double, double) {
                    fillCount++;
                });
                
                auto end = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
                latencies.push_back(static_cast<double>(duration));
            }
        } else {
            // Multi-threaded benchmark
            std::vector<std::thread> threads;
            std::vector<std::vector<double>> threadLatencies(m_threadCount);
            std::atomic<size_t> nextOrderIndex{0};
            
            for (size_t t = 0; t < m_threadCount; ++t) {
                threads.emplace_back([&, t]() {
                    threadLatencies[t].reserve(m_operationCount / m_threadCount);
                    
                    while (true) {
                        size_t i = nextOrderIndex.fetch_add(1, std::memory_order_relaxed);
                        if (i >= m_operationCount) break;
                        
                        auto start = std::chrono::high_resolution_clock::now();
                        
                        // Count fills
                        int fillCount = 0;
                        manager.addOrder(matchingOrders[i], [&fillCount](const std::string&, double, double, double) {
                            fillCount++;
                        });
                        
                        auto end = std::chrono::high_resolution_clock::now();
                        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
                        threadLatencies[t].push_back(static_cast<double>(duration));
                    }
                });
            }
            
            // Wait for all threads to finish
            for (auto& thread : threads) {
                thread.join();
            }
            
            // Combine latencies from all threads
            for (const auto& threadLatency : threadLatencies) {
                latencies.insert(latencies.end(), threadLatency.begin(), threadLatency.end());
            }
        }
        
        // End timing
        auto endTime = std::chrono::high_resolution_clock::now();
        
        // End memory measurement
        size_t endMemory = getCurrentMemoryUsage();
        
        // Calculate results
        result.durationSec = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count() / 1000000.0;
        result.operationsPerSecond = m_operationCount / result.durationSec;
        result.memoryUsageBytes = endMemory > startMemory ? endMemory - startMemory : 0;
        result.latencies = std::move(latencies);
        
        // Calculate latency statistics
        calculateLatencyStatistics(result);
        
        return result;
    }
    
    // Run mixed workload benchmark
    BenchmarkResult benchmarkMixedWorkload() {
        BenchmarkResult result;
        result.name = "Mixed Workload Benchmark";
        result.type = OperationType::MIXED_WORKLOAD;
        result.operationCount = m_operationCount;
        result.threadCount = m_threadCount;
        result.symbolCount = m_symbolCount;
        
        // Create book manager
        BenchOrderBookManager manager;
        
        // Initialize order books for all symbols
        for (const auto& symbol : m_symbols) {
            manager.createOrderBook(symbol);
        }
        
        // Generate test orders
        auto orders = generateOrders(m_operationCount * 2); // 2x orders for different operations
        
        // Add some initial orders to the books
        for (size_t i = 0; i < m_operationCount / 4; ++i) {
            manager.addOrder(orders[i], nullptr);
        }
        
        // Prepare operation distribution
        // 50% new orders, 30% cancellations, 20% modifications
        std::vector<int> operationTypes(m_operationCount);
        size_t addOrderCount = m_operationCount * 0.5;
        size_t cancelOrderCount = m_operationCount * 0.3;
        
        for (size_t i = 0; i < addOrderCount; ++i) {
            operationTypes[i] = 0; // Add order
        }
        for (size_t i = addOrderCount; i < addOrderCount + cancelOrderCount; ++i) {
            operationTypes[i] = 1; // Cancel order
        }
        for (size_t i = addOrderCount + cancelOrderCount; i < m_operationCount; ++i) {
            operationTypes[i] = 2; // Modify order (cancel + add)
        }
        
        // Shuffle operation types
        std::shuffle(operationTypes.begin(), operationTypes.end(), m_rng);
        
        // Start memory measurement
        size_t startMemory = getCurrentMemoryUsage();
        
        // Prepare latency data
        std::vector<double> latencies;
        latencies.reserve(m_operationCount);
        
        // Prepare cancellation candidates
        std::vector<Order::OrderID> cancellationCandidates;
        for (size_t i = 0; i < m_operationCount / 4; ++i) {
            cancellationCandidates.push_back(orders[i]->getID());
        }
        
        // Start timing
        auto startTime = std::chrono::high_resolution_clock::now();
        
        if (m_threadCount == 1) {
            // Single-threaded benchmark
            size_t nextCancelIndex = 0;
            size_t nextAddIndex = m_operationCount / 4;
            
            for (size_t i = 0; i < m_operationCount; ++i) {
                auto start = std::chrono::high_resolution_clock::now();
                
                switch (operationTypes[i]) {
                    case 0: // Add order
                        if (nextAddIndex < orders.size()) {
                            manager.addOrder(orders[nextAddIndex++], nullptr);
                        }
                        break;
                    case 1: // Cancel order
                        if (!cancellationCandidates.empty()) {
                            size_t idx = nextCancelIndex++ % cancellationCandidates.size();
                            manager.cancelOrder(cancellationCandidates[idx]);
                        }
                        break;
                    case 2: // Modify order (cancel + add)
                        if (!cancellationCandidates.empty() && nextAddIndex < orders.size()) {
                            size_t idx = nextCancelIndex++ % cancellationCandidates.size();
                            manager.cancelOrder(cancellationCandidates[idx]);
                            manager.addOrder(orders[nextAddIndex++], nullptr);
                        }
                        break;
                }
                
                auto end = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
                latencies.push_back(static_cast<double>(duration));
            }
        } else {
            // Multi-threaded benchmark
            std::vector<std::thread> threads;
            std::vector<std::vector<double>> threadLatencies(m_threadCount);
            std::atomic<size_t> nextOperationIndex{0};
            std::atomic<size_t> nextCancelIndex{0};
            std::atomic<size_t> nextAddIndex{m_operationCount / 4};
            
            for (size_t t = 0; t < m_threadCount; ++t) {
                threads.emplace_back([&, t]() {
                    threadLatencies[t].reserve(m_operationCount / m_threadCount);
                    
                    while (true) {
                        size_t i = nextOperationIndex.fetch_add(1, std::memory_order_relaxed);
                        if (i >= m_operationCount) break;
                        
                        auto start = std::chrono::high_resolution_clock::now();
                        
                        switch (operationTypes[i]) {
                            case 0: // Add order
                                {
                                    size_t addIdx = nextAddIndex.fetch_add(1, std::memory_order_relaxed);
                                    if (addIdx < orders.size()) {
                                        manager.addOrder(orders[addIdx], nullptr);
                                    }
                                }
                                break;
                            case 1: // Cancel order
                                if (!cancellationCandidates.empty()) {
                                    size_t cancelIdx = nextCancelIndex.fetch_add(1, std::memory_order_relaxed);
                                    size_t idx = cancelIdx % cancellationCandidates.size();
                                    manager.cancelOrder(cancellationCandidates[idx]);
                                }
                                break;
                            case 2: // Modify order (cancel + add)
                                if (!cancellationCandidates.empty()) {
                                    size_t cancelIdx = nextCancelIndex.fetch_add(1, std::memory_order_relaxed);
                                    size_t idx = cancelIdx % cancellationCandidates.size();
                                    manager.cancelOrder(cancellationCandidates[idx]);
                                    
                                    size_t addIdx = nextAddIndex.fetch_add(1, std::memory_order_relaxed);
                                    if (addIdx < orders.size()) {
                                        manager.addOrder(orders[addIdx], nullptr);
                                    }
                                }
                                break;
                        }
                        
                        auto end = std::chrono::high_resolution_clock::now();
                        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
                        threadLatencies[t].push_back(static_cast<double>(duration));
                    }
                });
            }
            
            // Wait for all threads to finish
            for (auto& thread : threads) {
                thread.join();
            }
            
            // Combine latencies from all threads
            for (const auto& threadLatency : threadLatencies) {
                latencies.insert(latencies.end(), threadLatency.begin(), threadLatency.end());
            }
        }
        
        // End timing
        auto endTime = std::chrono::high_resolution_clock::now();
        
        // End memory measurement
        size_t endMemory = getCurrentMemoryUsage();
        
        // Calculate results
        result.durationSec = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count() / 1000000.0;
        result.operationsPerSecond = m_operationCount / result.durationSec;
        result.memoryUsageBytes = endMemory > startMemory ? endMemory - startMemory : 0;
        result.latencies = std::move(latencies);
        
        // Calculate latency statistics
        calculateLatencyStatistics(result);
        
        return result;
    }
    
    // Run a full suite of benchmarks with different configurations
    void runFullBenchmarkSuite() {
        // Prepare CSV file
        std::string csvFilename = "pegasus_benchmark_results.csv";
        std::ofstream csvHeader(csvFilename);
        csvHeader << "Name,OperationType,OperationCount,ThreadCount,SymbolCount,Duration(s),"
                 << "Throughput(ops/s),AvgLatency(ns),P50Latency(ns),P95Latency(ns),"
                 << "P99Latency(ns),MaxLatency(ns),MemoryUsage(bytes)\n";
        csvHeader.close();
        
        // Save original configuration
        size_t origThreadCount = m_threadCount;
        size_t origSymbolCount = m_symbolCount;
        size_t origOperationCount = m_operationCount;
        
        // Test different thread counts with a single symbol
        std::vector<size_t> threadCounts = {1, 2, 4, 8};
        m_symbolCount = 1;
        m_operationCount = 100000;
        
        std::cout << "Running single-symbol benchmarks with different thread counts..." << std::endl;
        
        for (size_t threads : threadCounts) {
            m_threadCount = threads;
            
            std::cout << "\nRunning with " << threads << " thread(s)..." << std::endl;
            
            auto addResult = benchmarkOrderAddition();
            addResult.name = "SingleSymbol_Add_T" + std::to_string(threads);
            addResult.print();
            addResult.saveToCSV(csvFilename);
            
            auto cancelResult = benchmarkOrderCancellation();
            cancelResult.name = "SingleSymbol_Cancel_T" + std::to_string(threads);
            cancelResult.print();
            cancelResult.saveToCSV(csvFilename);
            
            auto matchResult = benchmarkOrderMatching();
            matchResult.name = "SingleSymbol_Match_T" + std::to_string(threads);
            matchResult.print();
            matchResult.saveToCSV(csvFilename);
            
            auto mixedResult = benchmarkMixedWorkload();
            mixedResult.name = "SingleSymbol_Mixed_T" + std::to_string(threads);
            mixedResult.print();
            mixedResult.saveToCSV(csvFilename);
        }
        
        // Test different symbol counts with a fixed thread count
        m_threadCount = 4;
        std::vector<size_t> symbolCounts = {1, 5, 10, 20};
        m_operationCount = 100000;
        
        std::cout << "\nRunning multi-symbol benchmarks with " << m_threadCount << " threads..." << std::endl;
        
        for (size_t symbols : symbolCounts) {
            m_symbolCount = symbols;
            setSymbolCount(symbols);
            
            std::cout << "\nRunning with " << symbols << " symbol(s)..." << std::endl;
            
            auto addResult = benchmarkOrderAddition();
            addResult.name = "MultiSymbol_Add_S" + std::to_string(symbols);
            addResult.print();
            addResult.saveToCSV(csvFilename);
            
            auto cancelResult = benchmarkOrderCancellation();
            cancelResult.name = "MultiSymbol_Cancel_S" + std::to_string(symbols);
            cancelResult.print();
            cancelResult.saveToCSV(csvFilename);
            
            auto matchResult = benchmarkOrderMatching();
            matchResult.name = "MultiSymbol_Match_S" + std::to_string(symbols);
            matchResult.print();
            matchResult.saveToCSV(csvFilename);
            
            auto mixedResult = benchmarkMixedWorkload();
            mixedResult.name = "MultiSymbol_Mixed_S" + std::to_string(symbols);
            mixedResult.print();
            mixedResult.saveToCSV(csvFilename);
        }
        
        // Run one high-volume benchmark
        m_threadCount = 8;
        m_symbolCount = 20;
        setSymbolCount(m_symbolCount);
        m_operationCount = 1000000;
        
        std::cout << "\nRunning high-volume mixed workload benchmark..." << std::endl;
        
        auto highVolumeResult = benchmarkMixedWorkload();
        highVolumeResult.name = "HighVolume_Mixed_T" + std::to_string(m_threadCount) + "_S" + std::to_string(m_symbolCount);
        highVolumeResult.print();
        highVolumeResult.saveToCSV(csvFilename);
        
        // Restore original configuration
        m_threadCount = origThreadCount;
        m_symbolCount = origSymbolCount;
        setSymbolCount(m_symbolCount);
        m_operationCount = origOperationCount;
        
        std::cout << "\nBenchmark suite completed. Results saved to " << csvFilename << std::endl;
    }

private:
    size_t m_symbolCount;                  // Number of symbols to benchmark
    size_t m_threadCount;                  // Number of threads to use
    size_t m_operationCount;               // Number of operations per benchmark
    std::vector<std::string> m_symbols;    // Symbols to use in benchmarks
    
    // Order generation helpers
    std::mt19937 m_rng;
    std::mutex m_benchmarkMutex;
    
    // Calculate latency statistics
    void calculateLatencyStatistics(BenchmarkResult& result) {
        if (result.latencies.empty()) {
            result.averageLatencyNs = 0;
            result.p50LatencyNs = 0;
            result.p95LatencyNs = 0;
            result.p99LatencyNs = 0;
            result.maxLatencyNs = 0;
            return;
        }
        
        // Calculate average
        result.averageLatencyNs = std::accumulate(result.latencies.begin(), result.latencies.end(), 0.0) 
                                / result.latencies.size();
        
        // Calculate percentiles
        std::vector<double> sortedLatencies = result.latencies;
        std::sort(sortedLatencies.begin(), sortedLatencies.end());
        
        result.p50LatencyNs = calculatePercentile(sortedLatencies, 0.5);
        result.p95LatencyNs = calculatePercentile(sortedLatencies, 0.95);
        result.p99LatencyNs = calculatePercentile(sortedLatencies, 0.99);
        result.maxLatencyNs = sortedLatencies.back();
    }
    
    // Calculate percentile from sorted latency vector
    double calculatePercentile(const std::vector<double>& sortedLatencies, double percentile) const {
        if (sortedLatencies.empty()) return 0;
        
        double idx = percentile * (sortedLatencies.size() - 1);
        size_t idxLow = static_cast<size_t>(std::floor(idx));
        size_t idxHigh = static_cast<size_t>(std::ceil(idx));
        
        if (idxLow == idxHigh) {
            return sortedLatencies[idxLow];
        }
        
        double weight = idx - idxLow;
        return (1.0 - weight) * sortedLatencies[idxLow] + weight * sortedLatencies[idxHigh];
    }
    
    // Measure current memory usage
    size_t getCurrentMemoryUsage() const {
        return pegasus::getCurrentMemoryUsage();
    }
};

} // namespace pegasus

#endif // BENCHMARK_H