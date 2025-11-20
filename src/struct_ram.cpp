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

// Data structure with 3 vectors to be stored in mapped memory
struct TripleVectorData {
    size_t vector1_size = 0;
    size_t vector2_size = 0;
    size_t vector3_size = 0;
    size_t vector1_capacity = 0;
    size_t vector2_capacity = 0;
    size_t vector3_capacity = 0;
    
    // Data follows immediately after this header
    // Layout: [header][vector1_data][vector2_data][vector3_data]
    
    char* get_vector1_data() {
        return reinterpret_cast<char*>(this + 1);
    }
    
    char* get_vector2_data() {
        return get_vector1_data() + vector1_capacity;
    }
    
    char* get_vector3_data() {
        return get_vector2_data() + vector2_capacity;
    }
    
    size_t total_data_size() const {
        return vector1_capacity + vector2_capacity + vector3_capacity;
    }
    
    size_t total_used_size() const {
        return vector1_size + vector2_size + vector3_size;
    }
};

// Windowed struct storage that manages a struct with 3 vectors in mapped memory
class WindowedStructStorage {
private:
    size_t window_size_bytes;           // Size of each window in bytes
    size_t current_window = 0;          // Current window index (0-based)
    size_t current_file_size_bytes = 0; // Current file size
    size_t vector_capacity_per_window;  // How many elements each vector can hold per window
    
    std::string mapped_file;
    boost::interprocess::file_mapping* file_mapping = nullptr;
    boost::interprocess::mapped_region* mapped_region = nullptr;
    TripleVectorData* data_ptr = nullptr;
    
    void initialize_storage() {
        mapped_file = "/tmp/windowed_struct.dat";
        
        // Calculate capacity per vector (divide window space among 3 vectors + header)
        size_t header_size = sizeof(TripleVectorData);
        size_t available_data_space = window_size_bytes - header_size;
        vector_capacity_per_window = available_data_space / 3; // Divide equally among 3 vectors
        
        // Start with initial window size
        current_file_size_bytes = window_size_bytes;
        create_or_extend_file(mapped_file, current_file_size_bytes);
        
        file_mapping = new boost::interprocess::file_mapping(mapped_file.c_str(), boost::interprocess::read_write);
        mapped_region = new boost::interprocess::mapped_region(*file_mapping, boost::interprocess::read_write);
        
        // Initialize the struct in mapped memory
        data_ptr = static_cast<TripleVectorData*>(mapped_region->get_address());
        new(data_ptr) TripleVectorData(); // Placement new
        
        // Set initial capacities
        data_ptr->vector1_capacity = vector_capacity_per_window;
        data_ptr->vector2_capacity = vector_capacity_per_window;
        data_ptr->vector3_capacity = vector_capacity_per_window;
        
        std::cout << "Initialized struct storage with window size: " << window_size_bytes / (1024*1024) << " MB" << std::endl;
        std::cout << "Each vector capacity per window: " << vector_capacity_per_window / (1024*1024) << " MB" << std::endl;
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
        
        // Update data pointer and expand capacities
        data_ptr = static_cast<TripleVectorData*>(mapped_region->get_address());
        data_ptr->vector1_capacity += vector_capacity_per_window;
        data_ptr->vector2_capacity += vector_capacity_per_window;
        data_ptr->vector3_capacity += vector_capacity_per_window;
    }
    
    bool needs_growth() const {
        return (data_ptr->vector1_size >= data_ptr->vector1_capacity) ||
               (data_ptr->vector2_size >= data_ptr->vector2_capacity) ||
               (data_ptr->vector3_size >= data_ptr->vector3_capacity);
    }
    
public:
    explicit WindowedStructStorage(size_t window_size_mb) 
        : window_size_bytes(window_size_mb * 1024 * 1024)
    {
        std::cout << "Creating WindowedStructStorage with " << window_size_mb << " MB windows" << std::endl;
        initialize_storage();
    }
    
    ~WindowedStructStorage() {
        delete mapped_region;
        delete file_mapping;
        if (!mapped_file.empty()) {
            unlink(mapped_file.c_str());
        }
    }
    
    void push_to_vector1(char value) {
        if (needs_growth()) {
            flush_and_grow();
        }
        
        char* vec1_data = data_ptr->get_vector1_data();
        vec1_data[data_ptr->vector1_size] = value;
        data_ptr->vector1_size++;
    }
    
    void push_to_vector2(char value) {
        if (needs_growth()) {
            flush_and_grow();
        }
        
        char* vec2_data = data_ptr->get_vector2_data();
        vec2_data[data_ptr->vector2_size] = value;
        data_ptr->vector2_size++;
    }
    
    void push_to_vector3(char value) {
        if (needs_growth()) {
            flush_and_grow();
        }
        
        char* vec3_data = data_ptr->get_vector3_data();
        vec3_data[data_ptr->vector3_size] = value;
        data_ptr->vector3_size++;
    }
    
    char get_from_vector1(size_t index) const {
        if (index >= data_ptr->vector1_size) {
            throw std::out_of_range("Vector1 index out of range");
        }
        char* vec1_data = data_ptr->get_vector1_data();
        return vec1_data[index];
    }
    
    char get_from_vector2(size_t index) const {
        if (index >= data_ptr->vector2_size) {
            throw std::out_of_range("Vector2 index out of range");
        }
        char* vec2_data = data_ptr->get_vector2_data();
        return vec2_data[index];
    }
    
    char get_from_vector3(size_t index) const {
        if (index >= data_ptr->vector3_size) {
            throw std::out_of_range("Vector3 index out of range");
        }
        char* vec3_data = data_ptr->get_vector3_data();
        return vec3_data[index];
    }
    
    void print_storage_info() const {
        std::cout << "WindowedStructStorage status:" << std::endl;
        std::cout << "  Vector1: " << data_ptr->vector1_size << "/" << data_ptr->vector1_capacity 
                  << " elements (" << data_ptr->vector1_size / (1024*1024) << " MB)" << std::endl;
        std::cout << "  Vector2: " << data_ptr->vector2_size << "/" << data_ptr->vector2_capacity 
                  << " elements (" << data_ptr->vector2_size / (1024*1024) << " MB)" << std::endl;
        std::cout << "  Vector3: " << data_ptr->vector3_size << "/" << data_ptr->vector3_capacity 
                  << " elements (" << data_ptr->vector3_size / (1024*1024) << " MB)" << std::endl;
        std::cout << "  Total data: " << data_ptr->total_used_size() / (1024*1024) << " MB" << std::endl;
        std::cout << "  Current window: " << current_window << std::endl;
        std::cout << "  File size: " << current_file_size_bytes / (1024*1024) << " MB" << std::endl;
    }
    
    void force_flush() {
        if (mapped_region) {
            mapped_region->flush();
            std::cout << "Forced flush to disk completed" << std::endl;
        }
    }
    
    size_t get_vector1_size() const { return data_ptr->vector1_size; }
    size_t get_vector2_size() const { return data_ptr->vector2_size; }
    size_t get_vector3_size() const { return data_ptr->vector3_size; }
    size_t get_total_size() const { return data_ptr->total_used_size(); }
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
        // Create struct storage with 500MB windows
        size_t window_size_mb = 500;
        WindowedStructStorage struct_storage(window_size_mb);
        
        // Target allocation: 2GB total across all 3 vectors
        size_t target_gb = 2;
        size_t target_elements_per_vector = (target_gb * 1024ULL * 1024ULL * 1024ULL) / 3; // Divide among 3 vectors
        
        std::cout << "\nTarget: " << target_gb << " GB total (" 
                  << target_elements_per_vector / (1024*1024) << " million elements per vector)" << std::endl;
        std::cout << "---" << std::endl;
        
        // Allocate to all 3 vectors simultaneously in chunks
        const size_t chunk_size = 10 * 1024 * 1024; // 10MB chunks per vector
        char fill_value1 = 'A';
        char fill_value2 = 'X';
        char fill_value3 = '0';
        
        for (size_t allocated = 0; allocated < target_elements_per_vector; allocated += chunk_size)
        {
            size_t current_chunk = std::min(chunk_size, target_elements_per_vector - allocated);
            
            // Add data to all three vectors
            for (size_t i = 0; i < current_chunk; ++i)
            {
                struct_storage.push_to_vector1(fill_value1);
                struct_storage.push_to_vector2(fill_value2);
                struct_storage.push_to_vector3(fill_value3);
            }
            
            // Print progress every 100MB total (across all vectors)
            size_t total_allocated = struct_storage.get_total_size();
            if (total_allocated % (100ULL * 1024 * 1024) == 0)
            {
                struct_storage.print_storage_info();
                std::cout << "Virtual memory: " << get_virtual_memory_kb() / 1024 << " MB" << std::endl;
                std::cout << "---" << std::endl;
            }
            
            // Cycle through different fill values
            size_t cycle = (allocated / chunk_size);
            fill_value1 = 'A' + (cycle % 26);                    // A-Z
            fill_value2 = 'X' + (cycle % 3);                     // X, Y, Z  
            fill_value3 = '0' + (cycle % 10);                    // 0-9
        }
        
        std::cout << "\nFinal status:" << std::endl;
        struct_storage.print_storage_info();
        std::cout << "Final virtual memory: " << get_virtual_memory_kb() / 1024 << " MB" << std::endl;
        
        // Force final flush
        struct_storage.force_flush();
        
        // Test random access to all vectors
        std::cout << "\nTesting random access..." << std::endl;
        std::cout << "Vector1[0]: " << struct_storage.get_from_vector1(0) << std::endl;
        std::cout << "Vector2[0]: " << struct_storage.get_from_vector2(0) << std::endl;
        std::cout << "Vector3[0]: " << struct_storage.get_from_vector3(0) << std::endl;
        
        size_t mid_index = struct_storage.get_vector1_size() / 2;
        std::cout << "Vector1[" << mid_index << "]: " << struct_storage.get_from_vector1(mid_index) << std::endl;
        std::cout << "Vector2[" << mid_index << "]: " << struct_storage.get_from_vector2(mid_index) << std::endl;
        std::cout << "Vector3[" << mid_index << "]: " << struct_storage.get_from_vector3(mid_index) << std::endl;
        
        size_t last_index = struct_storage.get_vector1_size() - 1;
        std::cout << "Vector1[" << last_index << "]: " << struct_storage.get_from_vector1(last_index) << std::endl;
        std::cout << "Vector2[" << last_index << "]: " << struct_storage.get_from_vector2(last_index) << std::endl;
        std::cout << "Vector3[" << last_index << "]: " << struct_storage.get_from_vector3(last_index) << std::endl;
        
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