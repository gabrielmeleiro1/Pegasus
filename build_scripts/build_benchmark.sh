#!/bin/bash
g++ -std=c++17 -Wall -Werror -Wextra -o pegasus_benchmark src/core/*.cpp src/benchmark/benchmark_runner.cpp -I include/