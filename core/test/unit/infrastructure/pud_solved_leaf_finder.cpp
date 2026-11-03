#include <set>
#include <vector>
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

struct MockGetLiveFocusLeaves {
    MOCK_METHOD(const std::set<pud_global_node_id>&, live, ());
};

struct MockIsNodeRefuted {
    MOCK_METHOD(bool, is_refuted, (pud_global_node_id));
};

struct MockGetNodeGoals {
    MOCK_METHOD(const pud_goal_list&, get, (pud_global_node_id));
};

struct PudSolvedLeafFinderTest : public ::testing::Test {
    PudSolvedLeafFinderTest();
    MockGetLiveFocusLeaves focus_leaves;
    MockIsNodeRefuted refuted;
    MockGetNodeGoals goals;
    pud_solved_leaf_finder<MockGetLiveFocusLeaves, MockIsNodeRefuted, MockGetNodeGoals>
        finder{focus_leaves, refuted, goals};
    pud_goal_list empty;
    pud_goal_list open;
    std::set<pud_global_node_id> live;
};

PudSolvedLeafFinderTest::PudSolvedLeafFinderTest()
    : focus_leaves()
    , refuted()
    , goals()
    , empty()
    , open(pud_goal_list{.goals = immer::map<pud_body_goal_id, const expr*>{}.set(0, nullptr), .next_id = 1})
    , live{node(1, 5), node(1, 1), node(2, 0)} {}

}

TEST_F(PudSolvedLeafFinderTest, NoLiveFocusLeafMeansNoSolvedLeaf) {
    std::set<pud_global_node_id> none;
    EXPECT_CALL(focus_leaves, live()).WillOnce(ReturnRef(none));
    EXPECT_FALSE(finder.find().has_value());
}

TEST_F(PudSolvedLeafFinderTest, TheSmallestSolvedLeafIsReturned) {
    EXPECT_CALL(focus_leaves, live()).WillOnce(ReturnRef(live));
    EXPECT_CALL(refuted, is_refuted(node(1, 1))).WillOnce(Return(false));
    EXPECT_CALL(goals, get(node(1, 1))).WillOnce(ReturnRef(open));
    EXPECT_CALL(refuted, is_refuted(node(1, 5))).WillOnce(Return(false));
    EXPECT_CALL(goals, get(node(1, 5))).WillOnce(ReturnRef(empty));
    const std::optional<pud_global_node_id> found = finder.find();
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found.value(), node(1, 5));
}

TEST_F(PudSolvedLeafFinderTest, ALeafWithOpenGoalsIsNotSolved) {
    std::set<pud_global_node_id> only{node(1, 1)};
    EXPECT_CALL(focus_leaves, live()).WillOnce(ReturnRef(only));
    EXPECT_CALL(refuted, is_refuted(node(1, 1))).WillOnce(Return(false));
    EXPECT_CALL(goals, get(node(1, 1))).WillOnce(ReturnRef(open));
    EXPECT_FALSE(finder.find().has_value());
}

TEST_F(PudSolvedLeafFinderTest, ListingReturnsEverySolvedLeafInFrontierOrder) {
    EXPECT_CALL(focus_leaves, live()).WillOnce(ReturnRef(live));
    EXPECT_CALL(refuted, is_refuted(node(1, 1))).WillOnce(Return(false));
    EXPECT_CALL(goals, get(node(1, 1))).WillOnce(ReturnRef(empty));
    EXPECT_CALL(refuted, is_refuted(node(1, 5))).WillOnce(Return(true));
    EXPECT_CALL(refuted, is_refuted(node(2, 0))).WillOnce(Return(false));
    EXPECT_CALL(goals, get(node(2, 0))).WillOnce(ReturnRef(empty));
    EXPECT_EQ(finder.list(), (std::vector<pud_global_node_id>{node(1, 1), node(2, 0)}));
}
