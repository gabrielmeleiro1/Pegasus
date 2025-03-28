#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <chrono>
#include <thread>
#include <vector>
#include "benchmark.h"

using namespace pegasus;

void printUsage() {
    std::cout << "Pegasus Order Book Benchmark Runner" << std::endl;
    std::cout << "=================================" << std::endl;
    std::cout << "Usage: benchmark_runner [options]" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --threads=N      Set number of threads (default: 1)" << std::endl;
    std::cout << "  --symbols=N      Set number of symbols (default: 1)" << std::endl;
    std::cout << "  --operations=N   Set number of operations (default: 100000)" << std::endl;
    std::cout << "  --benchmark=TYPE Run specific benchmark type:" << std::endl;
    std::cout << "                   'add' - Order addition" << std::endl;
    std::cout << "                   'cancel' - Order cancellation" << std::endl;
    std::cout << "                   'match' - Order matching" << std::endl;
    std::cout << "                   'mixed' - Mixed workload" << std::endl;
    std::cout << "                   'all' - Run all benchmark types" << std::endl;
    std::cout << "                   'suite' - Run full benchmark suite (ignores other options)" << std::endl;
    std::cout << "  --help           Print this help message" << std::endl;
}

int main(int argc, char* argv[]) {
    // Default settings
    size_t threadCount = 1;
    size_t symbolCount = 1;
    size_t operationCount = 100000;
    std::string benchmarkType = "all";
    
    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--help") {
            printUsage();
            return 0;
        } else if (arg.find("--threads=") == 0) {
            threadCount = std::stoi(arg.substr(10));
        } else if (arg.find("--symbols=") == 0) {
            symbolCount = std::stoi(arg.substr(10));
        } else if (arg.find("--operations=") == 0) {
            operationCount = std::stoi(arg.substr(13));
        } else if (arg.find("--benchmark=") == 0) {
            benchmarkType = arg.substr(12);
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            printUsage();
            return 1;
        }
    }
    
    // Create benchmark instance
    OrderBookBenchmark benchmark;
    benchmark.setThreadCount(threadCount);
    benchmark.setSymbolCount(symbolCount);
    benchmark.setOperationCount(operationCount);
    
    // Print configuration
    std::cout << "Pegasus Order Book Benchmark" << std::endl;
    std::cout << "==========================" << std::endl;
    std::cout << "Configuration:" << std::endl;
    std::cout << "  Threads: " << threadCount << std::endl;
    std::cout << "  Symbols: " << symbolCount << std::endl;
    std::cout << "  Operations: " << operationCount << std::endl;
    std::cout << "  Benchmark Type: " << benchmarkType << std::endl;
    std::cout << std::endl;
    
    // Run the full benchmark suite
    if (benchmarkType == "suite") {
        std::cout << "Running full benchmark suite..." << std::endl;
        benchmark.runFullBenchmarkSuite();
        return 0;
    }
    
    // Run specific benchmark types
    if (benchmarkType == "all" || benchmarkType == "add") {
        std::cout << "Running order addition benchmark..." << std::endl;
        auto result = benchmark.benchmarkOrderAddition();
        result.print();
    }
    
    if (benchmarkType == "all" || benchmarkType == "cancel") {
        std::cout << "Running order cancellation benchmark..." << std::endl;
        auto result = benchmark.benchmarkOrderCancellation();
        result.print();
    }
    
    if (benchmarkType == "all" || benchmarkType == "match") {
        std::cout << "Running order matching benchmark..." << std::endl;
        auto result = benchmark.benchmarkOrderMatching();
        result.print();
    }
    
    if (benchmarkType == "all" || benchmarkType == "mixed") {
        std::cout << "Running mixed workload benchmark..." << std::endl;
        auto result = benchmark.benchmarkMixedWorkload();
        result.print();
    }
    
    return 0;
}