#include <gtest/gtest.h>

namespace sample_tests
{
    TEST(SampleTest, Addition)
    {
        EXPECT_EQ(1 + 1, 2);
    }

    TEST(SampleTest, Subtraction)
    {
        EXPECT_EQ(5 - 3, 2);
    }
}