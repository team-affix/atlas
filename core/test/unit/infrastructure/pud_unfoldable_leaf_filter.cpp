#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_unfoldable_leaf_filter.hpp"
#include "pud_test_node.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_goal_list.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockIsNodeLeaf {
    MOCK_METHOD(bool, is_leaf, (pud_global_node_id), (const));
};
struct MockIsNodeRefuted {
    MOCK_METHOD(bool, is_refuted, (pud_global_node_id), (const));
};
struct MockGetNodeGoals {
    MOCK_METHOD(const pud_goal_list&, get, (pud_global_node_id), (const));
};
struct MockAllowUnfold {
    MOCK_METHOD(bool, allow, (pud_global_node_id), (const));
};

using test_filter_t = pud_unfoldable_leaf_filter<
    NiceMock<MockIsNodeLeaf>, NiceMock<MockIsNodeRefuted>, NiceMock<MockGetNodeGoals>, NiceMock<MockAllowUnfold>>;

struct PudUnfoldableLeafFilterTest : public ::testing::Test {
    NiceMock<MockIsNodeLeaf> leaves;
    NiceMock<MockIsNodeRefuted> refuted;
    NiceMock<MockGetNodeGoals> goals;
    NiceMock<MockAllowUnfold> allower;
    test_filter_t filter{leaves, refuted, goals, allower};
    expr goal{.content = expr::var{.index = 0}};
    pud_goal_list open_goals{.goals = immer::map<pud_body_goal_id, const expr*>{}.set(0, &goal), .next_id = 1};
    pud_goal_list no_goals{.goals = {}, .next_id = 1};

    void SetUp() override {
        ON_CALL(leaves, is_leaf(_)).WillByDefault(Return(true));
        ON_CALL(refuted, is_refuted(_)).WillByDefault(Return(false));
        ON_CALL(goals, get(_)).WillByDefault(ReturnRef(open_goals));
        ON_CALL(allower, allow(_)).WillByDefault(Return(true));
    }
};

}

TEST_F(PudUnfoldableLeafFilterTest, AnUnrefutedLeafWithAnOpenGoalThatTheLimitsAllowIsUnfoldable) {
    EXPECT_TRUE(filter.is_unfoldable(pud_test_node(1)));
}

TEST_F(PudUnfoldableLeafFilterTest, AnUnfoldedNodeIsNotUnfoldable) {
    ON_CALL(leaves, is_leaf(pud_test_node(1))).WillByDefault(Return(false));
    EXPECT_FALSE(filter.is_unfoldable(pud_test_node(1)));
}

TEST_F(PudUnfoldableLeafFilterTest, ARefutedLeafIsNotUnfoldable) {
    ON_CALL(refuted, is_refuted(pud_test_node(1))).WillByDefault(Return(true));
    EXPECT_FALSE(filter.is_unfoldable(pud_test_node(1)));
}

TEST_F(PudUnfoldableLeafFilterTest, ASolvedLeafIsNotUnfoldable) {
    ON_CALL(goals, get(pud_test_node(1))).WillByDefault(ReturnRef(no_goals));
    EXPECT_FALSE(filter.is_unfoldable(pud_test_node(1)));
}

TEST_F(PudUnfoldableLeafFilterTest, ALeafTheLimitsHoldBackIsNotUnfoldable) {
    ON_CALL(allower, allow(pud_test_node(1))).WillByDefault(Return(false));
    EXPECT_FALSE(filter.is_unfoldable(pud_test_node(1)));
}
