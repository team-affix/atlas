#include <stdexcept>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_blocked_leaf_refuter.hpp"
#include "pud_test_node.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockListQueries {
    MOCK_METHOD(std::vector<pud_leaf_goal_query>, list, (), (const));
};
struct MockRefuteLeaf {
    MOCK_METHOD(std::vector<pud_global_node_id>, refute, (pud_global_node_id));
};
struct MockEraseRefuted {
    MOCK_METHOD(void, erase_refuted, ((const std::vector<pud_global_node_id>&)));
};

using test_refuter_t =
    pud_blocked_leaf_refuter<NiceMock<MockListQueries>, NiceMock<MockRefuteLeaf>, NiceMock<MockEraseRefuted>>;

pud_leaf_goal_query query_on(pud_node_id node, bool has_candidates) {
    return pud_leaf_goal_query{.node = pud_test_node(node), .goal = 0, .head = 40, .has_candidates = has_candidates};
}

struct PudBlockedLeafRefuterTest : public ::testing::Test {
    NiceMock<MockListQueries> queries;
    NiceMock<MockRefuteLeaf> leaf_refuter;
    NiceMock<MockEraseRefuted> frontier;
    test_refuter_t refuter{queries, leaf_refuter, frontier};
};

}

TEST_F(PudBlockedLeafRefuterTest, NoBlockedQueryRefutesNothing) {
    ON_CALL(queries, list()).WillByDefault(Return(std::vector<pud_leaf_goal_query>{query_on(3, true)}));
    EXPECT_CALL(leaf_refuter, refute(_)).Times(0);
    EXPECT_FALSE(refuter.refute_blocked());
}

TEST_F(PudBlockedLeafRefuterTest, OnlyTheFirstBlockedLeafIsRefutedAndWhatItRefutesLeavesTheFrontier) {
    ON_CALL(queries, list())
        .WillByDefault(Return(std::vector<pud_leaf_goal_query>{query_on(3, true), query_on(4, false), query_on(5, false)}));
    EXPECT_CALL(leaf_refuter, refute(pud_test_node(4)))
        .WillOnce(Return(std::vector<pud_global_node_id>{pud_test_node(4), pud_test_node(1)}));
    EXPECT_CALL(leaf_refuter, refute(pud_test_node(5))).Times(0);
    EXPECT_CALL(frontier, erase_refuted(std::vector<pud_global_node_id>{pud_test_node(4), pud_test_node(1)}));
    EXPECT_TRUE(refuter.refute_blocked());
}

// Listed leaves are live and unrefuted, so refuting one always refutes at least that leaf.
TEST_F(PudBlockedLeafRefuterTest, ARefutationThatRefutesNothingBreaksTheInvariant) {
    ON_CALL(queries, list()).WillByDefault(Return(std::vector<pud_leaf_goal_query>{query_on(4, false)}));
    ON_CALL(leaf_refuter, refute(pud_test_node(4))).WillByDefault(Return(std::vector<pud_global_node_id>{}));
    EXPECT_THROW(refuter.refute_blocked(), std::logic_error);
}
