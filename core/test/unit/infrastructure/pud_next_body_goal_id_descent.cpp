#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "functor_fixture.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/pud_next_body_goal_id_descent.hpp"

using ::testing::NiceMock;
using ::testing::ReturnRef;

namespace {

struct MockGetNodeAddedBodyGoals {
    MOCK_METHOD((const std::vector<const expr*>&), get, (pud_node_id));
};

using test_descent_t = pud_next_body_goal_id_descent<MockGetNodeAddedBodyGoals>;

struct PudNextBodyGoalIdDescentTest : public ::testing::Test {
    test_functors functors;
    expr_pool exprs;
    NiceMock<MockGetNodeAddedBodyGoals> get_body_goals;
    pud_node_id root = 0;
    pud_node_id child = 2;
    pud_node_id sibling = 3;
    std::vector<const expr*> no_goals{};

    const expr* goal(const char* name) {
        return exprs.make_functor(functors.id(name), {});
    }
};

TEST_F(PudNextBodyGoalIdDescentTest, RootCountsRootBodyGoals) {
    const std::vector<const expr*> goals{goal("g0"), goal("g1")};
    EXPECT_CALL(get_body_goals, get(root)).WillOnce(ReturnRef(goals));
    test_descent_t descent{get_body_goals, root};
    EXPECT_EQ(descent.next_body_goal_id(), 2u);
}

TEST_F(PudNextBodyGoalIdDescentTest, RootWithNoGoalsStartsAtZero) {
    EXPECT_CALL(get_body_goals, get(root)).WillOnce(ReturnRef(no_goals));
    test_descent_t descent{get_body_goals, root};
    EXPECT_EQ(descent.next_body_goal_id(), 0u);
}

TEST_F(PudNextBodyGoalIdDescentTest, DescendAdvancesByChildBodyGoals) {
    const std::vector<const expr*> root_goals{goal("r0"), goal("r1"), goal("r2"), goal("r3"), goal("r4")};
    const std::vector<const expr*> child_goals{goal("c0"), goal("c1"), goal("c2")};
    EXPECT_CALL(get_body_goals, get(root)).WillOnce(ReturnRef(root_goals));
    EXPECT_CALL(get_body_goals, get(child)).WillOnce(ReturnRef(child_goals));
    test_descent_t at_root{get_body_goals, root};
    EXPECT_EQ(at_root.descend(child).next_body_goal_id(), 8u);
}

TEST_F(PudNextBodyGoalIdDescentTest, ChildWithNoGoalsKeepsTheId) {
    const std::vector<const expr*> root_goals{goal("r0")};
    EXPECT_CALL(get_body_goals, get(root)).WillOnce(ReturnRef(root_goals));
    EXPECT_CALL(get_body_goals, get(child)).WillOnce(ReturnRef(no_goals));
    test_descent_t at_root{get_body_goals, root};
    EXPECT_EQ(at_root.descend(child).next_body_goal_id(), 1u);
}

TEST_F(PudNextBodyGoalIdDescentTest, DescendLeavesParentUntouched) {
    const std::vector<const expr*> root_goals{goal("r0")};
    const std::vector<const expr*> child_goals{goal("c0")};
    EXPECT_CALL(get_body_goals, get(root)).WillOnce(ReturnRef(root_goals));
    EXPECT_CALL(get_body_goals, get(child)).WillOnce(ReturnRef(child_goals));
    test_descent_t at_root{get_body_goals, root};
    at_root.descend(child);
    EXPECT_EQ(at_root.next_body_goal_id(), 1u);
}

TEST_F(PudNextBodyGoalIdDescentTest, SiblingsDescendIndependently) {
    const std::vector<const expr*> root_goals{goal("r0")};
    const std::vector<const expr*> child_goals{goal("c0")};
    const std::vector<const expr*> sibling_goals{goal("s0"), goal("s1"), goal("s2")};
    EXPECT_CALL(get_body_goals, get(root)).WillOnce(ReturnRef(root_goals));
    EXPECT_CALL(get_body_goals, get(child)).WillOnce(ReturnRef(child_goals));
    EXPECT_CALL(get_body_goals, get(sibling)).WillOnce(ReturnRef(sibling_goals));
    test_descent_t at_root{get_body_goals, root};
    EXPECT_EQ(at_root.descend(child).next_body_goal_id(), 2u);
    EXPECT_EQ(at_root.descend(sibling).next_body_goal_id(), 4u);
}

}  // namespace
