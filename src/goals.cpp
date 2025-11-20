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

// Individual goal data - just tracks size, actual data is stored separately
struct GoalData {
    size_t data_size = 0;        // Current number of elements in this goal
    
    size_t size() const { return data_size; }
};

// Main structure with column-wise data layout
struct GoalsData {
    size_t num_goals = 0;              // Number of goals (fixed at construction)
    size_t max_elements_per_goal = 0;  // Current capacity per goal (grows as needed)
    size_t window_size_elements = 0;   // Elements that fit in one window
    size_t current_window = 0;         // Current window number
    
    // Get pointer to goal metadata at index
    GoalData* get_goal(size_t goal_index) {
        if (goal_index >= num_goals) {
            throw std::out_of_range("Goal index out of range");
        }
        // Goals array follows immediately after this struct
        GoalData* goals_array = reinterpret_cast<GoalData*>(this + 1);
        return &goals_array[goal_index];
    }
    
    const GoalData* get_goal(size_t goal_index) const {
        if (goal_index >= num_goals) {
            throw std::out_of_range("Goal index out of range");
        }
        const GoalData* goals_array = reinterpret_cast<const GoalData*>(this + 1);
        return &goals_array[goal_index];
    }
    
    // Get pointer to data area (column-wise layout)
    char* get_data_area() {
        GoalData* goals_array = reinterpret_cast<GoalData*>(this + 1);
        return reinterpret_cast<char*>(goals_array + num_goals);
    }
    
    const char* get_data_area() const {
        const GoalData* goals_array = reinterpret_cast<const GoalData*>(this + 1);
        return reinterpret_cast<const char*>(goals_array + num_goals);
    }
    
    // Get element at specific goal and index (column-wise access)
    char get_element(size_t goal_index, size_t element_index) const {
        if (goal_index >= num_goals) {
            throw std::out_of_range("Goal index out of range");
        }
        const GoalData* goal = get_goal(goal_index);
        if (element_index >= goal->data_size) {
            throw std::out_of_range("Element index out of range");
        }
        
        // Column-wise layout: [all goals at index 0][all goals at index 1]...
        const char* data_area = get_data_area();
        size_t offset = (element_index * num_goals) + goal_index;
        return data_area[offset];
    }
    
    // Set element at specific goal and index
    void set_element(size_t goal_index, size_t element_index, char value) {
        if (goal_index >= num_goals) {
            throw std::out_of_range("Goal index out of range");
        }
        if (element_index >= max_elements_per_goal) {
            throw std::out_of_range("Element index exceeds capacity");
        }
        
        // Column-wise layout: [all goals at index 0][all goals at index 1]...
        char* data_area = get_data_area();
        size_t offset = (element_index * num_goals) + goal_index;
        data_area[offset] = value;
    }
    
    // Push back to a specific goal
    void push_to_goal(size_t goal_index, char value) {
        GoalData* goal = get_goal(goal_index);
        if (goal->data_size >= max_elements_per_goal) {
            throw std::runtime_error("Goal capacity exceeded - need to grow");
        }
        
        set_element(goal_index, goal->data_size, value);
        goal->data_size++;
    }
    
    size_t get_total_capacity_bytes() const {
        return max_elements_per_goal * num_goals * sizeof(char);
    }
    
    size_t get_header_size() const {
        return sizeof(GoalsData) + (num_goals * sizeof(GoalData));
    }
    
    size_t get_total_memory_footprint() const {
        return get_header_size() + get_total_capacity_bytes();
    }
    
    void print_layout_info() const {
        std::cout << "Column-wise data layout:" << std::endl;
        std::cout << "  Goals: " << num_goals << std::endl;
        std::cout << "  Max elements per goal: " << max_elements_per_goal << std::endl;
        std::cout << "  Data area size: " << get_total_capacity_bytes() / (1024*1024) << " MB" << std::endl;
        std::cout << "  Header size: " << get_header_size() / 1024 << " KB" << std::endl;
        std::cout << "  Memory layout:" << std::endl;
        std::cout << "    [Header] -> [Goal metadata array (" << num_goals << " goals)] -> [Index 0: all " << num_goals << " goals] -> [Index 1: all " << num_goals << " goals] -> ..." << std::endl;
        
        // Show goal size distribution
        size_t min_size = SIZE_MAX, max_size = 0, total_elements = 0;
        for (size_t i = 0; i < num_goals; ++i) {
            const GoalData* goal = get_goal(i);
            size_t size = goal->size();
            min_size = std::min(min_size, size);
            max_size = std::max(max_size, size);
            total_elements += size;
        }
        
        if (num_goals > 0) {
            std::cout << "  Goal sizes - Min: " << min_size << ", Max: " << max_size 
                      << ", Avg: " << total_elements / num_goals << ", Total: " << total_elements << std::endl;
        }
        
        // Show only first and last few goals for large numbers
        size_t goals_to_show = std::min(num_goals, size_t(3));
        for (size_t i = 0; i < goals_to_show; ++i) {
            const GoalData* goal = get_goal(i);
            std::cout << "    Goal[" << i << "]: size=" << goal->size() << std::endl;
        }
        if (num_goals > 6) {
            std::cout << "    ... (skipping goals 3 to " << (num_goals - 4) << ") ..." << std::endl;
            for (size_t i = num_goals - 3; i < num_goals; ++i) {
                const GoalData* goal = get_goal(i);
                std::cout << "    Goal[" << i << "]: size=" << goal->size() << std::endl;
            }
        } else if (num_goals > 3) {
            for (size_t i = goals_to_show; i < num_goals; ++i) {
                const GoalData* goal = get_goal(i);
                std::cout << "    Goal[" << i << "]: size=" << goal->size() << std::endl;
            }
        }
    }
    
    // Bulk operations for efficiency with many goals
    void push_to_all_goals(char value) {
        // Check if we have capacity for all goals
        for (size_t i = 0; i < num_goals; ++i) {
            GoalData* goal = get_goal(i);
            if (goal->data_size >= max_elements_per_goal) {
                throw std::runtime_error("Goal capacity exceeded - need to grow");
            }
        }
        
        // Add to all goals at once (more efficient for large goal counts)
        for (size_t i = 0; i < num_goals; ++i) {
            GoalData* goal = get_goal(i);
            set_element(i, goal->data_size, value + (i % 26)); // Vary by goal
            goal->data_size++;
        }
    }
    
    void push_to_goal_range(size_t start_goal, size_t end_goal, char base_value) {
        if (end_goal > num_goals) end_goal = num_goals;
        
        for (size_t i = start_goal; i < end_goal; ++i) {
            GoalData* goal = get_goal(i);
            if (goal->data_size >= max_elements_per_goal) {
                throw std::runtime_error("Goal capacity exceeded - need to grow");
            }
            set_element(i, goal->data_size, base_value + (i % 26));
            goal->data_size++;
        }
    }
};

// Manager class for memory-mapped GoalsData with windowed growth
class GoalsDataStorage {
private:
    std::string mapped_file;
    boost::interprocess::file_mapping* file_mapping = nullptr;
    boost::interprocess::mapped_region* mapped_region = nullptr;
    GoalsData* goals_data = nullptr;
    size_t current_file_size = 0;
    size_t window_size_bytes = 0;
    
    void grow_storage() {
        std::cout << "\n=== GROWING STORAGE ===" << std::endl;
        std::cout << "Current capacity: " << goals_data->max_elements_per_goal << " elements per goal" << std::endl;
        
        // Flush current data
        mapped_region->flush();
        
        // Calculate new capacity (add one more window worth of elements)
        size_t new_elements_per_goal = goals_data->max_elements_per_goal + goals_data->window_size_elements;
        size_t new_data_size = new_elements_per_goal * goals_data->num_goals * sizeof(char);
        size_t new_file_size = goals_data->get_header_size() + new_data_size;
        
        std::cout << "Growing to " << new_elements_per_goal << " elements per goal" << std::endl;
        std::cout << "New data size: " << new_data_size / (1024*1024) << " MB" << std::endl;
        std::cout << "Growing file from " << current_file_size / (1024*1024) 
                  << " MB to " << new_file_size / (1024*1024) << " MB" << std::endl;
        
        // Clean up old mapping
        delete mapped_region;
        delete file_mapping;
        
        // Extend file
        create_or_extend_file(mapped_file, new_file_size);
        current_file_size = new_file_size;
        
        // Create new mapping
        file_mapping = new boost::interprocess::file_mapping(mapped_file.c_str(), boost::interprocess::read_write);
        mapped_region = new boost::interprocess::mapped_region(*file_mapping, boost::interprocess::read_write);
        
        // Update pointer and capacity
        goals_data = static_cast<GoalsData*>(mapped_region->get_address());
        goals_data->max_elements_per_goal = new_elements_per_goal;
        goals_data->current_window++;
        
        std::cout << "New capacity: " << goals_data->max_elements_per_goal << " elements per goal" << std::endl;
        std::cout << "New file size: " << current_file_size / (1024*1024) << " MB" << std::endl;
        std::cout << "Current window: " << goals_data->current_window << std::endl;
        std::cout << "=== GROWTH COMPLETE ===" << std::endl;
    }

public:
    explicit GoalsDataStorage(size_t num_goals, size_t window_size_mb = 100) {
        mapped_file = "/tmp/goals_data_columnar_8k.dat";
        window_size_bytes = window_size_mb * 1024 * 1024;
        
        std::cout << "Creating column-wise GoalsDataStorage for LARGE scale:" << std::endl;
        std::cout << "  Number of goals: " << num_goals << std::endl;
        std::cout << "  Window size: " << window_size_mb << " MB" << std::endl;
        
        // Calculate initial capacity
        size_t header_size = sizeof(GoalsData) + (num_goals * sizeof(GoalData));
        std::cout << "  Header size: " << header_size / 1024 << " KB (" << header_size << " bytes)" << std::endl;
        
        if (header_size >= window_size_bytes) {
            throw std::runtime_error("Window size too small for " + std::to_string(num_goals) + " goals. Need at least " + std::to_string((header_size / (1024*1024)) + 1) + " MB");
        }
        
        size_t available_for_data = window_size_bytes - header_size;
        size_t initial_elements_per_goal = available_for_data / (num_goals * sizeof(char));
        
        std::cout << "  Available for data: " << available_for_data / (1024*1024) << " MB" << std::endl;
        std::cout << "  Initial elements per goal: " << initial_elements_per_goal << std::endl;
        std::cout << "  Total initial capacity: " << (initial_elements_per_goal * num_goals) / (1024*1024) << " MB" << std::endl;
        
        // Create initial file
        current_file_size = window_size_bytes;
        create_or_extend_file(mapped_file, current_file_size);
        
        // Create mapping
        file_mapping = new boost::interprocess::file_mapping(mapped_file.c_str(), boost::interprocess::read_write);
        mapped_region = new boost::interprocess::mapped_region(*file_mapping, boost::interprocess::read_write);
        
        // Initialize GoalsData
        goals_data = static_cast<GoalsData*>(mapped_region->get_address());
        new(goals_data) GoalsData(); // Placement new
        
        goals_data->num_goals = num_goals;
        goals_data->max_elements_per_goal = initial_elements_per_goal;
        goals_data->window_size_elements = initial_elements_per_goal; // Elements per window
        goals_data->current_window = 0;
        
        // Initialize each goal metadata
        std::cout << "  Initializing " << num_goals << " goal metadata..." << std::endl;
        for (size_t i = 0; i < num_goals; ++i) {
            GoalData* goal = goals_data->get_goal(i);
            new(goal) GoalData(); // Placement new
            
            // Progress indicator for large numbers
            if (num_goals > 1000 && (i + 1) % 1000 == 0) {
                std::cout << "    Initialized " << (i + 1) << "/" << num_goals << " goals..." << std::endl;
            }
        }
        
        std::cout << "Initialized with column-wise layout for " << num_goals << " goals!" << std::endl;
        goals_data->print_layout_info();
    }
    
    ~GoalsDataStorage() {
        delete mapped_region;
        delete file_mapping;
        if (!mapped_file.empty()) {
            unlink(mapped_file.c_str());
        }
    }
    
    void push_to_goal(size_t goal_index, char value) {
        try {
            goals_data->push_to_goal(goal_index, value);
        } catch (const std::runtime_error& e) {
            // Need more space
            grow_storage();
            goals_data->push_to_goal(goal_index, value); // Retry
        }
    }
    
    // Bulk operations for efficiency
    void push_to_all_goals(char base_value) {
        try {
            goals_data->push_to_all_goals(base_value);
        } catch (const std::runtime_error& e) {
            grow_storage();
            goals_data->push_to_all_goals(base_value);
        }
    }
    
    void push_to_goal_range(size_t start_goal, size_t end_goal, char base_value) {
        try {
            goals_data->push_to_goal_range(start_goal, end_goal, base_value);
        } catch (const std::runtime_error& e) {
            grow_storage();
            goals_data->push_to_goal_range(start_goal, end_goal, base_value);
        }
    }
    
    char get_from_goal(size_t goal_index, size_t element_index) const {
        return goals_data->get_element(goal_index, element_index);
    }
    
    size_t get_goal_size(size_t goal_index) const {
        const GoalData* goal = goals_data->get_goal(goal_index);
        return goal->size();
    }
    
    void print_status() const {
        std::cout << "\nGoalsDataStorage status:" << std::endl;
        goals_data->print_layout_info();
        std::cout << "  File size: " << current_file_size / (1024*1024) << " MB" << std::endl;
        std::cout << "  Current window: " << goals_data->current_window << std::endl;
    }
    
    void force_flush() {
        if (mapped_region) {
            mapped_region->flush();
            std::cout << "Forced flush to disk completed" << std::endl;
        }
    }
    
    size_t get_num_goals() const { return goals_data->num_goals; }
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
        // Create goals storage with 8000 goals and larger windows
        size_t num_goals = 8000;
        size_t window_size_mb = 500; // Larger window for 8K goals
        GoalsDataStorage storage(num_goals, window_size_mb);
        
        std::cout << "\nFilling 8000 goals with data (column-wise layout)..." << std::endl;
        
        // More efficient bulk operations for large goal counts
        const size_t elements_per_round = 100000; // 100K elements per round (more reasonable)
        const size_t rounds = 20; // 20 rounds = ~2M elements per goal
        
        for (size_t round = 0; round < rounds; ++round) {
            std::cout << "\n--- Round " << round + 1 << "/" << rounds << " ---" << std::endl;
            
            for (size_t element = 0; element < elements_per_round; ++element) {
                // Use bulk operations for efficiency
                if (element % 3 == 0) {
                    // Every 3rd element, add to all goals at once
                    storage.push_to_all_goals('A' + (element % 26));
                } else if (element % 3 == 1) {
                    // Add to first half of goals
                    storage.push_to_goal_range(0, num_goals / 2, 'X' + (element % 3));
                } else {
                    // Add to second half of goals
                    storage.push_to_goal_range(num_goals / 2, num_goals, '0' + (element % 10));
                }
                
                // Print progress less frequently for large numbers
                if ((element + 1) % 50000 == 0) { // Every 50K elements
                    std::cout << "  Added " << (element + 1) / 1000 << "K elements" << std::endl;
                }
            }
            
            storage.print_status();
            std::cout << "Virtual memory: " << get_virtual_memory_kb() / 1024 << " MB" << std::endl;
        }
        
        std::cout << "\nFinal status:" << std::endl;
        storage.print_status();
        std::cout << "Final virtual memory: " << get_virtual_memory_kb() / 1024 << " MB" << std::endl;
        
        // Force final flush
        storage.force_flush();
        
        // Test random access on a sampling of goals
        std::cout << "\nTesting column-wise access pattern (sampling):" << std::endl;
        std::vector<size_t> test_goals = {0, 1000, 4000, 7999}; // Sample goals
        
        for (size_t i = 0; i < 5; ++i) {
            std::cout << "Index " << i << ": ";
            for (size_t goal : test_goals) {
                if (i < storage.get_goal_size(goal)) {
                    std::cout << "Goal[" << goal << "]='" << storage.get_from_goal(goal, i) << "' ";
                }
            }
            std::cout << std::endl;
        }
        
        std::cout << "\nsuccess - Column-wise layout working with 8000 goals!" << std::endl;
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        std::cout << "Final virtual memory: " << get_virtual_memory_kb() / 1024 << " MB" << std::endl;
        return 1;
    }
}