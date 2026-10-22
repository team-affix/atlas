#include <map>
#include <random>
#include <set>
#include <stdexcept>
#include <gtest/gtest.h>
#include "infrastructure/pud_random_rule_chooser.hpp"

struct PudRandomRuleChooserTest : public ::testing::Test {
    std::mt19937 rng{7};
    pud_random_rule_chooser chooser{rng};
    std::set<pud_node_id> leaves{3, 8, 21, 40};
};

TEST_F(PudRandomRuleChooserTest, OnlyLeafIsAlwaysChosen) {
    for (int draw = 0; draw < 50; ++draw)
        EXPECT_EQ(chooser.choose_rule({9}), 9u);
}

TEST_F(PudRandomRuleChooserTest, ChosenLeafIsAlwaysOneOfTheOffered) {
    for (int draw = 0; draw < 200; ++draw)
        EXPECT_TRUE(leaves.contains(chooser.choose_rule(leaves)));
}

TEST_F(PudRandomRuleChooserTest, ChoicesAreRoughlyUniform) {
    std::map<pud_node_id, int> counts;
    for (int draw = 0; draw < 4000; ++draw)
        ++counts[chooser.choose_rule(leaves)];
    EXPECT_EQ(counts.size(), leaves.size());
    for (const auto& [leaf, count] : counts)
        EXPECT_GT(count, 800) << leaf;
}

TEST_F(PudRandomRuleChooserTest, SameSeedGivesTheSameChoices) {
    std::mt19937 other_rng{7};
    pud_random_rule_chooser other{other_rng};
    for (int draw = 0; draw < 100; ++draw)
        EXPECT_EQ(chooser.choose_rule(leaves), other.choose_rule(leaves));
}

TEST_F(PudRandomRuleChooserTest, NoLeavesIsAPreconditionViolation) {
    EXPECT_THROW(chooser.choose_rule({}), std::logic_error);
}
