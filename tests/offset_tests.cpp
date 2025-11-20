#include <gtest/gtest.h>
#include "OffsetCalculator.hpp"

namespace offset_tests
{
    class OffsetCalculator : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            // Set up test configuration
            offset::config::window_size = 2;
            offset::config::nb_objects = 2;

            // lets say I have a window_size of 2, 2 nb_objects and 2 windows that I want to grow to.
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
        }

        using T1 = int32_t;
        using T2 = int16_t;
        using T3 = int8_t;
        static constexpr size_t total_data_size_jump = sizeof(T1) + sizeof(T2) + sizeof(T3); // 4 + 2 + 1

        offset::OffsetCalculator<T1, total_data_size_jump, 0> calc1;
        offset::OffsetCalculator<T2, total_data_size_jump, sizeof(T1)> calc2;
        offset::OffsetCalculator<T3, total_data_size_jump, sizeof(T1) + sizeof(T2)> calc3;

        const size_t element_index = 0;
        const size_t window_index = 0;
    };

    class GIVEN_object1 : public OffsetCalculator
    {
    protected:
        const size_t object_index = 0;
    };

    TEST_F(GIVEN_object1, GIVEN_data1_WHEN_compute_offset_THEN_return_correct_offset)
    {
        EXPECT_EQ(calc1.compute_offset(object_index, element_index, window_index), 0);
    }

    TEST_F(GIVEN_object1, GIVEN_data2_WHEN_compute_offset_THEN_return_correct_offset)
    {
        EXPECT_EQ(calc2.compute_offset(object_index, element_index, window_index), 8);
    }

    TEST_F(GIVEN_object1, GIVEN_data3_WHEN_compute_offset_THEN_return_correct_offset)
    {
        EXPECT_EQ(calc3.compute_offset(object_index, element_index, window_index), 12);
    }

    class GIVEN_object2 : public OffsetCalculator
    {
    protected:
        const size_t object_index = 1;
    };

    TEST_F(GIVEN_object2, GIVEN_data1_WHEN_compute_offset_THEN_return_correct_offset)
    {
        EXPECT_EQ(calc1.compute_offset(object_index, element_index, window_index), 14 + 0);
    }

    TEST_F(GIVEN_object2, GIVEN_data2_WHEN_compute_offset_THEN_return_correct_offset)
    {
        EXPECT_EQ(calc2.compute_offset(object_index, element_index, window_index), 14 + 8);
    }

    TEST_F(GIVEN_object2, GIVEN_data3_WHEN_compute_offset_THEN_return_correct_offset)
    {
        EXPECT_EQ(calc3.compute_offset(object_index, element_index, window_index), 14 + 12);
    }

    class GIVEN_element_2 : virtual public OffsetCalculator
    {
    protected:
        const size_t element_index = 1;
    };

    class GIVEN_object1_element_2 : public GIVEN_element_2
    {
    protected:
        const size_t object_index = 0;
    };

    TEST_F(GIVEN_object1_element_2, GIVEN_data1_WHEN_compute_offset_THEN_return_correct_offset)
    {
        EXPECT_EQ(calc1.compute_offset(object_index, element_index, window_index), 0 + 1 * sizeof(T1));
    }

    TEST_F(GIVEN_object1_element_2, GIVEN_data2_WHEN_compute_offset_THEN_return_correct_offset)
    {
        EXPECT_EQ(calc2.compute_offset(object_index, element_index, window_index), 8 + 1 * sizeof(T2));
    }

    TEST_F(GIVEN_object1_element_2, GIVEN_data3_WHEN_compute_offset_THEN_return_correct_offset)
    {
        EXPECT_EQ(calc3.compute_offset(object_index, element_index, window_index), 12 + 1 * sizeof(T3));
    }

    class GIVEN_object2_element_2 : public GIVEN_element_2
    {
    protected:
        const size_t object_index = 1;
    };

    TEST_F(GIVEN_object2_element_2, GIVEN_data1_WHEN_compute_offset_THEN_return_correct_offset)
    {
        EXPECT_EQ(calc1.compute_offset(object_index, element_index, window_index), 14 + 0 + 1 * sizeof(T1));
    }

    TEST_F(GIVEN_object2_element_2, GIVEN_data2_WHEN_compute_offset_THEN_return_correct_offset)
    {
        EXPECT_EQ(calc2.compute_offset(object_index, element_index, window_index), 14 + 8 + 1 * sizeof(T2));
    }

    TEST_F(GIVEN_object2_element_2, GIVEN_data3_WHEN_compute_offset_THEN_return_correct_offset)
    {
        EXPECT_EQ(calc3.compute_offset(object_index, element_index, window_index), 14 + 12 + 1 * sizeof(T3));
    }

    class Given_window_2 : virtual public OffsetCalculator
    {
    protected:
        const size_t window_jump = 28;
        const size_t window_index = 1;
    };

    class GIVEN_object1_element_1_window_2 : public Given_window_2
    {
    protected:
        const size_t object_index = 0;
    };

    TEST_F(GIVEN_object1_element_1_window_2, GIVEN_data1_WHEN_compute_offset_THEN_return_correct_offset)
    {
        EXPECT_EQ(calc1.compute_offset(object_index, element_index, window_index), window_jump + 0 + element_index * sizeof(T1));
    }

    TEST_F(GIVEN_object1_element_1_window_2, GIVEN_data2_WHEN_compute_offset_THEN_return_correct_offset)
    {
        EXPECT_EQ(calc2.compute_offset(object_index, element_index, window_index), window_jump + 8 + element_index * sizeof(T2));
    }

    TEST_F(GIVEN_object1_element_1_window_2, GIVEN_data3_WHEN_compute_offset_THEN_return_correct_offset)
    {
        EXPECT_EQ(calc3.compute_offset(object_index, element_index, window_index), window_jump + 12 + element_index * sizeof(T3));
    }

    class GIVEN_object2_element_1_window_2 : public Given_window_2
    {
    protected:
        const size_t object_index = 1;
    };

    TEST_F(GIVEN_object2_element_1_window_2, GIVEN_data1_WHEN_compute_offset_THEN_return_correct_offset)
    {
        EXPECT_EQ(calc1.compute_offset(object_index, element_index, window_index), window_jump + 14 + 0 + element_index * sizeof(T1));
    }

    TEST_F(GIVEN_object2_element_1_window_2, GIVEN_data2_WHEN_compute_offset_THEN_return_correct_offset)
    {
        EXPECT_EQ(calc2.compute_offset(object_index, element_index, window_index), window_jump + 14 + 8 + element_index * sizeof(T2));
    }

    TEST_F(GIVEN_object2_element_1_window_2, GIVEN_data3_WHEN_compute_offset_THEN_return_correct_offset)
    {
        EXPECT_EQ(calc3.compute_offset(object_index, element_index, window_index), window_jump + 14 + 12 + element_index * sizeof(T3));
    }

    class GIVEN_element_2_window_2 : public Given_window_2, public GIVEN_element_2
    {
    };

    class GIVEN_object1_element_2_window_2 : public GIVEN_element_2_window_2
    {
    protected:
        const size_t object_index = 0;
    };

    TEST_F(GIVEN_object1_element_2_window_2, GIVEN_data1_WHEN_compute_offset_THEN_return_correct_offset)
    {
        EXPECT_EQ(calc1.compute_offset(object_index,element_index, window_index), window_jump + 0 + element_index * sizeof(T1));
    }

    TEST_F(GIVEN_object1_element_2_window_2, GIVEN_data2_WHEN_compute_offset_THEN_return_correct_offset)
    {
        EXPECT_EQ(calc2.compute_offset(object_index, element_index, window_index), window_jump + 8 + element_index * sizeof(T2));
    }

    TEST_F(GIVEN_object1_element_2_window_2, GIVEN_data3_WHEN_compute_offset_THEN_return_correct_offset)
    {
        EXPECT_EQ(calc3.compute_offset(object_index, element_index, window_index), window_jump + 12 + element_index * sizeof(T3));
    }

    class GIVEN_object2_element_2_window_2 : public GIVEN_element_2_window_2
    {
    protected:
        const size_t object_index = 1;
    };

    TEST_F(GIVEN_object2_element_2_window_2, GIVEN_data1_WHEN_compute_offset_THEN_return_correct_offset)
    {
        EXPECT_EQ(calc1.compute_offset(object_index, element_index, window_index), window_jump + 14 + 0 + element_index * sizeof(T1));
    }

    TEST_F(GIVEN_object2_element_2_window_2, GIVEN_data2_WHEN_compute_offset_THEN_return_correct_offset)
    {
        EXPECT_EQ(calc2.compute_offset(object_index, element_index, window_index), window_jump + 14 + 8 + element_index * sizeof(T2));
    }

    TEST_F(GIVEN_object2_element_2_window_2, GIVEN_data3_WHEN_compute_offset_THEN_return_correct_offset)
    {
        EXPECT_EQ(calc3.compute_offset(object_index, element_index, window_index), window_jump + 14 + 12 + element_index * sizeof(T3));
    }
}