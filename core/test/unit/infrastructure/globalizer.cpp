// globalizer: frame_offset + local_index arithmetic.

#include <gtest/gtest.h>
#include "infrastructure/globalizer.hpp"

struct GlobalizerTest : public ::testing::Test {
    globalizer g;
};

TEST_F(GlobalizerTest, GlobalizeZeroPair) {
    EXPECT_EQ(g.globalize(0, 0), 0u);
}

TEST_F(GlobalizerTest, GlobalizeNonzeroPair) {
    EXPECT_EQ(g.globalize(10, 3), 13u);
    EXPECT_EQ(g.globalize(100, 42), 142u);
}
