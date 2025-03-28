#ifndef MEMORY_USAGE_H
#define MEMORY_USAGE_H

#include <cstddef>
#include <iostream>

#if defined(__APPLE__) && defined(__MACH__)
// macOS implementation
#include <mach/mach.h>

namespace pegasus {

inline size_t getCurrentMemoryUsage() {
    struct task_basic_info info;
    mach_msg_type_number_t infoCount = TASK_BASIC_INFO_COUNT;
    
    if (task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&info, &infoCount) != KERN_SUCCESS) {
        return 0;
    }
    
    // Return resident set size in bytes
    return info.resident_size;
}

} // namespace pegasus

#elif defined(__linux__)
// Linux implementation
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sstream>
#include <fstream>

namespace pegasus {

inline size_t getCurrentMemoryUsage() {
    // Read from /proc/self/statm
    std::ifstream statm("/proc/self/statm");
    if (!statm.is_open()) {
        return 0;
    }
    
    long pages;
    statm >> pages; // Skip size
    statm >> pages; // Read resident pages
    
    // Convert pages to bytes (multiply by page size)
    return pages * sysconf(_SC_PAGESIZE);
}

} // namespace pegasus

#elif defined(_WIN32) || defined(_WIN64)
// Windows implementation
#include <windows.h>
#include <psapi.h>

namespace pegasus {

inline size_t getCurrentMemoryUsage() {
    PROCESS_MEMORY_COUNTERS pmc;
    if (!GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return 0;
    }
    
    // Return working set size in bytes
    return pmc.WorkingSetSize;
}

} // namespace pegasus

#else
// Generic fallback
namespace pegasus {

inline size_t getCurrentMemoryUsage() {
    // Not implemented for this platform
    std::cerr << "Memory usage measurement not implemented for this platform." << std::endl;
    return 0;
}

} // namespace pegasus

#endif

#endif // MEMORY_USAGE_H