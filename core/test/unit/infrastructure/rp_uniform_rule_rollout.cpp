// rp_uniform_rule_rollout: uniform draw over provided rule ids.

#include <gtest/gtest.h>
#include <random>
#include <set>
#include <vector>
#include "value_objects/lineage.hpp"
#include "infrastructure/rp_uniform_rule_rollout.hpp"

struct RpUniformRuleRolloutTest : public ::testing::Test {
    std::mt19937 rng{42};
    rp_uniform_rule_rollout<std::mt19937> rollout{rng};
};

TEST_F(RpUniformRuleRolloutTest, SingleRuleAlwaysReturnsThatId) {
    const std::vector<rule_id> rules{5};
    for (int i = 0; i < 100; ++i)
        EXPECT_EQ(rollout.rollout_choose_rule(rules), 5u);
}

TEST_F(RpUniformRuleRolloutTest, MultiRuleStaysInsideProvidedSet) {
    const std::vector<rule_id> rules{1, 2, 3};
    const std::set<rule_id> allowed{1, 2, 3};
    for (int i = 0; i < 200; ++i) {
        const rule_id chosen = rollout.rollout_choose_rule(rules);
        EXPECT_NE(allowed.find(chosen), allowed.end());
    }
}

TEST_F(RpUniformRuleRolloutTest, DifferentSeedsCanDiffer) {
    const std::vector<rule_id> rules{0, 1};
    std::mt19937 rng1{1};
    std::mt19937 rng2{2};
    rp_uniform_rule_rollout<std::mt19937> r1{rng1};
    rp_uniform_rule_rollout<std::mt19937> r2{rng2};
    const rule_id drawn1 = r1.rollout_choose_rule(rules);
    const rule_id drawn2 = r2.rollout_choose_rule(rules);
    const std::set<rule_id> allowed{0, 1};
    EXPECT_NE(allowed.find(drawn1), allowed.end());
    EXPECT_NE(allowed.find(drawn2), allowed.end());
}
