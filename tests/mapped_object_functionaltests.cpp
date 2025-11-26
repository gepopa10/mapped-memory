#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <fstream>
#include <sstream>
#include <chrono>

#include "MappedObject.hpp"
#include "OffsetCalculator.hpp"
#include "FileMappedRegion.hpp"
#include "InMemoryMappedRegion.hpp"

namespace mapped_object_tests
{
    class GIVEN_objects : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            offset::config::window_size = 2;
            offset::config::nb_objects = 10;
            const auto window_size_bytes = mapped_object::MappedObject::get_total_data_size_jump() * offset::config::window_size * offset::config::nb_objects;
            const std::string filename = "file_mapped_region_test.dat";
            region = new FileMappedRegion(filename, window_size_bytes);
            mapped_object::MappedObject::set_mapped_region(region);
            base = static_cast<char *>(region->get_address());
            objs.reserve(offset::config::nb_objects);
            for (int i = 0; i < offset::config::nb_objects; i++)
            {
                objs.emplace_back(i);
            }
        }

        void TearDown() override
        {
            delete region;
            mapped_object::MappedObject::set_mapped_region(nullptr);
        }
        FileMappedRegion *region;
        char *base;
        std::vector<mapped_object::MappedObject> objs;
    };

    TEST_F(GIVEN_objects, WHEN_push_back_THEN_data_written_to_memory)
    {
        for (int i = 0; i < 8; i++)
        {
            for (auto &obj : objs)
            {
                obj.data1.push_back(1 + i);
                obj.data2.push_back(2 + i);
                obj.data3.push_back(3 + i);
            }
        }

        EXPECT_EQ(objs[5].data2[6], 2 + 6);
    }

    class GIVEN_8000_objects : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            offset::config::window_size = 2;
            offset::config::nb_objects = 8000;
            const auto window_size_bytes = mapped_object::MappedObject::get_total_data_size_jump() * offset::config::window_size * offset::config::nb_objects;
            const std::string filename = "file_mapped_region_test.dat";
            region = new FileMappedRegion(filename, window_size_bytes);
            mapped_object::MappedObject::set_mapped_region(region);
            base = static_cast<char *>(region->get_address());
            objs.reserve(offset::config::nb_objects);
            for (int i = 0; i < offset::config::nb_objects; i++)
            {
                objs.emplace_back(i);
            }
        }

        void TearDown() override
        {
            delete region;
            mapped_object::MappedObject::set_mapped_region(nullptr);
        }
        FileMappedRegion *region;
        char *base;
        std::vector<mapped_object::MappedObject> objs;
    };

    TEST_F(GIVEN_8000_objects, WHEN_push_back_THEN_data_written_to_memory)
    {
        for (int i = 0; i < 2; i++)
        {
            for (auto &obj : objs)
            {
                obj.data1.push_back(1 + i);
                obj.data2.push_back(2 + i);
                obj.data3.push_back(3 + i);
            }
        }
        EXPECT_EQ(objs[7999].data2[1], 2 + 1);
    }

    class GIVEN_8000_objects_window_50 : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            offset::config::window_size = 50;
            offset::config::nb_objects = 8000;
            const auto window_size_bytes = mapped_object::MappedObject::get_total_data_size_jump() * offset::config::window_size * offset::config::nb_objects;
            const std::string filename = "file_mapped_region_test.dat";
            region = new FileMappedRegion(filename, window_size_bytes);
            mapped_object::MappedObject::set_mapped_region(region);
            base = static_cast<char *>(region->get_address());
            objs.reserve(offset::config::nb_objects);
            for (int i = 0; i < offset::config::nb_objects; i++)
            {
                objs.emplace_back(i);
            }
        }

        void TearDown() override
        {
            delete region;
            mapped_object::MappedObject::set_mapped_region(nullptr);
        }
        FileMappedRegion *region;
        char *base;
        std::vector<mapped_object::MappedObject> objs;
    };

    TEST_F(GIVEN_8000_objects_window_50, WHEN_push_back_THEN_data_written_to_memory)
    {
        for (int i = 0; i < 50; i++)
        {
            for (auto &obj : objs)
            {
                obj.data1.push_back(1 + i);
                obj.data2.push_back(2 + i);
                obj.data3.push_back(3 + i);
            }
        }

        const int i_value{49};
        EXPECT_EQ(objs[7568].data2[i_value], 2 + i_value);
    }

    class GIVEN_8000_objects_2_windows : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            offset::config::window_size = 2;
            offset::config::nb_objects = 8000;
            const auto window_size_bytes = mapped_object::MappedObject::get_total_data_size_jump() * offset::config::window_size * offset::config::nb_objects;
            const std::string filename = "file_mapped_region_test.dat";
            region = new FileMappedRegion(filename, window_size_bytes);
            mapped_object::MappedObject::set_mapped_region(region);
            base = static_cast<char *>(region->get_address());
            objs.reserve(offset::config::nb_objects);
            for (int i = 0; i < offset::config::nb_objects; i++)
            {
                objs.emplace_back(i);
            }
        }

        void TearDown() override
        {
            delete region;
            mapped_object::MappedObject::set_mapped_region(nullptr);
        }
        FileMappedRegion *region;
        char *base;
        std::vector<mapped_object::MappedObject> objs;
    };

    TEST_F(GIVEN_8000_objects_2_windows, WHEN_push_back_THEN_data_written_to_memory)
    {
        for (int i = 0; i < 4; i++)
        {
            for (auto &obj : objs)
            {
                obj.data1.push_back(1 + i);
                obj.data2.push_back(2 + i);
                obj.data3.push_back(3 + i);
            }
        }
        {
            const int i_value{1};
            EXPECT_EQ(objs[7568].data2[i_value], 2 + i_value);
        }
        {
            const int i_value{2};
            EXPECT_EQ(objs[7568].data2[i_value], 2 + i_value);
        }
        {
            const int i_value{3};
            EXPECT_EQ(objs[7568].data2[i_value], 2 + i_value);
        }
    }

    class GIVEN_8000_objects_multiple_windows : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            offset::config::window_size = 3;
            offset::config::nb_objects = 8000;
            const auto window_size_bytes = mapped_object::MappedObject::get_total_data_size_jump() * offset::config::window_size * offset::config::nb_objects;
            const std::string filename = "file_mapped_region_test.dat";
            region = new FileMappedRegion(filename, window_size_bytes);
            mapped_object::MappedObject::set_mapped_region(region);
            base = static_cast<char *>(region->get_address());
            objs.reserve(offset::config::nb_objects);
            for (int i = 0; i < offset::config::nb_objects; i++)
            {
                objs.emplace_back(i);
            }
        }

        void TearDown() override
        {
            delete region;
            mapped_object::MappedObject::set_mapped_region(nullptr);
        }
        FileMappedRegion *region;
        char *base;
        std::vector<mapped_object::MappedObject> objs;
    };

    TEST_F(GIVEN_8000_objects_multiple_windows, WHEN_push_back_THEN_data_written_to_memory)
    {
        for (int i = 0; i < 12; i++)
        {
            for (auto &obj : objs)
            {
                obj.data1.push_back(1 + i);
                obj.data2.push_back(2 + i);
                obj.data3.push_back(3 + i);
            }
        }

        {
            const int i_value{1};
            EXPECT_EQ(objs[7568].data2[i_value], 2 + i_value);
        }

        {
            const int i_value{11};
            EXPECT_EQ(objs[2345].data3[i_value], 3 + i_value);
        }
    }

    class GIVEN_8000_objects_lot_of_data : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            offset::config::window_size = 50;
            offset::config::nb_objects = 8000;
            const auto window_size_bytes = mapped_object::MappedObject::get_total_data_size_jump() * offset::config::window_size * offset::config::nb_objects;
            const std::string filename = "file_mapped_region_test.dat";
            region = new FileMappedRegion(filename, window_size_bytes);
            mapped_object::MappedObject::set_mapped_region(region);
            base = static_cast<char *>(region->get_address());
            objs.reserve(offset::config::nb_objects);
            for (int i = 0; i < offset::config::nb_objects; i++)
            {
                objs.emplace_back(i);
            }
        }

        void TearDown() override
        {
            delete region;
            mapped_object::MappedObject::set_mapped_region(nullptr);
        }
        FileMappedRegion *region;
        char *base;
        std::vector<mapped_object::MappedObject> objs;
    };

    TEST_F(GIVEN_8000_objects_lot_of_data, WHEN_push_back_THEN_data_written_to_memory)
    {
        for (int i = 0; i < 5000; i++)
        {
            for (auto &obj : objs)
            {
                obj.data1.push_back((1 + i) % std::numeric_limits<decltype(obj.data1)::underlying_type>::max());
                obj.data2.push_back((2 + i) % std::numeric_limits<decltype(obj.data2)::underlying_type>::max());
                obj.data3.push_back((3 + i) % std::numeric_limits<decltype(obj.data3)::underlying_type>::max());
            }
        }

        {
            const int i_value{1};
            EXPECT_EQ(objs[7568].data2[i_value], 2 + i_value);
        }

        {
            const int i_value{67};
            EXPECT_EQ(objs[2345].data3[i_value], 3 + i_value);
        }

        {
            const int i_value{787};
            EXPECT_EQ(objs[2345].data1[i_value], 1 + i_value);
        }

        {
            const int i_value{4600};
            EXPECT_EQ(objs[2345].data1[i_value], 1 + i_value);
        }
    }

    class GIVEN_8000_objects_lot_of_data_with_memory_hog : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            offset::config::window_size = 50;
            offset::config::nb_objects = 8000;
            const auto window_size_bytes = mapped_object::MappedObject::get_total_data_size_jump() * offset::config::window_size * offset::config::nb_objects;
            const std::string filename = "file_mapped_region_test.dat";
            region = new FileMappedRegion(filename, window_size_bytes);
            mapped_object::MappedObject::set_mapped_region(region);
            objs.reserve(offset::config::nb_objects);
            for (int i = 0; i < offset::config::nb_objects; i++)
            {
                objs.emplace_back(i);
            }
        }

        void TearDown() override
        {
            delete region;
            mapped_object::MappedObject::set_mapped_region(nullptr);
            free_memory_hog();
        }

        void allocate_memory_hog()
        {
            if (memory_hog_allocated)
            {
                return; // Already allocated
            }

            const size_t chunk_size = 100 * 1024 * 1024; // 100 MB
            size_t chunk_number = 0;

            std::cout << "\n=== Starting memory hog allocation ===" << std::endl;
            std::cout << "Chunk size: 100 MB" << std::endl;

            // Get available memory
            std::ifstream meminfo("/proc/meminfo");
            std::string line;
            size_t mem_available_kb = 0;

            while (std::getline(meminfo, line))
            {
                if (line.find("MemAvailable:") == 0)
                {
                    std::istringstream iss(line);
                    std::string label;
                    iss >> label >> mem_available_kb;
                    break;
                }
            }

            const size_t mem_available_bytes = mem_available_kb * 1024;
            const size_t target_bytes = static_cast<size_t>(mem_available_bytes * 0.95);
            const size_t target_chunks = target_bytes / chunk_size;

            std::cout << "Available memory: " << (mem_available_bytes / (1024.0 * 1024.0 * 1024.0)) << " GB" << std::endl;
            std::cout << "Target allocation (95%): " << (target_bytes / (1024.0 * 1024.0 * 1024.0)) << " GB" << std::endl;
            std::cout << "Target chunks: " << target_chunks << std::endl
                      << std::endl;

            while (chunk_number < target_chunks)
            {
                try
                {
                    char *chunk = new char[chunk_size];
                    // Touch the memory to ensure it's actually allocated
                    for (size_t i = 0; i < chunk_size; i += 4096)
                    {
                        chunk[i] = (char)i;
                    }
                    memory_chunks.push_back(chunk);
                    chunk_number++;

                    size_t total_mb = chunk_number * 100;
                    if (chunk_number % 10 == 0)
                    {
                        std::cout << "Allocated " << total_mb << " MB "
                                  << int((chunk_number * 100.0) / (target_chunks * 100) * 100) << "%" << std::endl;
                    }
                }
                catch (const std::bad_alloc &e)
                {
                    std::cout << "\n!!! Allocation failed at chunk " << chunk_number << " !!!" << std::endl;
                    std::cout << "Successfully allocated " << chunk_number
                              << " chunks = " << (chunk_number * 100) << " MB" << std::endl;
                    break;
                }
            }

            std::cout << "\nNow holding memory: " << (memory_chunks.size() * 100)
                      << " MB | Chunks: " << memory_chunks.size() << std::endl;
            const auto mem_available_mb = mem_available_bytes / 1024 / 1024;
            std::cout << "That's " << int((memory_chunks.size() * 100.0 / mem_available_mb) * 100)
                      << "% of available memory" << std::endl;
            std::cout << "=== Memory hog allocated ===" << std::endl
                      << std::endl;

            memory_hog_allocated = true;
        }

        void free_memory_hog()
        {
            if (!memory_chunks.empty())
            {
                std::cout << "\nFreeing memory hog: " << (memory_chunks.size() * 100) << " MB" << std::endl;
                for (char *chunk : memory_chunks)
                {
                    delete[] chunk;
                }
                memory_chunks.clear();
                memory_hog_allocated = false;
            }
        }

        IMappedRegion *region;
        std::vector<mapped_object::MappedObject> objs;
        std::vector<char *> memory_chunks;
        bool memory_hog_allocated = false;
    };

    TEST_F(GIVEN_8000_objects_lot_of_data_with_memory_hog, WHEN_push_back_THEN_data_written_to_memory)
    {
        for (int i = 0; i < 10000; i++)
        {
            // After 100 iterations, allocate memory hog
            if (i == 100)
            {
                std::cout << "\n>>> Iteration " << i << " - Activating memory hog! <<<" << std::endl;
                allocate_memory_hog();
                std::cout << ">>> Continuing with memory pressure... <<<\n"
                          << std::endl;
            }

            for (auto &obj : objs)
            {
                obj.data1.push_back((1 + i) % std::numeric_limits<decltype(obj.data1)::underlying_type>::max());
                obj.data2.push_back((2 + i) % std::numeric_limits<decltype(obj.data2)::underlying_type>::max());
                obj.data3.push_back((3 + i) % std::numeric_limits<decltype(obj.data3)::underlying_type>::max());
            }
        }

        std::chrono::duration<double, std::micro> stale_access_time;
        std::chrono::duration<double, std::micro> hot_access_time_from_another_data;
        std::chrono::duration<double, std::micro> hot_access_time_2;
        std::chrono::duration<double, std::micro> cold_access_time;

        {
            auto start = std::chrono::high_resolution_clock::now();
            const int i_value{1};
            EXPECT_EQ(objs[7568].data2[i_value], 2 + i_value);
            auto end = std::chrono::high_resolution_clock::now();
            stale_access_time = end - start;
            std::cout << "Stale memory access time (index 1): " << stale_access_time.count() << " us" << std::endl;
        }

        {
            auto start = std::chrono::high_resolution_clock::now();
            const int i_value{2};
            EXPECT_EQ(objs[7568].data1[i_value], 1 + i_value);
            auto end = std::chrono::high_resolution_clock::now();
            hot_access_time_from_another_data = end - start;
            std::cout << "Hot memory access time from previous data access (index 2): " << hot_access_time_from_another_data.count() << " us" << std::endl;
        }

        {
            auto start = std::chrono::high_resolution_clock::now();
            const int i_value{9999};
            EXPECT_EQ(objs[2345].data1[i_value], 1 + i_value);
            auto end = std::chrono::high_resolution_clock::now();
            hot_access_time_2 = end - start;
            std::cout << "Hot memory access time 2 (index 9999): " << hot_access_time_2.count() << " us" << std::endl;
        }

        {
            auto start = std::chrono::high_resolution_clock::now();
            const int i_value{5000};
            EXPECT_EQ(objs[2345].data1[i_value], 1 + i_value);
            auto end = std::chrono::high_resolution_clock::now();
            cold_access_time = end - start;
            std::cout << "Cold memory access time (index 2000): " << cold_access_time.count() << " us" << std::endl;
        }

        // Calculate average hot access time
        auto avg_hot_access_time = (hot_access_time_from_another_data + hot_access_time_2) / 2.0;

        std::cout << "\n=== Memory Access Performance Analysis ===" << std::endl;
        std::cout << "Stale access: " << stale_access_time.count() << " us" << std::endl;
        std::cout << "Hot access (avg): " << avg_hot_access_time.count() << " us" << std::endl;
        std::cout << "Cold access: " << cold_access_time.count() << " us" << std::endl;

        // Assert: First stale access should be at least 10x slower than hot accesses
        EXPECT_GT(stale_access_time.count(), avg_hot_access_time.count() * 10)
            << "Stale memory access should be at least 10x slower than hot memory. "
            << "Stale: " << stale_access_time.count() << " us, "
            << "Hot avg: " << avg_hot_access_time.count() << " us";

        // Assert: Hot accesses (index 2 and 9999) should be fast (< 10 us)
        EXPECT_LT(hot_access_time_from_another_data.count(), 10.0)
            << "Hot memory access should be < 10 us, got " << hot_access_time_from_another_data.count() << " us";
        EXPECT_LT(hot_access_time_2.count(), 10.0)
            << "Hot memory access should be < 10 us, got " << hot_access_time_2.count() << " us";

        // Assert: Cold access (index 2000) should be at least 5x slower than hot
        EXPECT_GT(cold_access_time.count(), hot_access_time_2.count() * 5)
            << "Cold memory access should be at least 5x slower than hot memory. "
            << "Cold: " << cold_access_time.count() << " us, "
            << "Hot avg: " << hot_access_time_2.count() << " us";
    }

    class GIVEN_8000_objects_lot_of_data_in_memory_with_memory_hog : public GIVEN_8000_objects_lot_of_data_with_memory_hog
    {
    protected:
        void SetUp() override
        {
            // Don't call base SetUp() - we override it completely
            offset::config::window_size = 50;
            offset::config::nb_objects = 8000;
            const auto window_size_bytes = mapped_object::MappedObject::get_total_data_size_jump() * offset::config::window_size * offset::config::nb_objects;

            region = new InMemoryMappedRegion(window_size_bytes);
            mapped_object::MappedObject::set_mapped_region(region);

            objs.reserve(offset::config::nb_objects);
            for (int i = 0; i < offset::config::nb_objects; i++)
            {
                objs.emplace_back(i);
            }
        }
    };

    TEST_F(GIVEN_8000_objects_lot_of_data_in_memory_with_memory_hog, WHEN_push_back_THEN_death)
    {
        EXPECT_DEATH(
            {
                for (int i = 0; i < 10000; i++)
                {
                    // After 100 iterations, allocate memory hog
                    if (i == 100)
                    {
                        std::cout << "\n>>> Iteration " << i << " - Activating memory hog! <<<" << std::endl;
                        allocate_memory_hog();
                        std::cout << ">>> Continuing with memory pressure... <<<\n"
                                  << std::endl;
                    }

                    for (auto &obj : objs)
                    {
                        obj.data1.push_back((1 + i) % std::numeric_limits<decltype(obj.data1)::underlying_type>::max());
                        obj.data2.push_back((2 + i) % std::numeric_limits<decltype(obj.data2)::underlying_type>::max());
                        obj.data3.push_back((3 + i) % std::numeric_limits<decltype(obj.data3)::underlying_type>::max());
                    }
                }
            },
            "");
    }
}