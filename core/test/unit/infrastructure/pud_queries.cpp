// pud_queries: adopt axioms, rewrite unfolded leaves, candidacy snapshots.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <stdexcept>
#include <variant>
#include <vector>
#include "infrastructure/pud_queries.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/om_interval.hpp"
#include "value_objects/pud_candidate_search_result.hpp"
#include "value_objects/pud_db_node.hpp"
#include "value_objects/pud_forced_unfold.hpp"
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
        , leftover_{expr::var{1}}
        , axiom_{pud_rule_id::axiom{0}}
        , other_{pud_rule_id::axiom{1}}
        , child_{pud_rule_id::inference{&axiom_, 0, &other_}}
        , node_{interval_, {}, {&body_}, 1}
        , two_goal_node_{interval_, {}, {&body_, &leftover_}, 1}
        , empty_node_{interval_, {}, {}, 1}
        , child_node_{nested_, {}, {&leftover_}, 2}
        , queries_(get_node_, allocate_, reinit_, candidate_, witness_) {
        ON_CALL(get_node_, get_node(&axiom_)).WillByDefault(ReturnRef(node_));
        ON_CALL(get_node_, get_node(&other_)).WillByDefault(ReturnRef(empty_node_));
        ON_CALL(get_node_, get_node(&child_)).WillByDefault(ReturnRef(child_node_));
        ON_CALL(allocate_, allocate_child_of(_)).WillByDefault(Return(nested_));
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
    expr leftover_;
    pud_rule_id axiom_;
    pud_rule_id other_;
    pud_rule_id child_;
    pud_db_node node_;
    pud_db_node two_goal_node_;
    pud_db_node empty_node_;
    pud_db_node child_node_;
    NiceMock<MockGetNode> get_node_;
    NiceMock<MockAllocateChildInterval> allocate_;
    NiceMock<MockReinit> reinit_;
    NiceMock<MockResumeCandidateSearch> candidate_;
    NiceMock<MockResumeWitnessSearch> witness_;
    test_queries_t queries_;
};

TEST_F(PudQueriesTest, AdoptAxiomStoresOneQueryPerBodyGoal) {
    EXPECT_CALL(reinit_, reinit(_));
    EXPECT_CALL(candidate_, resume(_, _));
    queries_.adopt_axiom(&axiom_);
    ASSERT_EQ(queries_.get(&axiom_).size(), 1u);
    EXPECT_EQ(queries_.get(&axiom_)[0]->body_goal, &body_);
    EXPECT_EQ(queries_.get(&axiom_)[0]->frame_offset, 1u);
    ASSERT_EQ(queries_.get(&axiom_)[0]->axiom_contexts.size(), 1u);
    EXPECT_EQ(queries_.get(&axiom_)[0]->axiom_contexts[0].cursor, &axiom_);
}

TEST_F(PudQueriesTest, AdoptAxiomAppendsContextOnExistingLeafQuery) {
    queries_.adopt_axiom(&axiom_);
    queries_.adopt_axiom(&other_);
    ASSERT_EQ(queries_.get(&axiom_)[0]->axiom_contexts.size(), 2u);
    EXPECT_EQ(queries_.get(&axiom_)[0]->axiom_contexts[1].cursor, &other_);
}

TEST_F(PudQueriesTest, TakeForcedUnfoldsYieldsUnitThenClears) {
    queries_.adopt_axiom(&axiom_);
    const std::vector<pud_forced_unfold> yields = queries_.take_forced_unfolds();
    ASSERT_EQ(yields.size(), 1u);
    ASSERT_TRUE(std::holds_alternative<pud_forced_unfold::unit>(yields[0].content));
    const auto unit = std::get<pud_forced_unfold::unit>(yields[0].content);
    EXPECT_EQ(unit.leaf, &axiom_);
    EXPECT_EQ(unit.body_goal_idx, 0u);
    EXPECT_TRUE(queries_.take_forced_unfolds().empty());
}

TEST_F(PudQueriesTest, TakeForcedUnfoldsYieldsRefutedWhenZeroLiveCursors) {
    ON_CALL(candidate_, resume(_, _)).WillByDefault(Return(
        pud_candidate_search_result{pud_candidate_search_result::axiom_refuted{}}));
    queries_.adopt_axiom(&axiom_);
    const std::vector<pud_forced_unfold> yields = queries_.take_forced_unfolds();
    ASSERT_EQ(yields.size(), 1u);
    ASSERT_TRUE(std::holds_alternative<pud_forced_unfold::refuted>(yields[0].content));
    EXPECT_EQ(std::get<pud_forced_unfold::refuted>(yields[0].content).leaf, &axiom_);
}

TEST_F(PudQueriesTest, LiveCalleesSkipsRefutedCursors) {
    queries_.adopt_axiom(&axiom_);
    queries_.adopt_axiom(&other_);
    ON_CALL(candidate_, resume(_, _)).WillByDefault(
        [this](pud_query&, pud_candidate_search_context& ctx) {
            if (ctx.cursor == &other_)
                return pud_candidate_search_result{
                    pud_candidate_search_result::axiom_refuted{}};
            return pud_candidate_search_result{
                pud_candidate_search_result::self_witness{}};
        });
    EXPECT_EQ(queries_.live_callees(&axiom_, 0),
              (std::vector<const pud_rule_id*>{&axiom_}));
}

TEST_F(PudQueriesTest, ReplaceUnfoldedResumesWatchersOfTheDeadLeaf) {
    ON_CALL(get_node_, get_node(&other_)).WillByDefault(ReturnRef(node_));
    ON_CALL(get_node_, get_node(&child_)).WillByDefault(ReturnRef(empty_node_));
    ON_CALL(candidate_, resume(_, _)).WillByDefault(
        [this](pud_query&, pud_candidate_search_context& ctx) {
            if (ctx.cursor != &axiom_)
                return pud_candidate_search_result{
                    pud_candidate_search_result::self_witness{}};
            ctx.live_edges = {{&other_, &other_}};
            return pud_candidate_search_result{
                pud_candidate_search_result::choice_point{}};
        });
    queries_.adopt_axiom(&axiom_);
    queries_.adopt_axiom(&other_);
    EXPECT_CALL(witness_, resume(_, _)).Times(::testing::AtLeast(1));
    queries_.replace_unfolded(&other_, 0, {&child_});
}

TEST_F(PudQueriesTest, ReplaceUnfoldedClearsParentAndForksLeftoverOntoChild) {
    ON_CALL(get_node_, get_node(&axiom_)).WillByDefault(ReturnRef(two_goal_node_));
    queries_.adopt_axiom(&axiom_);
    queries_.replace_unfolded(&axiom_, 0, {&child_});
    EXPECT_THROW(queries_.get(&axiom_), std::out_of_range);
    ASSERT_EQ(queries_.get(&child_).size(), 2u);
    EXPECT_EQ(queries_.get(&child_)[0]->body_goal, &leftover_);
    EXPECT_EQ(queries_.get(&child_)[1]->body_goal, &leftover_);
}
