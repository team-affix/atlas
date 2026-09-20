// pud_queries: own leaf queries, watch witnesses, resume on invalidate, dirty-set.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <stdexcept>
#include <vector>
#include "infrastructure/pud_queries.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/om_interval.hpp"
#include "value_objects/pud_candidate_search_result.hpp"
#include "value_objects/pud_db_node.hpp"
#include "value_objects/pud_query.hpp"
#include "value_objects/pud_rule_id.hpp"
#include "value_objects/pud_witness_search_result.hpp"

using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::_;

struct MockGetNode {
    MOCK_METHOD(const pud_db_node&, get_node, (const pud_rule_id*), ());
};

struct MockAllocateChildInterval {
    MOCK_METHOD(om_interval, allocate_child_of, (const om_interval&), ());
};

struct MockOrderedRoots {
    MOCK_METHOD(std::vector<const pud_rule_id*>, ordered_roots, (), ());
};

struct MockOrderedLeaves {
    MOCK_METHOD(std::vector<const pud_rule_id*>, ordered_leaves, (), ());
};

struct MockReinit {
    MOCK_METHOD(void, reinit, (pud_query&), ());
};

struct MockResumeCandidateSearch {
    MOCK_METHOD(pud_candidate_search_result, resume, (pud_query&, pud_candidate_search_context&), ());
};

struct MockResumeWitnessSearch {
    MOCK_METHOD(pud_witness_search_result, resume, (pud_query&, pud_witness_search_context&), ());
};

using test_queries_t = pud_queries<
    NiceMock<MockGetNode>,
    NiceMock<MockAllocateChildInterval>,
    NiceMock<MockOrderedRoots>,
    NiceMock<MockOrderedLeaves>,
    NiceMock<MockReinit>,
    NiceMock<MockResumeCandidateSearch>,
    NiceMock<MockResumeWitnessSearch>>;

struct PudQueriesTest : public ::testing::Test {
    PudQueriesTest()
        : open_(1)
        , close_(2)
        , nested_open_(3)
        , nested_close_(4)
        , interval_{om_label(&open_), om_label(&close_)}
        , nested_{om_label(&nested_open_), om_label(&nested_close_)}
        , body_{expr::var{0}}
        , axiom_{pud_rule_id::axiom{0}}
        , other_{pud_rule_id::axiom{1}}
        , dead_{pud_rule_id::inference{&axiom_, 0, &axiom_}}
        , sibling_{pud_rule_id::inference{&axiom_, 1, &axiom_}}
        , node_{interval_, {}, {&body_}, 1}
        , empty_node_{interval_, {}, {}, 1}
        , queries_(get_node_, allocate_, ordered_roots_, ordered_leaves_,
                   reinit_, candidate_, witness_) {
        ON_CALL(get_node_, get_node(&axiom_)).WillByDefault(ReturnRef(node_));
        ON_CALL(get_node_, get_node(&other_)).WillByDefault(ReturnRef(empty_node_));
        ON_CALL(allocate_, allocate_child_of(_)).WillByDefault(Return(nested_));
        ON_CALL(ordered_roots_, ordered_roots())
            .WillByDefault(Return(std::vector<const pud_rule_id*>{&axiom_}));
        ON_CALL(ordered_leaves_, ordered_leaves())
            .WillByDefault(Return(std::vector<const pud_rule_id*>{&axiom_}));
        ON_CALL(candidate_, resume(_, _)).WillByDefault(Return(
            pud_candidate_search_result{pud_candidate_search_result::self_witness{}}));
    }

    uint64_t open_;
    uint64_t close_;
    uint64_t nested_open_;
    uint64_t nested_close_;
    om_interval interval_;
    om_interval nested_;
    expr body_;
    pud_rule_id axiom_;
    pud_rule_id other_;
    pud_rule_id dead_;
    pud_rule_id sibling_;
    pud_db_node node_;
    pud_db_node empty_node_;
    NiceMock<MockGetNode> get_node_;
    NiceMock<MockAllocateChildInterval> allocate_;
    NiceMock<MockOrderedRoots> ordered_roots_;
    NiceMock<MockOrderedLeaves> ordered_leaves_;
    NiceMock<MockReinit> reinit_;
    NiceMock<MockResumeCandidateSearch> candidate_;
    NiceMock<MockResumeWitnessSearch> witness_;
    test_queries_t queries_;
};

TEST_F(PudQueriesTest, InstallStoresOneQueryPerBodyGoal) {
    EXPECT_CALL(reinit_, reinit(_));
    EXPECT_CALL(candidate_, resume(_, _));
    queries_.install(&axiom_);
    ASSERT_EQ(queries_.get(&axiom_).size(), 1u);
    EXPECT_EQ(queries_.get(&axiom_)[0]->body_goal, &body_);
    EXPECT_EQ(queries_.get(&axiom_)[0]->frame_offset, 1u);
    ASSERT_EQ(queries_.get(&axiom_)[0]->axiom_contexts.size(), 1u);
    EXPECT_EQ(queries_.get(&axiom_)[0]->axiom_contexts[0].cursor, &axiom_);
}

TEST_F(PudQueriesTest, ClearRemovesTheLeaf) {
    queries_.install(&axiom_);
    queries_.clear(&axiom_);
    EXPECT_THROW(queries_.get(&axiom_), std::out_of_range);
}

TEST_F(PudQueriesTest, AttachAxiomAppendsContextOnExistingLeafQuery) {
    queries_.install(&axiom_);
    ON_CALL(ordered_leaves_, ordered_leaves())
        .WillByDefault(Return(std::vector<const pud_rule_id*>{&axiom_, &other_}));
    queries_.attach_axiom(&other_);
    ASSERT_EQ(queries_.get(&axiom_)[0]->axiom_contexts.size(), 2u);
    EXPECT_EQ(queries_.get(&axiom_)[0]->axiom_contexts[1].cursor, &other_);
}

TEST_F(PudQueriesTest, TakeDirtyLeavesReturnsInstalledThenClears) {
    queries_.install(&axiom_);
    const std::vector<const pud_rule_id*> dirty = queries_.take_dirty_leaves();
    ASSERT_EQ(dirty.size(), 1u);
    EXPECT_EQ(dirty[0], &axiom_);
    EXPECT_TRUE(queries_.take_dirty_leaves().empty());
}

TEST_F(PudQueriesTest, SuccessfulWitnessResumeDoesNotResumeCandidate) {
    ON_CALL(candidate_, resume(_, _)).WillByDefault([this](pud_query&, pud_candidate_search_context& ctx) {
        ctx.live_edges = {{&dead_, &dead_}};
        return pud_candidate_search_result{pud_candidate_search_result::choice_point{}};
    });
    queries_.install(&axiom_);
    EXPECT_CALL(witness_, resume(_, _)).WillOnce(Return(
        pud_witness_search_result{pud_witness_search_result::found{}}));
    EXPECT_CALL(candidate_, resume(_, _)).Times(0);
    queries_.invalidate_leaf(&dead_);
}

TEST_F(PudQueriesTest, FailedWitnessResumeResumesCandidateContext) {
    ON_CALL(candidate_, resume(_, _)).WillByDefault([this](pud_query&, pud_candidate_search_context& ctx) {
        ctx.live_edges = {{&dead_, &dead_}, {&sibling_, &sibling_}};
        return pud_candidate_search_result{pud_candidate_search_result::choice_point{}};
    });
    queries_.install(&axiom_);
    EXPECT_CALL(witness_, resume(_, _)).WillOnce(Return(
        pud_witness_search_result{pud_witness_search_result::failed{}}));
    EXPECT_CALL(candidate_, resume(_, _)).WillOnce(Return(
        pud_candidate_search_result{pud_candidate_search_result::choice_point{}}));
    queries_.invalidate_leaf(&dead_);
}

TEST_F(PudQueriesTest, SelfWitnessCursorResumesCandidateSearch) {
    queries_.install(&axiom_);
    EXPECT_CALL(candidate_, resume(_, _)).WillOnce(Return(
        pud_candidate_search_result{pud_candidate_search_result::axiom_refuted{}}));
    queries_.invalidate_leaf(&axiom_);
}
