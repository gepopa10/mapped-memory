#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "MappedObject.hpp"
#include "OffsetCalculator.hpp"
#include "FileMappedRegion.hpp"

namespace mapped_object_tests
{
    class GIVEN_FileMappedRegion : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            offset::config::window_size = 2;
            offset::config::nb_objects = 2;
            const auto window_size_bytes = mapped_object::MappedObject::get_total_data_size_jump() * offset::config::window_size * offset::config::nb_objects;
            const std::string filename = "file_mapped_region_test.dat";
            region = new FileMappedRegion(filename, window_size_bytes);
            mapped_object::MappedObject::set_mapped_region(region);
            base = static_cast<char *>(region->get_address());
        }

        void TearDown() override
        {
            delete region;
            mapped_object::MappedObject::set_mapped_region(nullptr);
        }

        FileMappedRegion *region;
        char *base;
    };

    class GIVEN_object_1_file : public GIVEN_FileMappedRegion
    {
    protected:
        void SetUp() override
        {
            GIVEN_FileMappedRegion::SetUp();
            obj = std::make_unique<mapped_object::MappedObject>(0);
        }
        std::unique_ptr<mapped_object::MappedObject> obj;
    };

    class GIVEN_object_1_data_1_file : public GIVEN_object_1_file
    {
    protected:
        void SetUp() override
        {
            GIVEN_object_1_file::SetUp();
            data = &obj->data1;
        }

        using data_t = decltype(obj->data1)::underlying_type;
        decltype(obj->data1) *data;
    };

    TEST_F(GIVEN_object_1_data_1_file, WHEN_push_back_twice_THEN_data_written_to_memory)
    {
        data->push_back(42);
        data_t *data_elem_1_ptr = reinterpret_cast<data_t *>(base);
        EXPECT_EQ(*data_elem_1_ptr, 42);

        data->push_back(11);
        data_t *data_elem_2_ptr = reinterpret_cast<data_t *>(base + sizeof(data_t));
        EXPECT_EQ(*data_elem_2_ptr, 11);

        EXPECT_EQ((*data)[0], 42);
        EXPECT_EQ((*data)[1], 11);
    }

    class GIVEN_object_1_and_2_file : public GIVEN_FileMappedRegion
    {
    protected:
        void SetUp() override
        {
            GIVEN_FileMappedRegion::SetUp();
            obj1 = std::make_unique<mapped_object::MappedObject>(0);
            obj2 = std::make_unique<mapped_object::MappedObject>(1);
        }
        std::unique_ptr<mapped_object::MappedObject> obj1;
        std::unique_ptr<mapped_object::MappedObject> obj2;
    };

    TEST_F(GIVEN_object_1_and_2_file, WHEN_push_back_double_window_plus_one_different_object_THEN_memory_data_written_to_memory)
    {
        {
            int diff = 0;
            auto &data = obj1->data2;

            for (size_t i = 0; i < offset::config::window_size + 1; ++i)
            {
                data.push_back(i + diff);
            }
            EXPECT_EQ(data[0], 0 + diff);
            EXPECT_EQ(data[1], 1 + diff);
            EXPECT_EQ(data[2], 2 + diff);
        }

        {
            int diff = 20;
            auto &data = obj2->data3;

            for (size_t i = 0; i < offset::config::window_size + 3; ++i)
            {
                data.push_back(i + diff);
            }
            EXPECT_EQ(data[0], 0 + diff);
            EXPECT_EQ(data[1], 1 + diff);
            EXPECT_EQ(data[2], 2 + diff);
            EXPECT_EQ(data[3], 3 + diff);
            EXPECT_EQ(data[4], 4 + diff);
        }
    }
}