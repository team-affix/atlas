#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_smallest_goal_chooser.hpp"
#include "pud_test_node.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_goal_list.hpp"

using ::testing::NiceMock;
using ::testing::ReturnRef;

namespace {

struct MockGetNodeGoals {
    MOCK_METHOD(const pud_goal_list&, get, (pud_global_node_id), (const));
};

struct PudSmallestGoalChooserTest : public ::testing::Test {
    NiceMock<MockGetNodeGoals> goals;
    pud_smallest_goal_chooser<NiceMock<MockGetNodeGoals>> chooser{goals};
    expr goal{.content = expr::var{.index = 0}};
    pud_goal_list leaf_goals{
        .goals = immer::map<pud_body_goal_id, const expr*>{}.set(7, &goal).set(3, &goal).set(5, &goal),
        .next_id = 8};
};

}

TEST_F(PudSmallestGoalChooserTest, TheSmallestOpenGoalIdIsChosen) {
    ON_CALL(goals, get(pud_test_node(1))).WillByDefault(ReturnRef(leaf_goals));
    EXPECT_EQ(chooser.choose(pud_test_node(1)), 3u);
}
