#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <sstream>
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

// Windowed vector that pre-allocates window size and grows by 2x when full
template<typename T>
class WindowedVector {
private:
    size_t window_size_bytes;           // Size of each window in bytes
    size_t window_size_elements;        // Size of each window in elements
    size_t total_size = 0;              // Total number of elements
    size_t current_window = 0;          // Current window index (0-based)
    size_t current_file_size_bytes = 0; // Current file size
    
    std::string mapped_file;
    boost::interprocess::file_mapping* file_mapping = nullptr;
    boost::interprocess::mapped_region* mapped_region = nullptr;
    
    void initialize_storage() {
        mapped_file = "/tmp/windowed_vector.dat";
        
        // Start with initial window size
        current_file_size_bytes = window_size_bytes;
        create_or_extend_file(mapped_file, current_file_size_bytes);
        
        file_mapping = new boost::interprocess::file_mapping(mapped_file.c_str(), boost::interprocess::read_write);
        mapped_region = new boost::interprocess::mapped_region(*file_mapping, boost::interprocess::read_write);
        
        std::cout << "Initialized storage with window size: " << window_size_bytes / (1024*1024) << " MB" << std::endl;
    }
    
    void flush_and_grow() {
        std::cout << "Window " << current_window << " full. Flushing to disk and growing..." << std::endl;
        
        // Flush current data to disk
        mapped_region->flush();
        
        // Move to next window
        current_window++;
        
        // Calculate new file size (add another window)
        size_t new_file_size = current_file_size_bytes + window_size_bytes;
        
        std::cout << "Growing file from " << current_file_size_bytes / (1024*1024) 
                  << " MB to " << new_file_size / (1024*1024) << " MB" << std::endl;
        
        // Clean up old mapping
        delete mapped_region;
        delete file_mapping;
        
        // Extend file
        create_or_extend_file(mapped_file, new_file_size);
        current_file_size_bytes = new_file_size;
        
        // Create new mapping for entire file
        file_mapping = new boost::interprocess::file_mapping(mapped_file.c_str(), boost::interprocess::read_write);
        mapped_region = new boost::interprocess::mapped_region(*file_mapping, boost::interprocess::read_write);
    }
    
    bool is_window_full() const {
        size_t elements_in_current_window = total_size % window_size_elements;
        return elements_in_current_window == 0 && total_size > 0;
    }
    
public:
    explicit WindowedVector(size_t window_size_mb) 
        : window_size_bytes(window_size_mb * 1024 * 1024)
        , window_size_elements(window_size_bytes / sizeof(T))
    {
        std::cout << "Creating WindowedVector with " << window_size_mb << " MB windows (" 
                  << window_size_elements << " elements per window)" << std::endl;
        initialize_storage();
    }
    
    ~WindowedVector() {
        delete mapped_region;
        delete file_mapping;
        if (!mapped_file.empty()) {
            unlink(mapped_file.c_str());
        }
    }
    
    void push_back(const T& value) {
        // Check if we need to grow (when we've filled a complete window)
        if (is_window_full()) {
            flush_and_grow();
        }
        
        // Calculate position in the mapped region
        size_t global_index = total_size;
        T* base_addr = static_cast<T*>(mapped_region->get_address());
        base_addr[global_index] = value;
        
        total_size++;
    }
    
    T& operator[](size_t index) {
        if (index >= total_size) {
            throw std::out_of_range("Index out of range");
        }
        
        T* base_addr = static_cast<T*>(mapped_region->get_address());
        return base_addr[index];
    }
    
    size_t size() const { return total_size; }
    size_t size_bytes() const { return total_size * sizeof(T); }
    bool empty() const { return total_size == 0; }
    
    void print_storage_info() const {
        std::cout << "WindowedVector status:" << std::endl;
        std::cout << "  Elements: " << total_size << " (" << size_bytes() / (1024*1024) << " MB)" << std::endl;
        std::cout << "  Current window: " << current_window << std::endl;
        std::cout << "  File size: " << current_file_size_bytes / (1024*1024) << " MB" << std::endl;
        std::cout << "  Window utilization: " << (total_size % window_size_elements) 
                  << "/" << window_size_elements << " elements" << std::endl;
    }
    
    void force_flush() {
        if (mapped_region) {
            mapped_region->flush();
            std::cout << "Forced flush to disk completed" << std::endl;
        }
    }
};

int main()
{
    const size_t initial_mem = get_virtual_memory_kb();
    std::cout << "Initial virtual memory: " << initial_mem << " KB" << std::endl;
    MemoryInfo mem_info = get_memory_info();
    std::cout << "System memory - Total: " << mem_info.total_kb / 1024 << " MB, "
              << "Available: " << mem_info.available_kb / 1024 << " MB" << std::endl;

    try
    {
        // Create vector with 500MB windows
        size_t window_size_mb = 500;
        WindowedVector<char> windowed_vector(window_size_mb);
        
        // Target allocation: 3GB (6 windows of 500MB each)
        size_t target_gb = 30;
        size_t target_elements = target_gb * 1024ULL * 1024ULL * 1024ULL;
        
        std::cout << "\nTarget: " << target_gb << " GB (" << target_elements / (1024*1024) << " million elements)" << std::endl;
        std::cout << "This will trigger " << (target_elements / (window_size_mb * 1024 * 1024)) << " window flushes" << std::endl;
        std::cout << "---" << std::endl;
        
        // Allocate in chunks for progress reporting
        const size_t chunk_size = 50 * 1024 * 1024; // 50MB chunks for progress
        char fill_value = 'A';
        
        for (size_t allocated = 0; allocated < target_elements; allocated += chunk_size)
        {
            size_t current_chunk = std::min(chunk_size, target_elements - allocated);
            
            for (size_t i = 0; i < current_chunk; ++i)
            {
                windowed_vector.push_back(fill_value);
            }
            
            // Print progress every 100MB
            if ((allocated + current_chunk) % (100ULL * 1024 * 1024) == 0)
            {
                windowed_vector.print_storage_info();
                std::cout << "Virtual memory: " << get_virtual_memory_kb() / 1024 << " MB" << std::endl;
                std::cout << "---" << std::endl;
            }
            
            fill_value = 'A' + ((allocated / chunk_size) % 26);
        }
        
        std::cout << "\nFinal status:" << std::endl;
        windowed_vector.print_storage_info();
        std::cout << "Final virtual memory: " << get_virtual_memory_kb() / 1024 << " MB" << std::endl;
        
        // Force final flush
        windowed_vector.force_flush();
        
        // Test random access
        std::cout << "\nTesting random access..." << std::endl;
        std::cout << "Element at index 0: " << windowed_vector[0] << std::endl;
        std::cout << "Element at index " << windowed_vector.size()/2 << ": " << windowed_vector[windowed_vector.size()/2] << std::endl;
        std::cout << "Element at index " << windowed_vector.size()-1 << ": " << windowed_vector[windowed_vector.size()-1] << std::endl;
        
        std::cout << "\nsuccess" << std::endl;
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        std::cout << "Final virtual memory: " << get_virtual_memory_kb() / 1024 << " MB" << std::endl;
        return 1;
    }
}