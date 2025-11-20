#include <iostream>
#include <vector>
#include <fstream>
#include <string>

size_t get_virtual_memory_kb() {
    std::ifstream status("/proc/self/status");
    std::string line;
    
    while (std::getline(status, line)) {
        if (line.substr(0, 7) == "VmSize:") {
            size_t pos = line.find_first_of("0123456789");
            if (pos != std::string::npos) {
                return std::stoull(line.substr(pos));
            }
        }
    }
    return 0;
}

// ulimit -v max_memory_kb_allowed for max_memory_kb_allowed KB limit when running through terminal
int main()
{
    const size_t initial_mem = get_virtual_memory_kb();
    std::cout << "Initial virtual memory: " << initial_mem << " KB" << std::endl;
    size_t allocated_bytes_nb = 0;
    size_t max_memory_kb_allowed = 100'000;
    size_t max_memory_kb = max_memory_kb_allowed - initial_mem;
    size_t allocation_factor = 33; // because OS allocates more
    size_t max_memory_bytes = max_memory_kb * 1024 / allocation_factor;
    
    
    while (allocated_bytes_nb < max_memory_bytes + 100'000)
    {
        try
        {
            const uint8_t *const new_allocation = new uint8_t;
        }
        catch (const std::bad_alloc &e)
        {
            std::cerr << "Memory allocation failed at: " << allocated_bytes_nb << " bytes. " << allocated_bytes_nb / 1024 << " KB." << std::endl;
            std::cout << "Final virtual memory: " << get_virtual_memory_kb() << " KB" << std::endl;
            return 1;
        }

        allocated_bytes_nb++;
        
        // Print memory usage every 100KB allocated
        if (allocated_bytes_nb % (100 * 1024) == 0) {
            std::cout << "Allocated " << allocated_bytes_nb / 1024 << " KB, Virtual memory: " << get_virtual_memory_kb() << " KB" << std::endl;
        }
    }
    std::cout << "success" << std::endl;
    return 0;
}