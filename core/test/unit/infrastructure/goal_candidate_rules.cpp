// Goal–candidate rule index: link/unlink per goal, constrain on resolution prunes parent
// bucket, and get returns empty rule set for unknown goals.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../../../core/hpp/infrastructure/goal_candidate_rules.hpp"

using ::testing::IsEmpty;
using ::testing::UnorderedElementsAre;

namespace {

std::vector<rule_id> collect_rule_ids(i_rule_set& rs) {
    std::vector<rule_id> out;
    auto sm = rs.iterate();
    while (!sm.done()) {
        if (auto r = sm.resume())
            out.push_back(*r);
    }
    return out;
}

}  // namespace

struct GoalCandidateRulesTest : public ::testing::Test {
    static constexpr rule_id kRule0 = 0;
    static constexpr rule_id kRule1 = 1;

    goal_candidate_rules index;
    expr goal_e{expr::var{0}};
    expr head0{expr::var{1}};
    rule r0{&head0, {}};
    goal_lineage gl{nullptr, 0};
    resolution_lineage rl{&gl, kRule0};
};

TEST_F(GoalCandidateRulesTest, UnknownGoalReturnsEmptySet) {
    EXPECT_THAT(collect_rule_ids(index.get(&gl)), IsEmpty());
}

TEST_F(GoalCandidateRulesTest, LinkAddsRuleToGoal) {
    index.link_goal_candidate(&gl, kRule0);
    EXPECT_THAT(collect_rule_ids(index.get(&gl)), UnorderedElementsAre(kRule0));
}

TEST_F(GoalCandidateRulesTest, UnlinkRemovesRule) {
    index.link_goal_candidate(&gl, kRule0);
    index.link_goal_candidate(&gl, kRule1);
    index.unlink_goal_candidate(&gl, kRule0);
    EXPECT_THAT(collect_rule_ids(index.get(&gl)), UnorderedElementsAre(kRule1));
}

TEST_F(GoalCandidateRulesTest, ConstrainRemovesParentGoalBucket) {
    index.link_goal_candidate(&gl, kRule0);
    index.constrain_goal_candidate_rules(&rl);
    EXPECT_THAT(collect_rule_ids(index.get(&gl)), IsEmpty());
}

TEST_F(GoalCandidateRulesTest, ConstGetReturnsEmptyForUnknownGoal) {
    const goal_candidate_rules& cref = index;
    EXPECT_EQ(cref.get(&gl).size(), 0u);
}
