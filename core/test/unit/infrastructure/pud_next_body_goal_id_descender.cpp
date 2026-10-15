#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "functor_fixture.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/pud_next_body_goal_id_descender.hpp"

namespace {

struct MockGetNodeAddedBodyGoals {
    MOCK_METHOD((const std::vector<const expr*>&), get, (pud_node_id));
};

struct PudNextBodyGoalIdDescenderTest : public ::testing::Test {
    test_functors functors;
    expr_pool exprs;
    ::testing::NiceMock<MockGetNodeAddedBodyGoals> get_body_goals;
    pud_next_body_goal_id_descender<MockGetNodeAddedBodyGoals> descender{get_body_goals};

    const expr* goal(const char* name) {
        return exprs.make_functor(functors.id(name), {});
    }
};

TEST_F(PudNextBodyGoalIdDescenderTest, RootDescentCountsRootBodyGoals) {
    const pud_node_id root = 0;
    const std::vector<const expr*> goals{goal("g0"), goal("g1")};
    EXPECT_CALL(get_body_goals, get(root)).WillOnce(::testing::ReturnRef(goals));
    pud_next_body_goal_id_descent descent = descender.root_descent(root);
    EXPECT_EQ(descent.next_body_goal_id, 2u);
}

TEST_F(PudNextBodyGoalIdDescenderTest, DescendAdvancesByChildBodyGoals) {
    const pud_node_id child = 2;
    pud_next_body_goal_id_descent at_parent{.next_body_goal_id = 5};
    const std::vector<const expr*> child_goals{goal("c0"), goal("c1"), goal("c2")};
    EXPECT_CALL(get_body_goals, get(child)).WillOnce(::testing::ReturnRef(child_goals));
    pud_next_body_goal_id_descent at_child = descender.descend(at_parent, child);
    EXPECT_EQ(at_child.next_body_goal_id, 8u);
}

}
