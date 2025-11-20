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

// Custom hybrid vector that switches to memory-mapped storage after threshold
template<typename T>
class HybridVector {
private:
    static constexpr size_t THRESHOLD_BYTES = 4ULL * 1024 * 1024 * 1024; // 4GB
    static constexpr size_t CHUNK_SIZE_BYTES = 4ULL * 1024 * 1024 * 1024; // 4GB per chunk
    
    std::vector<T> ram_storage;  // For data under 4GB
    std::string mapped_file;     // File for mapped storage
    std::vector<std::unique_ptr<boost::interprocess::mapped_region>> mapped_regions;
    boost::interprocess::file_mapping* file_mapping = nullptr;
    size_t total_size = 0;
    
    bool using_mapped_storage() const {
        return total_size * sizeof(T) >= THRESHOLD_BYTES;
    }
    
    void switch_to_mapped_storage() {
        if (file_mapping) return; // Already using mapped storage
        
        std::cout << "Switching to memory-mapped storage at " << (total_size * sizeof(T)) / (1024*1024) << " MB" << std::endl;
        
        // Create the initial mapped file
        mapped_file = "/tmp/hybrid_vector.dat";
        size_t initial_size = CHUNK_SIZE_BYTES;
        create_or_extend_file(mapped_file, initial_size);
        
        file_mapping = new boost::interprocess::file_mapping(mapped_file.c_str(), boost::interprocess::read_write);
        
        // Create first mapped region
        auto region = std::make_unique<boost::interprocess::mapped_region>(
            *file_mapping, boost::interprocess::read_write, 0, CHUNK_SIZE_BYTES);
        
        // Copy existing RAM data to mapped storage
        T* mapped_addr = static_cast<T*>(region->get_address());
        std::copy(ram_storage.begin(), ram_storage.end(), mapped_addr);
        
        mapped_regions.push_back(std::move(region));
        
        // Clear RAM storage to free memory
        ram_storage.clear();
        ram_storage.shrink_to_fit();
    }
    
    void extend_mapped_storage() {
        size_t current_chunks = mapped_regions.size();
        size_t new_file_size = (current_chunks + 1) * CHUNK_SIZE_BYTES;
        
        std::cout << "Extending mapped storage to " << new_file_size / (1024*1024*1024) << " GB" << std::endl;
        
        // Extend the file
        create_or_extend_file(mapped_file, new_file_size);
        
        // Create new file mapping (old one becomes invalid after resize)
        delete file_mapping;
        file_mapping = new boost::interprocess::file_mapping(mapped_file.c_str(), boost::interprocess::read_write);
        
        // Recreate all existing regions with new mapping
        for (size_t i = 0; i < mapped_regions.size(); ++i) {
            mapped_regions[i] = std::make_unique<boost::interprocess::mapped_region>(
                *file_mapping, boost::interprocess::read_write, 
                i * CHUNK_SIZE_BYTES, CHUNK_SIZE_BYTES);
        }
        
        // Add new chunk
        auto new_region = std::make_unique<boost::interprocess::mapped_region>(
            *file_mapping, boost::interprocess::read_write, 
            current_chunks * CHUNK_SIZE_BYTES, CHUNK_SIZE_BYTES);
        
        mapped_regions.push_back(std::move(new_region));
    }
    
public:
    HybridVector() = default;
    
    ~HybridVector() {
        mapped_regions.clear();
        delete file_mapping;
        if (!mapped_file.empty()) {
            unlink(mapped_file.c_str());
        }
    }
    
    void push_back(const T& value) {
        if (!using_mapped_storage()) {
            // Use regular vector for small sizes
            ram_storage.push_back(value);
            total_size++;
            
            // Check if we need to switch to mapped storage
            if (using_mapped_storage()) {
                switch_to_mapped_storage();
            }
        } else {
            // Using mapped storage
            size_t elements_per_chunk = CHUNK_SIZE_BYTES / sizeof(T);
            size_t chunk_index = total_size / elements_per_chunk;
            size_t offset_in_chunk = total_size % elements_per_chunk;
            
            // Check if we need a new chunk
            if (chunk_index >= mapped_regions.size()) {
                extend_mapped_storage();
            }
            
            // Write to mapped region
            T* chunk_addr = static_cast<T*>(mapped_regions[chunk_index]->get_address());
            chunk_addr[offset_in_chunk] = value;
            
            total_size++;
        }
    }
    
    T& operator[](size_t index) {
        if (index >= total_size) {
            throw std::out_of_range("Index out of range");
        }
        
        if (!using_mapped_storage()) {
            return ram_storage[index];
        } else {
            size_t elements_per_chunk = CHUNK_SIZE_BYTES / sizeof(T);
            size_t chunk_index = index / elements_per_chunk;
            size_t offset_in_chunk = index % elements_per_chunk;
            
            T* chunk_addr = static_cast<T*>(mapped_regions[chunk_index]->get_address());
            return chunk_addr[offset_in_chunk];
        }
    }
    
    size_t size() const { return total_size; }
    size_t size_bytes() const { return total_size * sizeof(T); }
    bool empty() const { return total_size == 0; }
    
    void print_storage_info() const {
        std::cout << "Vector size: " << total_size << " elements (" 
                  << size_bytes() / (1024*1024) << " MB)" << std::endl;
        std::cout << "Using " << (using_mapped_storage() ? "mapped" : "RAM") << " storage" << std::endl;
        if (using_mapped_storage()) {
            std::cout << "Mapped chunks: " << mapped_regions.size() << std::endl;
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
        HybridVector<char> big_vector;
        
        // Test allocation beyond RAM
        size_t target_gb = (mem_info.total_kb / 1024 / 1024) + 2; // RAM + 2GB
        size_t target_elements = target_gb * 1024ULL * 1024ULL * 1024ULL;
        
        std::cout << "Target: " << target_gb << " GB (" << target_elements / (1024*1024) << " million elements)" << std::endl;
        
        // Allocate in chunks to avoid slow single-element pushes
        const size_t chunk_size = 100 * 1024 * 1024; // 100MB chunks
        char fill_value = 'A';
        
        for (size_t allocated = 0; allocated < target_elements; allocated += chunk_size)
        {
            size_t current_chunk = std::min(chunk_size, target_elements - allocated);
            
            for (size_t i = 0; i < current_chunk; ++i)
            {
                big_vector.push_back(fill_value);
            }
            
            // Print progress every 1GB
            if ((allocated + current_chunk) % (1024ULL * 1024 * 1024) == 0)
            {
                big_vector.print_storage_info();
                std::cout << "Virtual memory: " << get_virtual_memory_kb() / 1024 << " MB" << std::endl;
                std::cout << "---" << std::endl;
            }
            
            fill_value = 'A' + ((allocated / chunk_size) % 26);
        }
        
        std::cout << "\nFinal status:" << std::endl;
        big_vector.print_storage_info();
        std::cout << "Final virtual memory: " << get_virtual_memory_kb() / 1024 << " MB" << std::endl;
        
        // Test random access
        std::cout << "Testing random access..." << std::endl;
        std::cout << "Element at index 0: " << big_vector[0] << std::endl;
        std::cout << "Element at index " << big_vector.size()/2 << ": " << big_vector[big_vector.size()/2] << std::endl;
        std::cout << "Element at index " << big_vector.size()-1 << ": " << big_vector[big_vector.size()-1] << std::endl;
        
        std::cout << "success" << std::endl;
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        std::cout << "Final virtual memory: " << get_virtual_memory_kb() / 1024 << " MB" << std::endl;
        return 1;
    }
}