#include <set>
#include <stdexcept>
#include <gtest/gtest.h>
#include "infrastructure/pud_oldest_rule_chooser.hpp"

struct PudOldestRuleChooserTest : public ::testing::Test {
    pud_oldest_rule_chooser chooser;
};

TEST_F(PudOldestRuleChooserTest, OnlyLeafIsChosen) {
    EXPECT_EQ(chooser.choose_rule({7}), 7u);
}

TEST_F(PudOldestRuleChooserTest, OldestLeafIsChosen) {
    EXPECT_EQ(chooser.choose_rule({12, 4, 9}), 4u);
}

TEST_F(PudOldestRuleChooserTest, NoLeavesIsAPreconditionViolation) {
    EXPECT_THROW(chooser.choose_rule({}), std::logic_error);
}
