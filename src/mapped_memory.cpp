#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <sstream>
#include <array>
#include <memory>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <sys/stat.h>
#include <unistd.h>

struct MemoryInfo
{
    size_t total_kb = 0;
    size_t free_kb = 0;
    size_t available_kb = 0;
    size_t buffers_kb = 0;
    size_t cached_kb = 0;
};

MemoryInfo get_memory_info()
{
    MemoryInfo info;
    std::ifstream meminfo("/proc/meminfo");
    std::string line;

    while (std::getline(meminfo, line))
    {
        std::istringstream iss(line);
        std::string key;
        size_t value;
        std::string unit;

        if (iss >> key >> value >> unit)
        {
            if (key == "MemTotal:")
                info.total_kb = value;
            else if (key == "MemFree:")
                info.free_kb = value;
            else if (key == "MemAvailable:")
                info.available_kb = value;
            else if (key == "Buffers:")
                info.buffers_kb = value;
            else if (key == "Cached:")
                info.cached_kb = value;
        }
    }
    return info;
}

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

void create_or_extend_file(const std::string& filename, size_t new_size_bytes)
{
    int fd = open(filename.c_str(), O_CREAT | O_RDWR, 0644);
    if (fd == -1) {
        throw std::runtime_error("Failed to create file");
    }
    
    if (ftruncate(fd, new_size_bytes) == -1) {
        close(fd);
        throw std::runtime_error("Failed to resize file");
    }
    close(fd);
}

int main()
{
    const size_t initial_mem = get_virtual_memory_kb();
    std::cout << "Initial virtual memory: " << initial_mem << " KB" << std::endl;
    MemoryInfo mem_info = get_memory_info();
    std::cout << "System memory - Total: " << mem_info.total_kb / 1024 << " MB, "
              << "Free: " << mem_info.free_kb / 1024 << " MB, "
              << "Available: " << mem_info.available_kb / 1024 << " MB" << std::endl;

    // Allocate much more than RAM - let's try 2x RAM size
    uint64_t max_memory_mb = (mem_info.total_kb / 1024) * 2; // 2x total RAM
    std::cout << "Target allocation: " << max_memory_mb << " MB (more than RAM)" << std::endl;

    const std::string filename = "/tmp/memory_mapped.dat";
    std::vector<std::unique_ptr<boost::interprocess::mapped_region>> regions;
    bool trigger_killed_by_out_of_memory = false;
    size_t allocated_mb = 0;

    try
    {
        while (allocated_mb < max_memory_mb + (trigger_killed_by_out_of_memory ? 1 : 0))
        {
            size_t chunk_size_mb = 100;
            size_t new_size_bytes = (allocated_mb + chunk_size_mb) * 1024 * 1024; // Extend by 100MB
            
            // Extend the file
            create_or_extend_file(filename, new_size_bytes);
            
            // Create new mapping for the entire file
            boost::interprocess::file_mapping mapping(filename.c_str(), boost::interprocess::read_write);
            auto region = std::make_unique<boost::interprocess::mapped_region>(
                mapping, boost::interprocess::read_write, 
                allocated_mb * 1024 * 1024, chunk_size_mb * 1024 * 1024); // Map the new 100MB chunk

            // Write data to the mapped region (this forces it to be allocated)
            char* addr = static_cast<char*>(region->get_address());
            char fill_char = 'A' + (allocated_mb % 26);
            std::memset(addr, fill_char, chunk_size_mb * 1024 * 1024);
            
            // Force write to storage
            // region->flush();
            
            regions.push_back(std::move(region));
            allocated_mb+=100;

            // Print memory usage every 100MB allocated
            if (allocated_mb % 100 == 0)
            {
                std::cout << "Allocated " << allocated_mb << " MB, Virtual memory: " 
                          << get_virtual_memory_kb() / 1024 << " MB" << std::endl;
            }
        }
        
        std::cout << "success - Allocated " << allocated_mb << " MB to file-backed memory" << std::endl;
        
        // Cleanup
        regions.clear();
        unlink(filename.c_str());
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        std::cerr << "Memory allocation failed at: " << allocated_mb << " MB." << std::endl;
        std::cout << "Final virtual memory: " << get_virtual_memory_kb() / 1024 << " MB" << std::endl;
        
        // Cleanup
        regions.clear();
        unlink(filename.c_str());
        return 1;
    }
}