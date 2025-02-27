#!/bin/bash
g++ -std=c++17 -Wall -Werror -Wextra -o pegasus_simple_mt src/core/*.cpp src/threading/simple_mt.cpp -I include