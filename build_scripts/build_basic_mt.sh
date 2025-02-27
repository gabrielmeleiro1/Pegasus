#!/bin/bash
g++ -std=c++17 -Wall -Werror -Wextra -o pegasus_basic_mt src/core/*.cpp src/threading/basic_mt.cpp -I include