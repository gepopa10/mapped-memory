#include <gtest/gtest.h>
#include <gmock/gmock.h>

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
            const auto window_size_bytes = mapped_object::MappedObject::get_total_data_size_jump() * offset::config::window_size * offset::config::nb_objects;
            region = new InMemoryMappedRegion(window_size_bytes);
            mapped_object::MappedObject::set_mapped_region(region);
            base = static_cast<char *>(region->get_address());
        }

        void TearDown() override
        {
            delete region;
            mapped_object::MappedObject::set_mapped_region(nullptr);
        }

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

    TEST_F(GIVEN_object_1_data_1, WHEN_iterate_THEN_data_accessed)
    {
        data->push_back(42);
        data->push_back(11);
        auto ite = (*data).begin();

        ++ite;
        EXPECT_EQ(*ite, 11);
    }

    TEST_F(GIVEN_object_1_data_1, WHEN_iterate_to_end_THEN_end_reached)
    {
        data->push_back(42);
        data->push_back(11);
        auto ite = (*data).begin();

        ++ite;
        ++ite;
        EXPECT_EQ(ite, (*data).end());
    }

    TEST_F(GIVEN_object_1_data_1, WHEN_iterate_over_window_THEN_access_right_value)
    {
        data->push_back(42);
        data->push_back(11);
        data->push_back(13);
        auto ite = (*data).begin();

        ++ite;
        ++ite;
        EXPECT_EQ(*ite, 13);
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

    TEST_F(GIVEN_object_1, WHEN_push_back_multiple_datas_THEN_data_written_to_memory)
    {
        {
            auto &data = obj->data1;
            data.push_back(1);
            data.push_back(2);

            EXPECT_EQ(data[0], 1);
            EXPECT_EQ(data[1], 2);
        }
        {
            auto &data = obj->data2;
            data.push_back(11);
            data.push_back(12);

            EXPECT_EQ(data[0], 11);
            EXPECT_EQ(data[1], 12);
        }
        {
            auto &data = obj->data3;
            data.push_back(21);
            data.push_back(22);

            EXPECT_EQ(data[0], 21);
            EXPECT_EQ(data[1], 22);
        }
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

    TEST_F(GIVEN_object_2, WHEN_push_back_multiple_datas_THEN_data_written_to_memory)
    {
        {
            auto &data = obj->data1;
            data.push_back(1);
            data.push_back(2);

            EXPECT_EQ(data[0], 1);
            EXPECT_EQ(data[1], 2);
        }
        {
            auto &data = obj->data2;
            data.push_back(11);
            data.push_back(12);

            EXPECT_EQ(data[0], 11);
            EXPECT_EQ(data[1], 12);
        }
        {
            auto &data = obj->data3;
            data.push_back(21);
            data.push_back(22);

            EXPECT_EQ(data[0], 21);
            EXPECT_EQ(data[1], 22);
        }
    }

    class GIVEN_object_1_and_2 : public GIVEN_InMemoryMappedRegion
    {
    protected:
        void SetUp() override
        {
            GIVEN_InMemoryMappedRegion::SetUp();
            obj1 = std::make_unique<mapped_object::MappedObject>(0);
            obj2 = std::make_unique<mapped_object::MappedObject>(1);
        }
        std::unique_ptr<mapped_object::MappedObject> obj1;
        std::unique_ptr<mapped_object::MappedObject> obj2;
    };

    TEST_F(GIVEN_object_1_and_2, WHEN_push_back_multiple_datas_THEN_data_written_to_memory)
    {
        {
            auto &data = obj1->data1;
            data.push_back(1);
            data.push_back(2);

            EXPECT_EQ(data[0], 1);
            EXPECT_EQ(data[1], 2);
        }
        {
            auto &data = obj1->data2;
            data.push_back(11);
            data.push_back(12);

            EXPECT_EQ(data[0], 11);
            EXPECT_EQ(data[1], 12);
        }
        {
            auto &data = obj1->data3;
            data.push_back(21);
            data.push_back(22);

            EXPECT_EQ(data[0], 21);
            EXPECT_EQ(data[1], 22);
        }
        {
            auto &data = obj2->data1;
            data.push_back(91);
            data.push_back(92);

            EXPECT_EQ(data[0], 91);
            EXPECT_EQ(data[1], 92);
        }
        {
            auto &data = obj2->data2;
            data.push_back(91);
            data.push_back(92);

            EXPECT_EQ(data[0], 91);
            EXPECT_EQ(data[1], 92);
        }
        {
            auto &data = obj2->data3;
            data.push_back(91);
            data.push_back(92);

            EXPECT_EQ(data[0], 91);
            EXPECT_EQ(data[1], 92);
        }
    }

    // partial mock to check the nb of call to flush_and_grow only.
    class MockInMemoryMappedRegion : public InMemoryMappedRegion
    {
    public:
        MockInMemoryMappedRegion(size_t initial_size)
            : InMemoryMappedRegion(initial_size)
        {
            // Delegate to real implementation by default
            ON_CALL(*this, flush_and_grow())
                .WillByDefault([this]()
                               { InMemoryMappedRegion::flush_and_grow(); });
        }

        MOCK_METHOD(void, flush_and_grow, (), (override));
    };

    class GIVEN_MockInMemoryMappedRegion : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            offset::config::window_size = 2;
            offset::config::nb_objects = 2;
            const auto window_size_bytes = mapped_object::MappedObject::get_total_data_size_jump() * offset::config::window_size * offset::config::nb_objects;
            region = new MockInMemoryMappedRegion(window_size_bytes);
            mapped_object::MappedObject::set_mapped_region(region);
            base = static_cast<char *>(region->get_address());
        }

        void TearDown() override
        {
            delete region;
            mapped_object::MappedObject::set_mapped_region(nullptr);
        }

        MockInMemoryMappedRegion *region;
        char *base;
    };

    class GIVEN_object_1_mock : public GIVEN_MockInMemoryMappedRegion
    {
    protected:
        void SetUp() override
        {
            GIVEN_MockInMemoryMappedRegion::SetUp();
            obj = std::make_unique<mapped_object::MappedObject>(0);
        }
        std::unique_ptr<mapped_object::MappedObject> obj;
    };

    class GIVEN_object_1_data_1_mock : public GIVEN_object_1_mock
    {
    protected:
        void SetUp() override
        {
            GIVEN_object_1_mock::SetUp();
            data = &obj->data1;
        }

        using data_t = decltype(obj->data1)::underlying_type;
        decltype(obj->data1) *data;
    };

    TEST_F(GIVEN_object_1_data_1_mock, WHEN_push_back_thrice_THEN_flush_and_grow_called)
    {
        EXPECT_CALL(*region, flush_and_grow())
            .Times(testing::Exactly(1));

        // Trigger condition that should call flush_and_grow
        for (size_t i = 0; i < offset::config::window_size + 1; ++i)
        {
            data->push_back(i);
        }

        EXPECT_EQ((*data)[0], 0);
        EXPECT_EQ((*data)[1], 1);
        EXPECT_EQ((*data)[2], 2);
    }

    class GIVEN_object_1_multiple_data_mock : public GIVEN_object_1_mock
    {
    protected:
        void SetUp() override
        {
            GIVEN_object_1_mock::SetUp();
        }
    };

    TEST_F(GIVEN_object_1_multiple_data_mock, WHEN_push_back_multiple_data_THEN_flush_and_grow_called_once)
    {
        EXPECT_CALL(*region, flush_and_grow())
            .Times(testing::Exactly(1));

        {
            int diff = 10;
            auto &data = obj->data1;

            for (size_t i = 0; i < offset::config::window_size; ++i)
            {
                data.push_back(i + diff);
            }
            EXPECT_EQ(data[0], 0 + diff);
            EXPECT_EQ(data[1], 1 + diff);
        }

        // Trigger condition that should call flush_and_grow
        {
            int diff = 0;
            auto &data = obj->data2;

            for (size_t i = 0; i < offset::config::window_size + 1; ++i)
            {
                data.push_back(i + diff);
            }
            EXPECT_EQ(data[0], 0 + diff);
            EXPECT_EQ(data[1], 1 + diff);
            EXPECT_EQ(data[2], 2 + diff);
        }

        // now these should not trigger grow since the window has already grown
        obj->data1.push_back(12);
        EXPECT_EQ(obj->data1[2], 12);

        {
            int diff = 20;
            auto &data = obj->data3;

            for (size_t i = 0; i < offset::config::window_size + 2; ++i)
            {
                data.push_back(i + diff);
            }
            EXPECT_EQ(data[0], 0 + diff);
            EXPECT_EQ(data[1], 1 + diff);
            EXPECT_EQ(data[2], 2 + diff);
            EXPECT_EQ(data[3], 3 + diff);
        }
    }

    class GIVEN_object_2_mock : public GIVEN_MockInMemoryMappedRegion
    {
    protected:
        void SetUp() override
        {
            GIVEN_MockInMemoryMappedRegion::SetUp();
            obj = std::make_unique<mapped_object::MappedObject>(1);
        }
        std::unique_ptr<mapped_object::MappedObject> obj;
    };

    class GIVEN_object_2_data_1_mock : public GIVEN_object_2_mock
    {
    protected:
        void SetUp() override
        {
            GIVEN_object_2_mock::SetUp();
            data = &obj->data1;
        }

        using data_t = decltype(obj->data1)::underlying_type;
        decltype(obj->data1) *data;
    };

    TEST_F(GIVEN_object_2_data_1_mock, WHEN_push_back_thrice_THEN_flush_and_grow_called)
    {
        EXPECT_CALL(*region, flush_and_grow())
            .Times(testing::Exactly(1));

        // Trigger condition that should call flush_and_grow
        for (size_t i = 0; i < offset::config::window_size + 1; ++i)
        {
            data->push_back(i);
        }

        EXPECT_EQ((*data)[0], 0);
        EXPECT_EQ((*data)[1], 1);
        EXPECT_EQ((*data)[2], 2);
    }

    class GIVEN_object_2_multiple_data_mock : public GIVEN_object_2_mock
    {
    protected:
        void SetUp() override
        {
            GIVEN_object_2_mock::SetUp();
        }
    };

    TEST_F(GIVEN_object_2_multiple_data_mock, WHEN_push_back_multiple_data_THEN_flush_and_grow_called_once)
    {
        EXPECT_CALL(*region, flush_and_grow())
            .Times(testing::Exactly(1));

        {
            int diff = 10;
            auto &data = obj->data1;

            for (size_t i = 0; i < offset::config::window_size; ++i)
            {
                data.push_back(i + diff);
            }
            EXPECT_EQ(data[0], 0 + diff);
            EXPECT_EQ(data[1], 1 + diff);
        }

        // Trigger condition that should call flush_and_grow
        {
            int diff = 0;
            auto &data = obj->data2;

            for (size_t i = 0; i < offset::config::window_size + 1; ++i)
            {
                data.push_back(i + diff);
            }
            EXPECT_EQ(data[0], 0 + diff);
            EXPECT_EQ(data[1], 1 + diff);
            EXPECT_EQ(data[2], 2 + diff);
        }

        // now these should not trigger grow since the window has already grown
        obj->data1.push_back(12);
        EXPECT_EQ(obj->data1[2], 12);

        {
            int diff = 20;
            auto &data = obj->data3;

            for (size_t i = 0; i < offset::config::window_size + 2; ++i)
            {
                data.push_back(i + diff);
            }
            EXPECT_EQ(data[0], 0 + diff);
            EXPECT_EQ(data[1], 1 + diff);
            EXPECT_EQ(data[2], 2 + diff);
            EXPECT_EQ(data[3], 3 + diff);
        }
    }

    TEST_F(GIVEN_object_2_multiple_data_mock, WHEN_push_back_double_window_plus_one_THEN_flush_and_grow_called_twice)
    {
        EXPECT_CALL(*region, flush_and_grow())
            .Times(testing::Exactly(2));

        {
            int diff = 0;
            auto &data = obj->data2;

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
            auto &data = obj->data3;

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

    class GIVEN_object_1_and_2_mock : public GIVEN_MockInMemoryMappedRegion
    {
    protected:
        void SetUp() override
        {
            GIVEN_MockInMemoryMappedRegion::SetUp();
            obj1 = std::make_unique<mapped_object::MappedObject>(0);
            obj2 = std::make_unique<mapped_object::MappedObject>(1);
        }
        std::unique_ptr<mapped_object::MappedObject> obj1;
        std::unique_ptr<mapped_object::MappedObject> obj2;
    };

    TEST_F(GIVEN_object_1_and_2_mock, WHEN_push_back_double_window_plus_one_different_object_THEN_flush_and_grow_called_twice)
    {
        EXPECT_CALL(*region, flush_and_grow())
            .Times(testing::Exactly(2));

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

    class GIVEN_object_1_and_2_in_memory_region : public GIVEN_InMemoryMappedRegion
    {
    protected:
        void SetUp() override
        {
            GIVEN_InMemoryMappedRegion::SetUp();
            obj1 = std::make_unique<mapped_object::MappedObject>(0);
            obj2 = std::make_unique<mapped_object::MappedObject>(1);
        }
        std::unique_ptr<mapped_object::MappedObject> obj1;
        std::unique_ptr<mapped_object::MappedObject> obj2;
    };

    TEST_F(GIVEN_object_1_and_2_in_memory_region, WHEN_push_back_double_window_plus_one_different_object_THEN_memory_data_written_to_memory)
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

    TEST_F(GIVEN_object_1_and_2_in_memory_region, WHEN_push_back_double_window_plus_one_different_object_THEN_memory_data_written_to_memory_and_iterator_works)
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

            {
                int i = 0;
                for (auto &value : data)
                {
                    EXPECT_EQ(value, i++ + diff);
                }
            }

            {
                int i = 0;
                for (const auto &value : data)
                {
                    EXPECT_EQ(value, i++ + diff);
                }
            }
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

            {
                int i = 0;
                for (auto &value : data)
                {
                    EXPECT_EQ(value, i++ + diff);
                }
            }

            {
                int i = 0;
                for (const auto &value : data)
                {
                    EXPECT_EQ(value, i++ + diff);
                }
            }
        }
    }

    class GIVEN_data_int8 : public GIVEN_object_1_data_3
    {
    };

    TEST_F(GIVEN_data_int8, WHEN_push_back_value_too_large_THEN_throws_overflow_error)
    {
        EXPECT_THROW(data->push_back(200), std::overflow_error);
        EXPECT_THROW(data->push_back(-129), std::overflow_error);
    }
}