#include <iostream>
#include <vector>
#include <type_traits>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "memory_info.hpp"
#include "utils.hpp"

namespace config
{
    const std::string mapped_file = "memory_mapped.dat";
    constexpr size_t window_size = 2;
    constexpr size_t nb_objects = 2;
}

namespace common
{
    size_t total_window_size = config::window_size;
    static boost::interprocess::file_mapping *file_mapping;
    static boost::interprocess::mapped_region *mapped_region;
    void flush_and_grow();
}

struct Object
{
    int32_t *data1_holder;
    using T1 = std::remove_pointer_t<decltype(data1_holder)>;
    int16_t *data2_holder;
    using T2 = std::remove_pointer_t<decltype(data2_holder)>;
    int8_t *data3_holder;
    using T3 = std::remove_pointer_t<decltype(data3_holder)>;
    static constexpr size_t total_data_size_jump = sizeof(T1) + sizeof(T2) + sizeof(T3); // 4 + 2 + 1

    template <typename T, size_t object_offset = 0, size_t data_holder_offset = 0>
    struct Accessor
    {
        size_t current_element_index = 0;
        size_t current_window_index = 0;

        void push_back(T value)
        {
            if (current_element_index != 0 && current_element_index % config::window_size == 0)
            {
                // as soon as one of the object request to push outside the window we need to grow,
                // but we need to avoid that other grow it afterwards also!
                if (current_window_index == (common::total_window_size / config::window_size - 1))
                {
                    common::flush_and_grow();
                    common::total_window_size += config::window_size;
                }

                current_window_index++;
            }

            size_t data_size_jump = config::window_size * data_holder_offset;
            size_t object_data_jump = total_data_size_jump * config::window_size;
            size_t window_jump = object_data_jump * config::nb_objects;
            size_t total_byte_offset = window_jump * current_window_index +
            object_offset * object_data_jump +
            data_size_jump +
            (current_element_index % config::window_size) * sizeof(T);
            
            // object 0 data 1.1 1.2 ... 1.window_size, data 2.1 2.2 ... 2.window_size
            // object 1 data 1.1 1.2 ... 1.window_size, data 2.1 2.2 ... 2.window_size
            // ...
            // object 0 data 1.window_size+1 1.2 ... 1.2xwindow_size, data 2.window_size+1 2.2 ... 2.2xwindow_size
            // object 1 data 1.window_size+1 1.2 ... 1.2xwindow_size, data 2.window_size+1 2.2 ... 2.2xwindow_size
            
            char *base_addr = static_cast<char *>(common::mapped_region->get_address());
            T *target = reinterpret_cast<T *>(base_addr + total_byte_offset);
            *target = value;
            current_element_index++;
        }
    };

    // lets say I have a window_size of 2, 2 nb_objects and 2 windows
    // object 0: 0,1,2,3      4,5,6,7     | 8,9    10,11  | 12 13
    // object 1: 14,15,16,17  18,19,20,21 | 22,23  24,25  | 26 27
    // next window --------------------------------------------
    // object 0: 28,29,30,31  32,33,34,35 | 36,37  38,39  | 40 41
    // object 1: 42,43,44,45  46,47,48,49 | 50,51  52,53  | 54 55

    // total_data_size_jump = 7
    // data_size_jump = 2 * (0-4-6)
    // object_data_jump = 7 * 2 = 14 ok!
    // window_jump = 7 * 2 * 2 = 28 ok!
    // total_byte_offset = 28 * w_index + 14 * (object 0-1) + 2 * (offset 0-4-6) + e * sizeof(T)

    Accessor<T1, 0, 0> data1;
    Accessor<T2, 0, sizeof(T1)> data2;
    Accessor<T3, 0, sizeof(T1) + sizeof(T2)> data3;
};

namespace common
{
    size_t window_size_bytes = Object::total_data_size_jump * config::window_size * config::nb_objects;

    void flush_and_grow()
    {
        std::cout << "Flushing to disk and growing..." << std::endl;
        common::mapped_region->flush();

        window_size_bytes *= 2;

        delete common::mapped_region;
        delete common::file_mapping;

        create_or_extend_file(config::mapped_file, window_size_bytes);

        // Create new mapping for entire file
        common::file_mapping = new boost::interprocess::file_mapping(config::mapped_file.c_str(), boost::interprocess::read_write);
        common::mapped_region = new boost::interprocess::mapped_region(*common::file_mapping, boost::interprocess::read_write);
    }
}

struct MappedObjects
{
    std::vector<Object> objects;

    MappedObjects()
    {
        objects.resize(config::nb_objects);
        boost::interprocess::file_mapping::remove(config::mapped_file.c_str());
        create_or_extend_file(config::mapped_file, common::window_size_bytes);

        if (!common::file_mapping)
        {
            common::file_mapping = new boost::interprocess::file_mapping(config::mapped_file.c_str(), boost::interprocess::read_write);
            common::mapped_region = new boost::interprocess::mapped_region(*common::file_mapping, boost::interprocess::read_write);
        }

        std::cout << "Initialized storage with window size: " << common::window_size_bytes << " bytes" << std::endl;
    }

    ~MappedObjects()
    {
        if (common::mapped_region)
        {
            delete common::mapped_region;
            delete common::file_mapping;
            common::mapped_region = nullptr;
            common::file_mapping = nullptr;
            if (!config::mapped_file.empty())
            {
                unlink(config::mapped_file.c_str());
            }
            boost::interprocess::file_mapping::remove(config::mapped_file.c_str());
        }
    }

    void print_free_memory()
    {
        std::cout << ", current stored bytes: " << common::window_size_bytes
                  << ", mapped region size: " << common::mapped_region->get_size() << "\n";
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
        MappedObjects storage;
        size_t object_index = 0;
        for (int i = 0; i < config::window_size * 2; i++)
        {
            storage.objects[object_index].data1.push_back(i);
            storage.objects[object_index].data2.push_back(i);
            storage.objects[object_index].data3.push_back(i);
            const MemoryInfo mem_info = get_memory_info();
            std::cout << "test available: " << mem_info.available_kb << "\n";
        }

        std::cout <<"end test" << std::endl;
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