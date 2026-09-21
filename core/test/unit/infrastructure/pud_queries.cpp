// pud_queries: adopt axioms, rewrite unfolded leaves, candidacy snapshots.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cstddef>
#include <deque>
#include <random>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
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

struct MockRootInterval {
    MOCK_METHOD(om_interval, root_interval, (const pud_rule_id*), ());
};

struct MockAllocateChildInterval {
    MOCK_METHOD(om_interval, allocate_child_of, (const om_interval&), ());
};

struct MockReinit {
    MOCK_METHOD(void, reinit, (pud_query&), ());
};

struct MockDropQueryEnv {
    MOCK_METHOD(void, drop_query, (pud_query*), ());
};

struct MockResumeCandidateSearch {
    MOCK_METHOD(pud_candidate_search_result, resume, (pud_query&, pud_candidate_search_context&), ());
};

struct MockResumeWitnessSearch {
    MOCK_METHOD(pud_witness_search_result, resume, (pud_query&, pud_witness_search_context&), ());
};

using test_queries_t = pud_queries<
    NiceMock<MockGetNode>,
    NiceMock<MockRootInterval>,
    NiceMock<MockAllocateChildInterval>,
    NiceMock<MockReinit>,
    NiceMock<MockDropQueryEnv>,
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
        , node_{{}, {&body_}, 1}
        , two_goal_node_{{}, {&body_, &leftover_}, 1}
        , empty_node_{{}, {}, 1}
        , child_node_{{}, {&leftover_}, 2}
        , queries_(get_node_, root_interval_, allocate_, reinit_, drop_,
                   candidate_, witness_) {
        ON_CALL(get_node_, get_node(&axiom_)).WillByDefault(ReturnRef(node_));
        ON_CALL(get_node_, get_node(&other_)).WillByDefault(ReturnRef(empty_node_));
        ON_CALL(get_node_, get_node(&child_)).WillByDefault(ReturnRef(child_node_));
        ON_CALL(root_interval_, root_interval(_)).WillByDefault(Return(interval_));
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
    NiceMock<MockRootInterval> root_interval_;
    NiceMock<MockAllocateChildInterval> allocate_;
    NiceMock<MockReinit> reinit_;
    NiceMock<MockDropQueryEnv> drop_;
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
    EXPECT_EQ(queries_.get(&axiom_)[0]->axiom_contexts[0].added_body_goals,
              (std::vector<const expr*>{&body_}));
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

TEST_F(PudQueriesTest, AdoptEmptyBodyInstallsNoQueriesAndMarksDirty) {
    ON_CALL(get_node_, get_node(&axiom_)).WillByDefault(ReturnRef(empty_node_));
    queries_.adopt_axiom(&axiom_);
    ASSERT_EQ(queries_.get(&axiom_).size(), 0u);
    EXPECT_TRUE(queries_.take_forced_unfolds().empty());
}

TEST_F(PudQueriesTest, AdoptMultiGoalInstallsOneQueryPerGoal) {
    ON_CALL(get_node_, get_node(&axiom_)).WillByDefault(ReturnRef(two_goal_node_));
    queries_.adopt_axiom(&axiom_);
    ASSERT_EQ(queries_.get(&axiom_).size(), 2u);
    EXPECT_EQ(queries_.get(&axiom_)[0]->body_goal, &body_);
    EXPECT_EQ(queries_.get(&axiom_)[1]->body_goal, &leftover_);
}

TEST_F(PudQueriesTest, AdoptSecondAxiomQueryRootContextsIncludePriors) {
    ON_CALL(get_node_, get_node(&other_)).WillByDefault(ReturnRef(node_));
    queries_.adopt_axiom(&axiom_);
    queries_.adopt_axiom(&other_);
    ASSERT_EQ(queries_.get(&other_).size(), 1u);
    ASSERT_EQ(queries_.get(&other_)[0]->axiom_contexts.size(), 2u);
    EXPECT_EQ(queries_.get(&other_)[0]->axiom_contexts[0].cursor, &axiom_);
    EXPECT_EQ(queries_.get(&other_)[0]->axiom_contexts[1].cursor, &other_);
}

TEST_F(PudQueriesTest, LiveCalleesOnSecondBodyGoal) {
    ON_CALL(get_node_, get_node(&axiom_)).WillByDefault(ReturnRef(two_goal_node_));
    queries_.adopt_axiom(&axiom_);
    EXPECT_EQ(queries_.live_callees(&axiom_, 1),
              (std::vector<const pud_rule_id*>{&axiom_}));
}

TEST_F(PudQueriesTest, ReplaceUnfoldedForksDistinctLeftoverAndChildGoals) {
    expr child_goal{expr::var{2}};
    pud_db_node child_with_goal{{}, {&child_goal}, 2};
    ON_CALL(get_node_, get_node(&axiom_)).WillByDefault(ReturnRef(two_goal_node_));
    ON_CALL(get_node_, get_node(&child_)).WillByDefault(ReturnRef(child_with_goal));
    queries_.adopt_axiom(&axiom_);
    queries_.replace_unfolded(&axiom_, 0, {&child_});
    ASSERT_EQ(queries_.get(&child_).size(), 2u);
    EXPECT_EQ(queries_.get(&child_)[0]->body_goal, &leftover_);
    EXPECT_EQ(queries_.get(&child_)[1]->body_goal, &child_goal);
}

TEST_F(PudQueriesTest, ReplaceUnfoldedWithEmptyChildrenClearsParent) {
    queries_.adopt_axiom(&axiom_);
    queries_.replace_unfolded(&axiom_, 0, {});
    EXPECT_THROW(queries_.get(&axiom_), std::out_of_range);
}

TEST_F(PudQueriesTest, ResumeDeadWitnessResumesCandidateWhenCursorMatchesEmptyEdges) {
    ON_CALL(get_node_, get_node(&other_)).WillByDefault(ReturnRef(node_));
    ON_CALL(get_node_, get_node(&child_)).WillByDefault(ReturnRef(empty_node_));
    queries_.adopt_axiom(&axiom_);
    queries_.adopt_axiom(&other_);
    EXPECT_CALL(candidate_, resume(_, _)).Times(::testing::AtLeast(1));
    queries_.replace_unfolded(&other_, 0, {&child_});
}

TEST_F(PudQueriesTest, ResumeDeadWitnessDropsFailedEdgesThenResumesCandidate) {
    bool other_is_live = true;
    ON_CALL(get_node_, get_node(&other_)).WillByDefault(ReturnRef(node_));
    ON_CALL(get_node_, get_node(&child_)).WillByDefault(ReturnRef(empty_node_));
    ON_CALL(candidate_, resume(_, _)).WillByDefault(
        [this, &other_is_live](pud_query&, pud_candidate_search_context& ctx) {
            if (ctx.cursor != &axiom_)
                return pud_candidate_search_result{
                    pud_candidate_search_result::self_witness{}};
            if (other_is_live)
                ctx.live_edges = {{&other_, &other_}};
            return pud_candidate_search_result{
                pud_candidate_search_result::choice_point{}};
        });
    ON_CALL(witness_, resume(_, _)).WillByDefault(Return(
        pud_witness_search_result{pud_witness_search_result::failed{}}));
    queries_.adopt_axiom(&axiom_);
    queries_.adopt_axiom(&other_);
    other_is_live = false;
    EXPECT_CALL(candidate_, resume(_, _)).Times(::testing::AtLeast(1));
    queries_.replace_unfolded(&other_, 0, {&child_});
    bool saw_dropped = false;
    for (const pud_candidate_search_context& ctx : queries_.get(&axiom_)[0]->axiom_contexts) {
        for (const pud_witness_search_context& edge : ctx.live_edges) {
            if (edge.current == &other_)
                saw_dropped = true;
        }
    }
    EXPECT_FALSE(saw_dropped);
}

TEST_F(PudQueriesTest, TakeForcedUnfoldsRefuteShortCircuitsLaterUnits) {
    ON_CALL(get_node_, get_node(&axiom_)).WillByDefault(ReturnRef(two_goal_node_));
    ON_CALL(candidate_, resume(_, _)).WillByDefault(
        [this](pud_query& query, pud_candidate_search_context&) {
            if (query.body_goal == &body_)
                return pud_candidate_search_result{
                    pud_candidate_search_result::axiom_refuted{}};
            return pud_candidate_search_result{
                pud_candidate_search_result::self_witness{}};
        });
    queries_.adopt_axiom(&axiom_);
    const std::vector<pud_forced_unfold> yields = queries_.take_forced_unfolds();
    ASSERT_EQ(yields.size(), 1u);
    ASSERT_TRUE(std::holds_alternative<pud_forced_unfold::refuted>(yields[0].content));
    EXPECT_EQ(std::get<pud_forced_unfold::refuted>(yields[0].content).leaf, &axiom_);
}

TEST_F(PudQueriesTest, TakeForcedUnfoldsEmitsUnitPerUnaryGoal) {
    ON_CALL(get_node_, get_node(&axiom_)).WillByDefault(ReturnRef(two_goal_node_));
    queries_.adopt_axiom(&axiom_);
    const std::vector<pud_forced_unfold> yields = queries_.take_forced_unfolds();
    ASSERT_EQ(yields.size(), 2u);
    ASSERT_TRUE(std::holds_alternative<pud_forced_unfold::unit>(yields[0].content));
    ASSERT_TRUE(std::holds_alternative<pud_forced_unfold::unit>(yields[1].content));
    EXPECT_EQ(std::get<pud_forced_unfold::unit>(yields[0].content).body_goal_idx, 0u);
    EXPECT_EQ(std::get<pud_forced_unfold::unit>(yields[1].content).body_goal_idx, 1u);
}

TEST_F(PudQueriesTest, TakeForcedUnfoldsOrdersDirtyLeavesByRuleId) {
    ON_CALL(get_node_, get_node(&other_)).WillByDefault(ReturnRef(node_));
    ON_CALL(candidate_, resume(_, _)).WillByDefault(
        [](pud_query& query, pud_candidate_search_context& ctx) {
            if (!query.axiom_contexts.empty()
                    && ctx.cursor == query.axiom_contexts.front().cursor)
                return pud_candidate_search_result{
                    pud_candidate_search_result::self_witness{}};
            return pud_candidate_search_result{
                pud_candidate_search_result::axiom_refuted{}};
        });
    queries_.adopt_axiom(&other_);
    queries_.adopt_axiom(&axiom_);
    const std::vector<pud_forced_unfold> yields = queries_.take_forced_unfolds();
    ASSERT_EQ(yields.size(), 2u);
    ASSERT_TRUE(std::holds_alternative<pud_forced_unfold::unit>(yields[0].content));
    ASSERT_TRUE(std::holds_alternative<pud_forced_unfold::unit>(yields[1].content));
    EXPECT_EQ(std::get<pud_forced_unfold::unit>(yields[0].content).leaf, &axiom_);
    EXPECT_EQ(std::get<pud_forced_unfold::unit>(yields[1].content).leaf, &other_);
}

TEST_F(PudQueriesTest, GetUnknownLeafThrows) {
    EXPECT_THROW(queries_.get(&axiom_), std::out_of_range);
}

TEST_F(PudQueriesTest, WatchUnwatchAcrossAdoptReplaceTake) {
    queries_.adopt_axiom(&axiom_);
    EXPECT_NO_THROW(queries_.get(&axiom_));
    queries_.take_forced_unfolds();
    EXPECT_TRUE(queries_.take_forced_unfolds().empty());
    queries_.replace_unfolded(&axiom_, 0, {&child_});
    EXPECT_THROW(queries_.get(&axiom_), std::out_of_range);
    EXPECT_NO_THROW(queries_.get(&child_));
    queries_.take_forced_unfolds();
    EXPECT_TRUE(queries_.take_forced_unfolds().empty());
}

TEST_F(PudQueriesTest, StressManyAxiomsAttachToAllLeaves) {
    struct rec {
        pud_rule_id id;
        pud_db_node node;
    };
    std::deque<rec> axioms;
    ON_CALL(get_node_, get_node(_)).WillByDefault([&](const pud_rule_id* id) -> const pud_db_node& {
        for (const rec& entry : axioms) {
            if (&entry.id == id)
                return entry.node;
        }
        return node_;
    });
    for (int idx = 0; idx < 32; ++idx) {
        axioms.push_back(rec{
            pud_rule_id{pud_rule_id::axiom{static_cast<size_t>(idx)}},
            pud_db_node{{}, {&body_}, 1}});
        queries_.adopt_axiom(&axioms.back().id);
    }
    ASSERT_EQ(queries_.get(&axioms.front().id).size(), 1u);
    EXPECT_EQ(queries_.get(&axioms.front().id)[0]->axiom_contexts.size(), 32u);
    ASSERT_EQ(queries_.get(&axioms.back().id).size(), 1u);
    EXPECT_EQ(queries_.get(&axioms.back().id)[0]->axiom_contexts.size(), 32u);
}

TEST_F(PudQueriesTest, FuzzAdoptThenReplaceTake) {
    struct rec {
        pud_rule_id id;
        pud_db_node node;
    };
    std::deque<rec> store;
    std::vector<const pud_rule_id*> owned;
    ON_CALL(get_node_, get_node(_)).WillByDefault([&](const pud_rule_id* id) -> const pud_db_node& {
        for (const rec& entry : store) {
            if (&entry.id == id)
                return entry.node;
        }
        return node_;
    });

    constexpr uint32_t k_seed = 42;
    std::mt19937 rng{k_seed};
    std::ostringstream log;
    for (int idx = 0; idx < 16; ++idx) {
        store.push_back(rec{
            pud_rule_id{pud_rule_id::axiom{store.size()}},
            pud_db_node{{}, {&body_}, 1}});
        const pud_rule_id* id = &store.back().id;
        queries_.adopt_axiom(id);
        owned.push_back(id);
    }
    std::uniform_int_distribution<int> op_dist(0, 2);
    for (int step = 0; step < 64; ++step) {
        const int op = op_dist(rng);
        log << step << ':' << op << ' ';
        switch (op) {
        case 0:
            if (!owned.empty()) {
                const pud_rule_id* leaf = owned[rng() % owned.size()];
                if (!queries_.get(leaf).empty())
                    queries_.live_callees(leaf, 0);
            }
            break;
        case 1:
            if (!owned.empty()) {
                const size_t parent_idx = rng() % owned.size();
                const pud_rule_id* parent = owned[parent_idx];
                if (queries_.get(parent).empty())
                    break;
                store.push_back(rec{
                    pud_rule_id{pud_rule_id::inference{parent, 0, parent}},
                    pud_db_node{{}, {&leftover_}, 2}});
                const pud_rule_id* child = &store.back().id;
                queries_.replace_unfolded(parent, 0, {child});
                owned.erase(owned.begin() + static_cast<std::ptrdiff_t>(parent_idx));
                owned.push_back(child);
                EXPECT_THROW(queries_.get(parent), std::out_of_range)
                    << "seed " << k_seed << " log " << log.str();
            }
            break;
        case 2:
            queries_.take_forced_unfolds();
            EXPECT_TRUE(queries_.take_forced_unfolds().empty())
                << "seed " << k_seed << " log " << log.str();
            break;
        }
        for (const pud_rule_id* leaf : owned)
            EXPECT_NO_THROW(queries_.get(leaf)) << "seed " << k_seed << " log " << log.str();
    }
}
