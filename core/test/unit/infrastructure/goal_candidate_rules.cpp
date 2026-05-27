// Goal–candidate rule index: link/unlink per goal, constrain on resolution prunes parent
// bucket, and get returns empty rule set for unknown goals.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../../../core/hpp/infrastructure/goal_candidate_rules.hpp"

using ::testing::IsEmpty;
using ::testing::SizeIs;
using ::testing::UnorderedElementsAre;

namespace {

std::vector<const rule*> collect_rules(i_rule_set& rs) {
    std::vector<const rule*> out;
    auto sm = rs.iterate();
    while (!sm.done()) {
        if (auto r = sm.resume())
            out.push_back(*r);
    }
    return out;
}

}  // namespace

struct GoalCandidateRulesTest : public ::testing::Test {
    goal_candidate_rules index;
    expr goal_e{expr::var{0}};
    expr head0{expr::var{1}};
    expr head1{expr::var{2}};
    rule r0{&head0, {}};
    rule r1{&head1, {}};
    goal_lineage gl{nullptr, &goal_e};
    resolution_lineage rl{&gl, &r0};
};

TEST_F(GoalCandidateRulesTest, UnknownGoalReturnsEmptySet) {
    EXPECT_THAT(collect_rules(index.get(&gl)), IsEmpty());
}

TEST_F(GoalCandidateRulesTest, LinkAddsRuleToGoal) {
    index.link_goal_candidate(&gl, &r0);
    EXPECT_THAT(collect_rules(index.get(&gl)), UnorderedElementsAre(&r0));
}

TEST_F(GoalCandidateRulesTest, UnlinkRemovesRule) {
    index.link_goal_candidate(&gl, &r0);
    index.link_goal_candidate(&gl, &r1);
    index.unlink_goal_candidate(&gl, &r0);
    EXPECT_THAT(collect_rules(index.get(&gl)), UnorderedElementsAre(&r1));
}

TEST_F(GoalCandidateRulesTest, ConstrainRemovesParentGoalBucket) {
    index.link_goal_candidate(&gl, &r0);
    index.constrain_goal_candidate_rules(&rl);
    EXPECT_THAT(collect_rules(index.get(&gl)), IsEmpty());
}

TEST_F(GoalCandidateRulesTest, ConstGetReturnsEmptyForUnknownGoal) {
    const goal_candidate_rules& cref = index;
    EXPECT_EQ(cref.get(&gl).size(), 0u);
}
