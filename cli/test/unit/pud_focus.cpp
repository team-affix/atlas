#include <stdexcept>
#include <string>
#include <gtest/gtest.h>
#include "infrastructure/pud_focus.hpp"

TEST(PudFocusTest, ParsesHexModuleAndDecimalNode) {
    const pud_global_node_id focus = pud_focus{}.parse("a1b:12");
    EXPECT_EQ(focus.module, 0xa1bu);
    EXPECT_EQ(focus.node, 12u);
}

TEST(PudFocusTest, RejectsAMissingSeparator) {
    EXPECT_THROW(pud_focus{}.parse("a1b"), std::runtime_error);
}

TEST(PudFocusTest, RejectsANonDecimalNode) {
    EXPECT_THROW(pud_focus{}.parse("a1b:c"), std::runtime_error);
}
