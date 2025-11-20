#include <gtest/gtest.h>
#include "MappedObject.hpp"
#include "InMemoryMappedRegion.hpp"
#include "OffsetCalculator.hpp"

namespace mapped_object_tests
{
    class GIVEN_InMemoryMappedRegion : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            offset::config::window_size = 2;
            offset::config::nb_objects = 2;
            window_size_bytes = mapped_object::MappedObject::get_total_data_size_jump() * offset::config::window_size * offset::config::nb_objects;
            region = new InMemoryMappedRegion(window_size_bytes, window_size_bytes);
            mapped_object::MappedObject::set_mapped_region(region);
            base = static_cast<char *>(region->get_address());
        }

        void TearDown() override
        {
            delete region;
            mapped_object::MappedObject::set_mapped_region(nullptr);
        }

        size_t window_size_bytes;
        InMemoryMappedRegion *region;
        char *base;
    };

    class GIVEN_object_1 : public GIVEN_InMemoryMappedRegion
    {
    protected:
        void SetUp() override
        {
            GIVEN_InMemoryMappedRegion::SetUp();
            obj = std::make_unique<mapped_object::MappedObject>(0);
        }
        std::unique_ptr<mapped_object::MappedObject> obj;
    };

    class GIVEN_object_1_data_1 : public GIVEN_object_1
    {
    protected:
        void SetUp() override
        {
            GIVEN_object_1::SetUp();
            data = &obj->data1;
        }

        using data_t = decltype(obj->data1)::underlying_type;
        decltype(obj->data1) *data;
    };

    TEST_F(GIVEN_object_1_data_1, WHEN_push_back_twice_THEN_data_written_to_memory)
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

    class GIVEN_object_1_data_2 : public GIVEN_object_1
    {
    protected:
        void SetUp() override
        {
            GIVEN_object_1::SetUp();
            data = &obj->data2;
        }

        using data_t = decltype(obj->data2)::underlying_type;
        decltype(obj->data2) *data;
    };

    TEST_F(GIVEN_object_1_data_2, WHEN_push_back_twice_THEN_data_written_to_memory)
    {
        data->push_back(3);
        data->push_back(4);

        EXPECT_EQ((*data)[0], 3);
        EXPECT_EQ((*data)[1], 4);
    }

    class GIVEN_object_1_data_3 : public GIVEN_object_1
    {
    protected:
        void SetUp() override
        {
            GIVEN_object_1::SetUp();
            data = &obj->data3;
        }

        using data_t = decltype(obj->data3)::underlying_type;
        decltype(obj->data3) *data;
    };

    TEST_F(GIVEN_object_1_data_3, WHEN_push_back_twice_THEN_data_written_to_memory)
    {
        data->push_back(5);
        data->push_back(6);

        EXPECT_EQ((*data)[0], 5);
        EXPECT_EQ((*data)[1], 6);
    }

    class GIVEN_object_2 : public GIVEN_InMemoryMappedRegion
    {
    protected:
        void SetUp() override
        {
            GIVEN_InMemoryMappedRegion::SetUp();
            obj = std::make_unique<mapped_object::MappedObject>(1);
        }
        std::unique_ptr<mapped_object::MappedObject> obj;
        const size_t obj_2_jump = mapped_object::MappedObject::get_total_data_size_jump() * offset::config::window_size;
    };

    class GIVEN_object_2_data_1 : public GIVEN_object_2
    {
    protected:
        void SetUp() override
        {
            GIVEN_object_2::SetUp();
            data = &obj->data1;
        }

        using data_t = decltype(obj->data1)::underlying_type;
        decltype(obj->data1) *data;
    };

    TEST_F(GIVEN_object_2_data_1, WHEN_push_back_twice_THEN_data_written_to_memory)
    {
        data->push_back(42);
        data_t *data_elem_1_ptr = reinterpret_cast<data_t *>(base + obj_2_jump);
        EXPECT_EQ(*data_elem_1_ptr, 42);

        data->push_back(11);
        data_t *data_elem_2_ptr = reinterpret_cast<data_t *>(base + obj_2_jump + sizeof(data_t));
        EXPECT_EQ(*data_elem_2_ptr, 11);

        EXPECT_EQ((*data)[0], 42);
        EXPECT_EQ((*data)[1], 11);
    }

    class GIVEN_object_2_data_2 : public GIVEN_object_2
    {
    protected:
        void SetUp() override
        {
            GIVEN_object_2::SetUp();
            data = &obj->data2;
        }

        using data_t = decltype(obj->data2)::underlying_type;
        decltype(obj->data2) *data;
    };

    TEST_F(GIVEN_object_2_data_2, WHEN_push_back_twice_THEN_data_written_to_memory)
    {
        data->push_back(3);
        data->push_back(4);

        EXPECT_EQ((*data)[0], 3);
        EXPECT_EQ((*data)[1], 4);
    }

    class GIVEN_object_2_data_3 : public GIVEN_object_2
    {
    protected:
        void SetUp() override
        {
            GIVEN_object_2::SetUp();
            data = &obj->data3;
        }

        using data_t = decltype(obj->data3)::underlying_type;
        decltype(obj->data3) *data;
    };

    TEST_F(GIVEN_object_2_data_3, WHEN_push_back_twice_THEN_data_written_to_memory)
    {
        data->push_back(9);
        data->push_back(10);

        EXPECT_EQ((*data)[0], 9);
        EXPECT_EQ((*data)[1], 10);
    }
}