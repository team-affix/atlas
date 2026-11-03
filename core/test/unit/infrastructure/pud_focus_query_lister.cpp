#include <set>
#include <stdexcept>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_focus_query_lister.hpp"
#include "value_objects/pud_node_queries.hpp"
#include "value_objects/pud_goal_list.hpp"
#include "pud_test_node.hpp"

using ::testing::_;
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockGetLiveFocusLeaves {
    MOCK_METHOD(const std::set<pud_global_node_id>&, live, (), (const));
};
struct MockIsNodeRefuted {
    MOCK_METHOD(bool, is_refuted, (pud_global_node_id), (const));
};
struct MockGetNodeQueries {
    MOCK_METHOD(const pud_node_queries&, get, (pud_global_node_id), (const));
};
struct MockGetNodeGoals {
    MOCK_METHOD(const pud_goal_list&, get, (pud_global_node_id), (const));
};
struct MockIsQueryExhausted {
    MOCK_METHOD(bool, is_exhausted, (pud_mhq_head_id), (const));
};

using test_lister_t = pud_focus_query_lister<
    NiceMock<MockGetLiveFocusLeaves>, NiceMock<MockIsNodeRefuted>, NiceMock<MockGetNodeQueries>,
    NiceMock<MockGetNodeGoals>, NiceMock<MockIsQueryExhausted>>;

// Leaf 3 is a live focus leaf with goals 0, 1 and 2. Goals 0 and 1 hold queries 40 and 41,
// which have candidates; goal 2 holds query 42, which is exhausted.
struct PudFocusQueryListerTest : public ::testing::Test {
    NiceMock<MockGetLiveFocusLeaves> frontier;
    NiceMock<MockIsNodeRefuted> refuted;
    NiceMock<MockGetNodeQueries> node_queries;
    NiceMock<MockGetNodeGoals> goals;
    NiceMock<MockIsQueryExhausted> exhausted;
    test_lister_t lister{frontier, refuted, node_queries, goals, exhausted};
    std::set<pud_global_node_id> live{pud_test_node(3)};
    expr goal_expr{expr::functor{1, {}}};
    pud_goal_list leaf_goals{
        .goals = immer::map<pud_body_goal_id, const expr*>{}.set(2, &goal_expr).set(0, &goal_expr).set(1, &goal_expr),
        .next_id = 3};
    pud_node_queries leaf_queries{pud_node_queries{}.set(0, 40).set(1, 41).set(2, 42)};

    void SetUp() override {
        ON_CALL(frontier, live()).WillByDefault(ReturnRef(live));
        ON_CALL(refuted, is_refuted(_)).WillByDefault(Return(false));
        ON_CALL(node_queries, get(pud_test_node(3))).WillByDefault(ReturnRef(leaf_queries));
        ON_CALL(goals, get(pud_test_node(3))).WillByDefault(ReturnRef(leaf_goals));
        ON_CALL(exhausted, is_exhausted(40)).WillByDefault(Return(false));
        ON_CALL(exhausted, is_exhausted(41)).WillByDefault(Return(false));
        ON_CALL(exhausted, is_exhausted(42)).WillByDefault(Return(true));
    }
};

}

TEST_F(PudFocusQueryListerTest, EveryGoalOfAFocusLeafIsListedInGoalOrderWithWhetherItHasCandidates) {
    EXPECT_EQ(lister.list(), (std::vector<pud_leaf_goal_query>{
        pud_leaf_goal_query{.node = pud_test_node(3), .goal = 0, .head = 40, .has_candidates = true},
        pud_leaf_goal_query{.node = pud_test_node(3), .goal = 1, .head = 41, .has_candidates = true},
        pud_leaf_goal_query{.node = pud_test_node(3), .goal = 2, .head = 42, .has_candidates = false},
    }));
}

TEST_F(PudFocusQueryListerTest, ARefutedLeafIsNotListed) {
    ON_CALL(refuted, is_refuted(pud_test_node(3))).WillByDefault(Return(true));
    EXPECT_CALL(node_queries, get(_)).Times(0);
    EXPECT_THAT(lister.list(), IsEmpty());
}

TEST_F(PudFocusQueryListerTest, LeavesAreListedInFrontierOrder) {
    pud_goal_list one_goal{.goals = immer::map<pud_body_goal_id, const expr*>{}.set(0, &goal_expr), .next_id = 1};
    pud_node_queries one_query{pud_node_queries{}.set(0, 40)};
    live.insert(pud_test_node(1));
    ON_CALL(node_queries, get(pud_test_node(1))).WillByDefault(ReturnRef(one_query));
    ON_CALL(goals, get(pud_test_node(1))).WillByDefault(ReturnRef(one_goal));
    const std::vector<pud_leaf_goal_query> listed = lister.list();
    ASSERT_EQ(listed.size(), 4u);
    EXPECT_EQ(listed[0].node, pud_test_node(1));
    EXPECT_EQ(listed[1].node, pud_test_node(3));
}

TEST_F(PudFocusQueryListerTest, AGoalWithoutAQueryBreaksTheInvariant) {
    leaf_queries = leaf_queries.erase(1);
    EXPECT_THROW(lister.list(), std::logic_error);
}
