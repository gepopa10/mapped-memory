#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/anonymous_shared_memory.hpp>

size_t get_virtual_memory_kb()
{
    std::ifstream status("/proc/self/status");
    std::string line;

    while (std::getline(status, line))
    {
        if (line.substr(0, 7) == "VmSize:")
        {
            size_t pos = line.find_first_of("0123456789");
            if (pos != std::string::npos)
            {
                return std::stoull(line.substr(pos));
            }
        }
    }
    return 0;
}

int main()
{
    const size_t initial_mem = get_virtual_memory_kb();
    std::cout << "Initial virtual memory: " << initial_mem << " KB" << std::endl;
    uint64_t max_memory_kbytes = 8 * 1024 * 1024; // 8 GB
    std::vector<char> vec;
    bool trigger_killed_by_out_of_memory = false;


    while (vec.size() / 1024 < max_memory_kbytes + trigger_killed_by_out_of_memory? 1: 0)
    {
        try
        {
            std::array<char, 1 * 1024 * 1024> alloc_1MB{};
            vec.insert(vec.end(), alloc_1MB.begin(), alloc_1MB.end());
        }
        catch (const std::bad_alloc &e)
        {
            std::cerr << "size: " << vec.size() << std::endl;
            std::cerr << "Memory allocation failed at: " << vec.size() / 1024 / 1024 << " MB." << std::endl;
            std::cout << "Final virtual memory: " << get_virtual_memory_kb() / 1024 << " MB" << std::endl;
            return 1;
        }

        // Print memory usage every 100KB allocated
        if (vec.size() % (100 * 1024) == 0)
        {
            std::cout << "Allocated " << vec.size() / 1024 / 1024 << " MB, Virtual memory: " << get_virtual_memory_kb() / 1024 << " MB" << std::endl;
        }
    }
    std::cout << "success" << std::endl;
    return 0;
}