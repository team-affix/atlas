#include <map>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_node_deactivator.hpp"
#include "pud_test_node.hpp"

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::InSequence;
using ::testing::Invoke;
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockEraseNodeField {
    MOCK_METHOD(void, erase, (pud_global_node_id));
};
struct MockInvalidateQueryLeaf {
    MOCK_METHOD(std::vector<pud_mhq_head_id>, invalidate_leaf, (pud_global_node_id));
};
struct MockMarkQueryExhausted {
    MOCK_METHOD(void, mark_exhausted, (pud_mhq_head_id));
};
struct MockFindQueryLeaves {
    MOCK_METHOD((coroutine<pud_unfold, void>), find_leaves, (pud_mhq_head_id), (const));
};

using test_node_deactivator_t = pud_node_deactivator<
    NiceMock<MockEraseNodeField>, NiceMock<MockEraseNodeField>, NiceMock<MockEraseNodeField>,
    NiceMock<MockEraseNodeField>, NiceMock<MockInvalidateQueryLeaf>, NiceMock<MockMarkQueryExhausted>,
    NiceMock<MockFindQueryLeaves>>;

// Node 5's witnesses leave no query without a candidate unless a test says otherwise. Query 40 is held by
// leaves 7 and 8, query 41 by leaf 9.
struct PudNodeDeactivatorTest : public ::testing::Test {
    NiceMock<MockEraseNodeField> erase_queries;
    NiceMock<MockEraseNodeField> erase_watches;
    NiceMock<MockEraseNodeField> erase_bindings;
    NiceMock<MockEraseNodeField> erase_lvcs;
    NiceMock<MockInvalidateQueryLeaf> invalidate;
    NiceMock<MockMarkQueryExhausted> introducers;
    NiceMock<MockFindQueryLeaves> leaf_finder;
    test_node_deactivator_t deactivator{
        erase_queries, erase_watches, erase_bindings, erase_lvcs, invalidate, introducers, leaf_finder};
    std::map<pud_mhq_head_id, std::vector<pud_global_node_id>> held{
        {40, {pud_test_node(7), pud_test_node(8)}}, {41, {pud_test_node(9)}}};

    coroutine<pud_unfold, void> find_held_leaves(pud_mhq_head_id head) {
        for (const pud_global_node_id leaf : held.at(head))
            co_yield pud_unfold{.leaf = leaf, .goal = 0};
    }

    void SetUp() override {
        ON_CALL(invalidate, invalidate_leaf(pud_test_node(5))).WillByDefault(Return(std::vector<pud_mhq_head_id>{}));
        ON_CALL(leaf_finder, find_leaves(_)).WillByDefault(Invoke(this, &PudNodeDeactivatorTest::find_held_leaves));
    }
};

}

TEST_F(PudNodeDeactivatorTest, TheActiveFieldsAreErasedInReverseOfActivationThenItStopsWitnessing) {
    InSequence order;
    EXPECT_CALL(erase_queries, erase(pud_test_node(5)));
    EXPECT_CALL(erase_watches, erase(pud_test_node(5)));
    EXPECT_CALL(erase_bindings, erase(pud_test_node(5)));
    EXPECT_CALL(erase_lvcs, erase(pud_test_node(5)));
    EXPECT_CALL(invalidate, invalidate_leaf(pud_test_node(5)));
    deactivator.deactivate(pud_test_node(5));
}

TEST_F(PudNodeDeactivatorTest, NothingIsStrandedWhileEveryQueryKeepsACandidate) {
    EXPECT_CALL(leaf_finder, find_leaves(_)).Times(0);
    EXPECT_THAT(deactivator.deactivate(pud_test_node(5)), IsEmpty());
}

TEST_F(PudNodeDeactivatorTest, EveryLeafHoldingAQueryLeftWithoutACandidateIsReturned) {
    ON_CALL(invalidate, invalidate_leaf(pud_test_node(5))).WillByDefault(Return(std::vector<pud_mhq_head_id>{40, 41}));
    EXPECT_THAT(
        deactivator.deactivate(pud_test_node(5)), ElementsAre(pud_test_node(7), pud_test_node(8), pud_test_node(9)));
}

TEST_F(PudNodeDeactivatorTest, AQueryLeftWithoutACandidateIsMarkedExhaustedBeforeItsLeavesAreFound) {
    ON_CALL(invalidate, invalidate_leaf(pud_test_node(5))).WillByDefault(Return(std::vector<pud_mhq_head_id>{40, 41}));
    InSequence order;
    EXPECT_CALL(introducers, mark_exhausted(40u));
    EXPECT_CALL(leaf_finder, find_leaves(40u));
    EXPECT_CALL(introducers, mark_exhausted(41u));
    EXPECT_CALL(leaf_finder, find_leaves(41u));
    deactivator.deactivate(pud_test_node(5));
}
