#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "MappedObject.hpp"
#include "OffsetCalculator.hpp"
#include "FileMappedRegion.hpp"

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

    // to make it pass configs should be in a singleton

    // class GIVEN_8000_objects_multiple_windows : public ::testing::Test
    // {
    // protected:
    //     void SetUp() override
    //     {
    //         offset::config::window_size = 3;
    //         offset::config::nb_objects = 8000;
    //         const auto window_size_bytes = mapped_object::MappedObject::get_total_data_size_jump() * offset::config::window_size * offset::config::nb_objects;
    //         const std::string filename = "file_mapped_region_test.dat";
    //         region = new FileMappedRegion(filename, window_size_bytes);
    //         mapped_object::MappedObject::set_mapped_region(region);
    //         base = static_cast<char *>(region->get_address());
    //         objs.reserve(offset::config::nb_objects);
    //         for (int i = 0; i < offset::config::nb_objects; i++)
    //         {
    //             objs.emplace_back(i);
    //         }
    //     }

    //     void TearDown() override
    //     {
    //         delete region;
    //         mapped_object::MappedObject::set_mapped_region(nullptr);
    //     }
    //     FileMappedRegion *region;
    //     char *base;
    //     std::vector<mapped_object::MappedObject> objs;
    // };

    // TEST_F(GIVEN_8000_objects_multiple_windows, WHEN_push_back_THEN_data_written_to_memory)
    // {
    //     for (int i = 0; i < 6; i++)
    //     {
    //         for (auto &obj : objs)
    //         {
    //             obj.data1.push_back(1 + i);
    //             obj.data2.push_back(2 + i);
    //             obj.data3.push_back(3 + i);
    //         }
    //     }
    //     {
    //         const int i_value{1};
    //         EXPECT_EQ(objs[7568].data2[i_value], 2 + i_value);
    //     }
    
    // }
}