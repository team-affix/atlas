#include <cstdint>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_unfold_allower.hpp"
#include "value_objects/pud_goal_list.hpp"

using ::testing::Return;
using ::testing::ReturnRef;

namespace {

pud_global_node_id node() {
    return pud_global_node_id{.module = 1, .node = 4};
}

pud_goal_list goals_of(size_t goal_count) {
    pud_goal_list goals;
    for (size_t index = 0; index < goal_count; ++index)
        goals.goals = goals.goals.set(static_cast<pud_body_goal_id>(index), nullptr);
    return goals;
}

struct MockCountNodeDepth {
    MOCK_METHOD(size_t, count, (pud_global_node_id));
};

struct MockGetNodeGoals {
    MOCK_METHOD(const pud_goal_list&, get, (pud_global_node_id));
};

struct PudUnfoldAllowerTest : public ::testing::Test {
    MockCountNodeDepth depths;
    MockGetNodeGoals goals;
    pud_unfold_allower<MockCountNodeDepth, MockGetNodeGoals> allower{depths, goals, 4};
    pud_goal_list at_cap = goals_of(64);
    pud_goal_list over_cap = goals_of(65);
};

}

TEST_F(PudUnfoldAllowerTest, ANodeUnderBothCapsIsAllowed) {
    EXPECT_CALL(depths, count(node())).WillOnce(Return(3u));
    EXPECT_CALL(goals, get(node())).WillOnce(ReturnRef(at_cap));
    EXPECT_TRUE(allower.allow(node()));
}

TEST_F(PudUnfoldAllowerTest, ANodeAtTheDepthCapIsRefused) {
    EXPECT_CALL(depths, count(node())).WillOnce(Return(4u));
    EXPECT_CALL(goals, get).Times(0);
    EXPECT_FALSE(allower.allow(node()));
}

TEST_F(PudUnfoldAllowerTest, ANodeWithTooManyOpenGoalsIsRefused) {
    EXPECT_CALL(depths, count(node())).WillOnce(Return(0u));
    EXPECT_CALL(goals, get(node())).WillOnce(ReturnRef(over_cap));
    EXPECT_FALSE(allower.allow(node()));
}

TEST_F(PudUnfoldAllowerTest, SixtyFourOpenGoalsAreStillAllowed) {
    EXPECT_CALL(depths, count(node())).WillOnce(Return(0u));
    EXPECT_CALL(goals, get(node())).WillOnce(ReturnRef(at_cap));
    EXPECT_TRUE(allower.allow(node()));
}
