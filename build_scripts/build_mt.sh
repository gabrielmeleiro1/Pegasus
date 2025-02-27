#!/bin/bash
g++ -std=c++17 -Wall -Werror -Wextra -o pegasus_mt src/core/*.cpp src/examples/main_mt.cpp -I include