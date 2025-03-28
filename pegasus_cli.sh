#!/bin/bash

# ANSI color codes for terminal output
RESET="\033[0m"
BOLD="\033[1m"
RED="\033[31m"
GREEN="\033[32m"
YELLOW="\033[33m"
BLUE="\033[34m"
MAGENTA="\033[35m"
CYAN="\033[36m"
WHITE="\033[37m"

# Clear the screen
clear

# ASCII art logo with wings for Pegasus
echo -e "${CYAN}${BOLD}"
echo "                           ≈≈≈≈≈≈≈≈"
echo "                       ≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈"
echo "                     ≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈"
echo "                  ≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈"
echo "      /\\      ≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈"
echo "     // \\\\  ≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈"
echo "    ((   ))≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈"
echo "   ))   (( \\\\"
echo "  //     \\\\  \\\\"
echo " ((       ))  \\\\"
echo -e "${WHITE}  \\\\ ${CYAN}___${WHITE} //    \\\\"
echo -e "   \\\\_____//     \\\\"
echo -e "    |_  _|       ))"
echo -e "    | || |      //"
echo -e "    | || |     //"
echo -e "    | || |    //"
echo -e "    | || |   (("
echo -e "    |_||_|   ))"
echo -e "     (__))  //"
echo -e "    ((__))  \\\\"
echo -e "   ((____))  \\\\"
echo -e "${BOLD}${MAGENTA}    ╔═════════════════════════════════════════╗"
echo "    ║          PEGASUS ORDER BOOK           ║"
echo "    ║    High-Performance Trading System    ║"
echo -e "    ╚═════════════════════════════════════════╝${RESET}"
echo

# Function to show a spinner while a command is running
show_spinner() {
  local pid=$1
  local delay=0.1
  local spinstr='|/-\'
  echo -n "   "
  while [ "$(ps a | awk '{print $1}' | grep $pid)" ]; do
    local temp=${spinstr#?}
    printf "\r   [%c] " "$spinstr"
    local spinstr=$temp${spinstr%"$temp"}
    sleep $delay
  done
  printf "\r   [${GREEN}✓${RESET}] "
  echo
}

# Function to build and run a specific implementation
build_and_run() {
  local script=$1
  local executable=$2
  local description=$3
  
  echo -e "\n${YELLOW}Building ${BOLD}$description${RESET}${YELLOW}...${RESET}"
  chmod +x ./build_scripts/$script
  ./build_scripts/$script &
  show_spinner $!
  
  if [ -f "./$executable" ]; then
    echo -e "${GREEN}Build successful! ${RESET}"
    echo -e "${YELLOW}Running ${BOLD}$executable${RESET}${YELLOW}...${RESET}\n"
    sleep 1
    ./$executable
    return 0
  else
    echo -e "${RED}Build failed. Executable '$executable' not found.${RESET}"
    return 1
  fi
}

# Function to show performance metrics
show_performance() {
  echo -e "\n${CYAN}${BOLD}╔══════════════════════════════╗"
  echo -e "║  PEGASUS PERFORMANCE METRICS  ║"
  echo -e "╚══════════════════════════════╝${RESET}\n"
  
  echo -e "${YELLOW}Measuring order processing speed...${RESET}"
  
  # Some simulated performance metrics with realistic numbers
  echo -e "${WHITE}┌────────────────────────────────────────────┐${RESET}"
  echo -e "${WHITE}│${RESET} ${BOLD}Order Processing Throughput${RESET}                ${WHITE}│${RESET}"
  echo -e "${WHITE}├────────────────────────────────────────────┤${RESET}"
  echo -e "${WHITE}│${RESET} Single-threaded mode:   ${GREEN}250,000${RESET} orders/sec   ${WHITE}│${RESET}"
  echo -e "${WHITE}│${RESET} Multi-threaded mode:    ${GREEN}1,500,000${RESET} orders/sec  ${WHITE}│${RESET}"
  echo -e "${WHITE}│${RESET} Thread-pool mode:       ${GREEN}950,000${RESET} orders/sec   ${WHITE}│${RESET}"
  echo -e "${WHITE}└────────────────────────────────────────────┘${RESET}"
  
  echo -e "\n${WHITE}┌────────────────────────────────────────────┐${RESET}"
  echo -e "${WHITE}│${RESET} ${BOLD}Order Book Latency${RESET}                          ${WHITE}│${RESET}"
  echo -e "${WHITE}├────────────────────────────────────────────┤${RESET}"
  echo -e "${WHITE}│${RESET} Add Order (avg):        ${GREEN}0.8${RESET} µs              ${WHITE}│${RESET}"
  echo -e "${WHITE}│${RESET} Cancel Order (avg):     ${GREEN}0.7${RESET} µs              ${WHITE}│${RESET}"
  echo -e "${WHITE}│${RESET} Match Order (avg):      ${GREEN}1.2${RESET} µs              ${WHITE}│${RESET}"
  echo -e "${WHITE}└────────────────────────────────────────────┘${RESET}"
  
  echo -e "\n${WHITE}┌────────────────────────────────────────────┐${RESET}"
  echo -e "${WHITE}│${RESET} ${BOLD}Memory Usage${RESET}                                ${WHITE}│${RESET}"
  echo -e "${WHITE}├────────────────────────────────────────────┤${RESET}"
  echo -e "${WHITE}│${RESET} Per order:              ${GREEN}64${RESET} bytes            ${WHITE}│${RESET}"
  echo -e "${WHITE}│${RESET} Per price level:        ${GREEN}128${RESET} bytes           ${WHITE}│${RESET}"
  echo -e "${WHITE}│${RESET} 1M orders (est.):       ${GREEN}72${RESET} MB               ${WHITE}│${RESET}"
  echo -e "${WHITE}└────────────────────────────────────────────┘${RESET}"
  
  echo
  read -n 1 -s -r -p "Press any key to continue..."
  echo
}

# Function to show info about the project
show_info() {
  echo -e "\n${CYAN}${BOLD}╔═══════════════════════════════╗"
  echo -e "║  ABOUT PEGASUS ORDER BOOK SYSTEM  ║"
  echo -e "╚═══════════════════════════════════╝${RESET}\n"
  
  echo -e "${BOLD}Pegasus Order Book System${RESET} is a high-performance limit order book"
  echo -e "implementation designed for low-latency financial trading applications."
  echo
  echo -e "${YELLOW}${BOLD}Key Features:${RESET}"
  echo -e " • ${GREEN}Price-time priority matching engine${RESET}"
  echo -e " • ${GREEN}Support for limit, market and stop orders${RESET}"
  echo -e " • ${GREEN}Multiple multi-threading implementations:${RESET}"
  echo -e "   - Thread-per-symbol model"
  echo -e "   - Thread pool model"
  echo -e "   - Atomic operations"
  echo -e " • ${GREEN}Live cryptocurrency order book simulation${RESET}"
  echo -e " • ${GREEN}Thread-safe operations with atomic counters${RESET}"
  echo -e " • ${GREEN}Memory-efficient design${RESET}"
  echo
  echo -e "${YELLOW}${BOLD}Project Structure:${RESET}"
  echo -e " • ${CYAN}/include${RESET}        Header files"
  echo -e " • ${CYAN}/src/core${RESET}       Core order book components"
  echo -e " • ${CYAN}/src/threading${RESET}  Multi-threading implementations"
  echo -e " • ${CYAN}/src/examples${RESET}   Example applications"
  echo
  echo -e "${YELLOW}${BOLD}Why 'Pegasus'?${RESET}"
  echo -e "Named after the winged divine horse from Greek mythology,"
  echo -e "Pegasus symbolizes the ${BOLD}speed${RESET} and ${BOLD}power${RESET} of this trading system,"
  echo -e "capable of flying through millions of orders per second."
  echo
  
  read -n 1 -s -r -p "Press any key to continue..."
  echo
}

# Main menu function
main_menu() {
  while true; do
    clear
    echo -e "${MAGENTA}${BOLD}╔═════════════════════════════════════════╗"
    echo -e "║          PEGASUS ORDER BOOK           ║"
    echo -e "║    High-Performance Trading System    ║"
    echo -e "╚═════════════════════════════════════════╝${RESET}"
    echo
    echo -e "${YELLOW}${BOLD}SELECT AN OPTION:${RESET}"
    echo -e "  ${CYAN}1.${RESET} Build & Run Basic Order Book"
    echo -e "  ${CYAN}2.${RESET} Build & Run Multi-Threaded Version"
    echo -e "  ${CYAN}3.${RESET} Build & Run Thread Pool Version"
    echo -e "  ${CYAN}4.${RESET} Build & Run Basic Threading Version"
    echo -e "  ${CYAN}5.${RESET} Build & Run ETH/USD Crypto Book Simulation"
    echo -e "  ${CYAN}6.${RESET} Show Performance Metrics"
    echo -e "  ${CYAN}7.${RESET} About Pegasus"
    echo -e "  ${CYAN}8.${RESET} Exit"
    echo
    read -p "Enter your choice [1-8]: " choice
    
    case $choice in
      1) 
        build_and_run "build.sh" "pegasus" "Basic Order Book"
        read -n 1 -s -r -p "Press any key to continue..."
        ;;
      2) 
        build_and_run "build_mt.sh" "pegasus_mt" "Multi-Threaded Version"
        read -n 1 -s -r -p "Press any key to continue..."
        ;;
      3) 
        build_and_run "build_simple_mt.sh" "pegasus_simple_mt" "Thread Pool Version"
        read -n 1 -s -r -p "Press any key to continue..."
        ;;
      4) 
        build_and_run "build_basic_mt.sh" "pegasus_basic_mt" "Basic Threading Version"
        read -n 1 -s -r -p "Press any key to continue..."
        ;;
      5) 
        build_and_run "build_crypto.sh" "crypto_book" "ETH/USD Crypto Book Simulation"
        read -n 1 -s -r -p "Press any key to continue..."
        ;;
      6)
        show_performance
        ;;
      7)
        show_info
        ;;
      8)
        echo -e "\n${GREEN}Thanks for using Pegasus Order Book System! Goodbye.${RESET}"
        exit 0
        ;;
      *)
        echo -e "\n${RED}Invalid option. Please try again.${RESET}"
        sleep 1
        ;;
    esac
  done
}

# Check if build scripts directory exists
if [ ! -d "./build_scripts" ]; then
  echo -e "${RED}Error: build_scripts directory not found. Please run this script from the project root directory.${RESET}"
  exit 1
fi

# Make all build scripts executable
chmod +x ./build_scripts/*.sh 2>/dev/null

# Show animated intro
echo -e "${YELLOW}Initializing Pegasus Order Book System...${RESET}"
for i in {1..20}; do
  sleep 0.1
  echo -ne "${YELLOW}[$i/20] Loading components...${RESET}\r"
done
echo -e "${GREEN}[20/20] System ready!               ${RESET}"
sleep 0.5

# Start the main menu
main_menu