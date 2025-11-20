#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <sys/stat.h>
#include <unistd.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

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

void create_or_extend_file(const std::string &filename, size_t new_size_bytes)
{
    int fd = open(filename.c_str(), O_CREAT | O_RDWR, 0644);
    if (fd == -1)
    {
        throw std::runtime_error("Failed to create file");
    }

    if (ftruncate(fd, new_size_bytes) == -1)
    {
        close(fd);
        throw std::runtime_error("Failed to resize file");
    }
    close(fd);
}

template <typename T>
class MappedVector
{
    const std::string mapped_file = "/tmp/memory_mapped.dat";
    static constexpr size_t window_size = 100000;
    static constexpr size_t window_size_bytes = window_size * sizeof(T);
    static boost::interprocess::file_mapping *file_mapping;
    static boost::interprocess::mapped_region *mapped_region;
    size_t current_file_size_bytes = 0;
    size_t current_window = 0; // Current window index (0-based)
    size_t total_size = 0;
    size_t window_size_elements = 0;

    std::string generate_unique_name()
    {
        boost::uuids::random_generator gen;
        boost::uuids::uuid id = gen();
        return "Vector_" + boost::uuids::to_string(id);
    }

    std::string data_name = generate_unique_name();

    void flush_and_grow()
    {
        std::cout << "Window " << current_window << " full. Flushing to disk and growing..." << std::endl;

        // Flush current data to disk
        mapped_region->flush();

        // Move to next window
        current_window++;

        // Calculate new file size (add another window)
        size_t new_file_size = current_file_size_bytes + window_size_bytes;

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

    bool is_window_full() const
    {
        size_t elements_in_current_window = total_size % window_size_elements;
        return elements_in_current_window == 0 && total_size > 0;
    }

public:
    MappedVector()
    {
        window_size_elements = window_size_bytes / sizeof(T);
        boost::interprocess::file_mapping::remove(mapped_file.c_str());
        current_file_size_bytes = window_size_bytes;
        create_or_extend_file(mapped_file, current_file_size_bytes);

        if (!file_mapping)
        {
            file_mapping = new boost::interprocess::file_mapping(mapped_file.c_str(), boost::interprocess::read_write);
            mapped_region = new boost::interprocess::mapped_region(*file_mapping, boost::interprocess::read_write);
        }

        std::cout << "Initialized storage with window size: " << window_size_bytes << " bytes" << std::endl;
    }
    ~MappedVector()
    {
        if (mapped_region)
        {
            delete mapped_region;
            delete file_mapping;
            mapped_region = nullptr;
            file_mapping = nullptr;
            if (!mapped_file.empty())
            {
                unlink(mapped_file.c_str());
            }
            boost::interprocess::file_mapping::remove(mapped_file.c_str());
        }
    }

    void push_back(T data)
    {
        if (is_window_full())
        {
            flush_and_grow();
        }

        size_t global_index = total_size;
        T *base_addr = static_cast<T *>(mapped_region->get_address());
        base_addr[global_index] = data;
        total_size++;
    }

    void print_free_memory()
    {
        std::cout << "size: " << total_size
                  << ", current_file_size_bytes: " << current_file_size_bytes
                  << ", mapped region size: " << mapped_region->get_size() << "\n";
    }
};

template <typename T>
boost::interprocess::file_mapping *MappedVector<T>::file_mapping = nullptr;

template <typename T>
boost::interprocess::mapped_region *MappedVector<T>::mapped_region = nullptr;

struct MappedVectors
{
    MappedVector<int64_t> vec1;
    MappedVector<int64_t> vec2;

    void push_back(int64_t data)
    {
        vec1.push_back(data);
        vec2.push_back(data);
    }

    void status()
    {
        vec1.print_free_memory();
    }
};

int main()
{
    const size_t initial_mem = get_virtual_memory_kb();
    std::cout << "Initial virtual memory: " << initial_mem << " KB" << std::endl;
    MemoryInfo mem_info = get_memory_info();
    std::cout << "System memory - Total: " << mem_info.total_kb / 1024 << " MB, "
              << "Free: " << mem_info.free_kb / 1024 << " MB, "
              << "Available: " << mem_info.available_kb / 1024 << " MB" << std::endl;

    try
    {
        MappedVectors vecs;
        for (int i = 0; i < 1'000'000'000; i++)
        {
            vecs.push_back(i);
            vecs.status();
            const MemoryInfo mem_info = get_memory_info();
            std::cout <<"test available: " << mem_info.available_kb << "\n";
        }
    }
    catch (const boost::interprocess::interprocess_exception &e)
    {
        std::cerr << "Interprocess error: " << e.what() << std::endl;
        std::cout << "Final virtual memory: " << get_virtual_memory_kb() / 1024 << " MB" << std::endl;
        return 1;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        std::cout << "Final virtual memory: " << get_virtual_memory_kb() / 1024 << " MB" << std::endl;
        return 1;
    }

    return 0;
}