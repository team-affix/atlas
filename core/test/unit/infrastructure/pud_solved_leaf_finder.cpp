#include <set>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_solved_leaf_finder.hpp"
#include "value_objects/pud_goal_list.hpp"

using ::testing::Return;
using ::testing::ReturnRef;

namespace {

pud_global_node_id node(pud_module_id module, pud_node_id id) {
    return pud_global_node_id{.module = module, .node = id};
}

struct MockGetLiveNodes {
    MOCK_METHOD(const std::set<pud_global_node_id>&, live, ());
};

struct MockContainsFocusNode {
    MOCK_METHOD(bool, contains, (pud_global_node_id));
};

struct MockIsNodeRefuted {
    MOCK_METHOD(bool, is_refuted, (pud_global_node_id));
};

struct MockGetNodeGoals {
    MOCK_METHOD(const pud_goal_list&, get, (pud_global_node_id));
};

struct PudSolvedLeafFinderTest : public ::testing::Test {
    MockGetLiveNodes live_nodes;
    MockContainsFocusNode focus;
    MockIsNodeRefuted refuted;
    MockGetNodeGoals goals;
    pud_solved_leaf_finder<
        MockGetLiveNodes, MockContainsFocusNode, MockIsNodeRefuted, MockGetNodeGoals>
        finder{live_nodes, focus, refuted, goals};
    pud_goal_list empty;
    pud_goal_list open = [] {
        pud_goal_list goals;
        goals.goals = goals.goals.set(0, nullptr);
        return goals;
    }();
    std::set<pud_global_node_id> live{
        node(1, 5),
        node(1, 1),
        node(2, 0),
    };
};

}

TEST_F(PudSolvedLeafFinderTest, AnEmptyFrontierHasNoSolvedLeaf) {
    std::set<pud_global_node_id> none;
    EXPECT_CALL(live_nodes, live()).WillOnce(ReturnRef(none));
    EXPECT_FALSE(finder.find().has_value());
}

TEST_F(PudSolvedLeafFinderTest, TheSmallestSolvedFocusLeafIsReturned) {
    EXPECT_CALL(live_nodes, live()).WillOnce(ReturnRef(live));
    EXPECT_CALL(focus, contains(node(1, 1))).WillOnce(Return(false));
    EXPECT_CALL(focus, contains(node(1, 5))).WillOnce(Return(true));
    EXPECT_CALL(refuted, is_refuted(node(1, 5))).WillOnce(Return(false));
    EXPECT_CALL(goals, get(node(1, 5))).WillOnce(ReturnRef(empty));
    const std::optional<pud_global_node_id> found = finder.find();
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found.value(), node(1, 5));
}

TEST_F(PudSolvedLeafFinderTest, ALeafWithOpenGoalsIsNotSolved) {
    std::set<pud_global_node_id> only{node(1, 1)};
    EXPECT_CALL(live_nodes, live()).WillOnce(ReturnRef(only));
    EXPECT_CALL(focus, contains(node(1, 1))).WillOnce(Return(true));
    EXPECT_CALL(refuted, is_refuted(node(1, 1))).WillOnce(Return(false));
    EXPECT_CALL(goals, get(node(1, 1))).WillOnce(ReturnRef(open));
    EXPECT_FALSE(finder.find().has_value());
}
