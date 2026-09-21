// pud_queries: adopt axioms, unfold_site, replace_unfolded returns forced unfolds.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cstddef>
#include <deque>
#include <random>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>
#include "infrastructure/pud_queries.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/om_interval.hpp"
#include "value_objects/pud_candidate_search_context.hpp"
#include "value_objects/pud_forced_unfold.hpp"
#include "value_objects/pud_rule_id.hpp"
#include "value_objects/pud_unfold_site.hpp"
#include "value_objects/pud_witness_pair.hpp"
#include "value_objects/pud_witness_search_context.hpp"

using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::_;

struct MockGetAddedBodyGoals {
    MOCK_METHOD(const std::vector<const expr*>&, get, (const pud_rule_id*), ());
};

struct MockGetLvc {
    MOCK_METHOD(uint32_t, get, (const pud_rule_id*), ());
};

struct MockGetBaseInterval {
    MOCK_METHOD(om_interval, get, (const pud_rule_id*), ());
};

struct MockAllocateChildInterval {
    MOCK_METHOD(om_interval, allocate_child_of, (const om_interval&), ());
};

struct MockDropEnv {
    MOCK_METHOD(void, drop_env, (om_interval), ());
};

struct MockResumeCandidateSearch {
    MOCK_METHOD(void, resume, (pud_candidate_search_context&), ());
};

struct MockResumeWitnessSearch {
    MOCK_METHOD(void, resume, (pud_witness_search_context&), ());
};

using test_queries_t = pud_queries<
    NiceMock<MockGetAddedBodyGoals>,
    NiceMock<MockGetLvc>,
    NiceMock<MockGetBaseInterval>,
    NiceMock<MockAllocateChildInterval>,
    NiceMock<MockDropEnv>,
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
        , drain_{pud_rule_id::axiom{2}}
        , child_{pud_rule_id::inference{&axiom_, 0, &other_}}
        , axiom_goals_{&body_}
        , two_goals_{&body_, &leftover_}
        , empty_goals_{}
        , child_goals_{&leftover_}
        , drain_goals_{&body_}
        , queries_(get_added_body_goals_, get_lvc_, get_base_interval_, allocate_,
                   drop_, candidate_, witness_) {
        ON_CALL(get_added_body_goals_, get(&axiom_)).WillByDefault(ReturnRef(axiom_goals_));
        ON_CALL(get_added_body_goals_, get(&other_)).WillByDefault(ReturnRef(empty_goals_));
        ON_CALL(get_added_body_goals_, get(&drain_)).WillByDefault(ReturnRef(drain_goals_));
        ON_CALL(get_added_body_goals_, get(&child_)).WillByDefault(ReturnRef(child_goals_));
        ON_CALL(get_lvc_, get(&axiom_)).WillByDefault(Return(1u));
        ON_CALL(get_lvc_, get(&other_)).WillByDefault(Return(1u));
        ON_CALL(get_lvc_, get(&drain_)).WillByDefault(Return(1u));
        ON_CALL(get_lvc_, get(&child_)).WillByDefault(Return(2u));
        ON_CALL(get_base_interval_, get(_)).WillByDefault(Return(interval_));
        ON_CALL(allocate_, allocate_child_of(_)).WillByDefault(Return(nested_));
    }

    std::vector<pud_candidate_search_context*> group_at(const pud_rule_id* leaf, size_t idx) {
        return queries_.unfold_site(leaf, idx).live;
    }

    pud_candidate_search_context* ctx_at(const pud_rule_id* leaf, size_t idx) {
        return group_at(leaf, idx).at(0);
    }

    std::vector<pud_forced_unfold> drain_forced() {
        return queries_.replace_unfolded(&drain_, 0, {&child_});
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
    pud_rule_id drain_;
    pud_rule_id child_;
    std::vector<const expr*> axiom_goals_;
    std::vector<const expr*> two_goals_;
    std::vector<const expr*> empty_goals_;
    std::vector<const expr*> child_goals_;
    std::vector<const expr*> drain_goals_;
    NiceMock<MockGetAddedBodyGoals> get_added_body_goals_;
    NiceMock<MockGetLvc> get_lvc_;
    NiceMock<MockGetBaseInterval> get_base_interval_;
    NiceMock<MockAllocateChildInterval> allocate_;
    NiceMock<MockDropEnv> drop_;
    NiceMock<MockResumeCandidateSearch> candidate_;
    NiceMock<MockResumeWitnessSearch> witness_;
    test_queries_t queries_;
};

TEST_F(PudQueriesTest, AdoptAxiomStoresOneQueryPerBodyGoal) {
    queries_.adopt_axiom(&axiom_);
    pud_candidate_search_context* ctx = ctx_at(&axiom_, 0);
    EXPECT_EQ(ctx->body_goal, &body_);
    EXPECT_EQ(ctx->frame_offset, 1u);
    ASSERT_EQ(group_at(&axiom_, 0).size(), 1u);
    EXPECT_EQ(ctx->cursor, &axiom_);
    EXPECT_EQ(ctx->added_body_goals,
              (std::vector<const expr*>{&body_}));
}

TEST_F(PudQueriesTest, AdoptAxiomAppendsContextOnExistingLeafQuery) {
    queries_.adopt_axiom(&axiom_);
    queries_.adopt_axiom(&other_);
    ASSERT_EQ(group_at(&axiom_, 0).size(), 2u);
    EXPECT_EQ(group_at(&axiom_, 0)[1]->cursor, &other_);
}

TEST_F(PudQueriesTest, ReplaceUnfoldedYieldsUnitThenDoesNotRepeat) {
    ON_CALL(candidate_, resume(_)).WillByDefault(
        [this](pud_candidate_search_context& ctx) {
            if (ctx.cursor == &drain_)
                ctx.cursor = nullptr;
        });
    queries_.adopt_axiom(&drain_);
    queries_.adopt_axiom(&axiom_);
    const std::vector<pud_forced_unfold> yields = drain_forced();
    bool saw_axiom_unit = false;
    for (const pud_forced_unfold& yield : yields) {
        if (!std::holds_alternative<pud_forced_unfold::unit>(yield.content))
            continue;
        const auto unit = std::get<pud_forced_unfold::unit>(yield.content);
        if (unit.leaf != &axiom_)
            continue;
        saw_axiom_unit = true;
        EXPECT_EQ(unit.body_goal_idx, 0u);
    }
    EXPECT_TRUE(saw_axiom_unit);
    const std::vector<pud_forced_unfold> again =
        queries_.replace_unfolded(&child_, 0, {});
    for (const pud_forced_unfold& yield : again) {
        if (!std::holds_alternative<pud_forced_unfold::unit>(yield.content))
            continue;
        EXPECT_NE(std::get<pud_forced_unfold::unit>(yield.content).leaf, &axiom_);
    }
}

TEST_F(PudQueriesTest, ReplaceUnfoldedYieldsRefutedWhenZeroLiveCursors) {
    ON_CALL(candidate_, resume(_)).WillByDefault(
        [](pud_candidate_search_context& ctx) {
            ctx.cursor = nullptr;
        });
    queries_.adopt_axiom(&drain_);
    queries_.adopt_axiom(&axiom_);
    const std::vector<pud_forced_unfold> yields = drain_forced();
    bool saw_axiom_refuted = false;
    for (const pud_forced_unfold& yield : yields) {
        if (!std::holds_alternative<pud_forced_unfold::refuted>(yield.content))
            continue;
        if (std::get<pud_forced_unfold::refuted>(yield.content).leaf != &axiom_)
            continue;
        saw_axiom_refuted = true;
    }
    EXPECT_TRUE(saw_axiom_refuted);
}

TEST_F(PudQueriesTest, UnfoldSiteSkipsRefutedCursors) {
    queries_.adopt_axiom(&axiom_);
    queries_.adopt_axiom(&other_);
    ON_CALL(candidate_, resume(_)).WillByDefault(
        [this](pud_candidate_search_context& ctx) {
            if (ctx.cursor == &other_)
                ctx.cursor = nullptr;
        });
    ASSERT_EQ(queries_.unfold_site(&axiom_, 0).live.size(), 1u);
    EXPECT_EQ(queries_.unfold_site(&axiom_, 0).live[0]->cursor, &axiom_);
}

TEST_F(PudQueriesTest, ReplaceUnfoldedResumesWatchersOfTheDeadLeaf) {
    ON_CALL(get_added_body_goals_, get(&other_)).WillByDefault(ReturnRef(axiom_goals_));
    ON_CALL(get_added_body_goals_, get(&child_)).WillByDefault(ReturnRef(empty_goals_));
    ON_CALL(candidate_, resume(_)).WillByDefault(
        [this](pud_candidate_search_context& ctx) {
            if (ctx.cursor != &axiom_)
                return;
            ctx.witnesses = pud_witness_pair{
                {nested_, &body_, 1, &other_, &other_},
                {nested_, &body_, 1, &axiom_, &axiom_}};
        });
    queries_.adopt_axiom(&axiom_);
    queries_.adopt_axiom(&other_);
    EXPECT_CALL(witness_, resume(_)).Times(::testing::AtLeast(1));
    queries_.replace_unfolded(&other_, 0, {&child_});
}

TEST_F(PudQueriesTest, ReplaceUnfoldedClearsParentAndForksLeftoverOntoChild) {
    ON_CALL(get_added_body_goals_, get(&axiom_)).WillByDefault(ReturnRef(two_goals_));
    queries_.adopt_axiom(&axiom_);
    queries_.replace_unfolded(&axiom_, 0, {&child_});
    EXPECT_THROW(queries_.unfold_site(&axiom_, 0), std::out_of_range);
    EXPECT_EQ(ctx_at(&child_, 0)->body_goal, &leftover_);
    EXPECT_EQ(ctx_at(&child_, 1)->body_goal, &leftover_);
}

TEST_F(PudQueriesTest, AdoptEmptyBodyInstallsNoQueries) {
    ON_CALL(get_added_body_goals_, get(&axiom_)).WillByDefault(ReturnRef(empty_goals_));
    queries_.adopt_axiom(&drain_);
    queries_.adopt_axiom(&axiom_);
    const std::vector<pud_forced_unfold> yields = drain_forced();
    for (const pud_forced_unfold& yield : yields) {
        if (std::holds_alternative<pud_forced_unfold::unit>(yield.content))
            EXPECT_NE(std::get<pud_forced_unfold::unit>(yield.content).leaf, &axiom_);
        if (std::holds_alternative<pud_forced_unfold::refuted>(yield.content))
            EXPECT_NE(std::get<pud_forced_unfold::refuted>(yield.content).leaf, &axiom_);
    }
}

TEST_F(PudQueriesTest, AdoptMultiGoalInstallsOneQueryPerGoal) {
    ON_CALL(get_added_body_goals_, get(&axiom_)).WillByDefault(ReturnRef(two_goals_));
    queries_.adopt_axiom(&axiom_);
    EXPECT_EQ(ctx_at(&axiom_, 0)->body_goal, &body_);
    EXPECT_EQ(ctx_at(&axiom_, 1)->body_goal, &leftover_);
}

TEST_F(PudQueriesTest, AdoptSecondAxiomQueryRootContextsIncludePriors) {
    ON_CALL(get_added_body_goals_, get(&other_)).WillByDefault(ReturnRef(axiom_goals_));
    queries_.adopt_axiom(&axiom_);
    queries_.adopt_axiom(&other_);
    ASSERT_EQ(group_at(&other_, 0).size(), 2u);
    EXPECT_EQ(group_at(&other_, 0)[0]->cursor, &axiom_);
    EXPECT_EQ(group_at(&other_, 0)[1]->cursor, &other_);
}

TEST_F(PudQueriesTest, UnfoldSiteOnSecondBodyGoal) {
    ON_CALL(get_added_body_goals_, get(&axiom_)).WillByDefault(ReturnRef(two_goals_));
    queries_.adopt_axiom(&axiom_);
    ASSERT_EQ(queries_.unfold_site(&axiom_, 1).live.size(), 1u);
    EXPECT_EQ(queries_.unfold_site(&axiom_, 1).live[0]->cursor, &axiom_);
}

TEST_F(PudQueriesTest, ReplaceUnfoldedForksDistinctLeftoverAndChildGoals) {
    expr child_goal{expr::var{2}};
    std::vector<const expr*> child_with_goal{&child_goal};
    ON_CALL(get_added_body_goals_, get(&axiom_)).WillByDefault(ReturnRef(two_goals_));
    ON_CALL(get_added_body_goals_, get(&child_)).WillByDefault(ReturnRef(child_with_goal));
    queries_.adopt_axiom(&axiom_);
    queries_.replace_unfolded(&axiom_, 0, {&child_});
    EXPECT_EQ(ctx_at(&child_, 0)->body_goal, &leftover_);
    EXPECT_EQ(ctx_at(&child_, 1)->body_goal, &child_goal);
}

TEST_F(PudQueriesTest, ReplaceUnfoldedWithEmptyChildrenClearsParent) {
    queries_.adopt_axiom(&axiom_);
    queries_.replace_unfolded(&axiom_, 0, {});
    EXPECT_THROW(queries_.unfold_site(&axiom_, 0), std::out_of_range);
}

TEST_F(PudQueriesTest, ResumeDeadWitnessResumesCandidateWhenCursorMatchesEmptyEdges) {
    ON_CALL(get_added_body_goals_, get(&other_)).WillByDefault(ReturnRef(axiom_goals_));
    ON_CALL(get_added_body_goals_, get(&child_)).WillByDefault(ReturnRef(empty_goals_));
    queries_.adopt_axiom(&axiom_);
    queries_.adopt_axiom(&other_);
    EXPECT_CALL(candidate_, resume(_)).Times(::testing::AtLeast(1));
    queries_.replace_unfolded(&other_, 0, {&child_});
}

TEST_F(PudQueriesTest, ResumeDeadWitnessDropsFailedEdgesThenResumesCandidate) {
    bool other_is_live = true;
    ON_CALL(get_added_body_goals_, get(&other_)).WillByDefault(ReturnRef(axiom_goals_));
    ON_CALL(get_added_body_goals_, get(&child_)).WillByDefault(ReturnRef(empty_goals_));
    ON_CALL(candidate_, resume(_)).WillByDefault(
        [this, &other_is_live](pud_candidate_search_context& ctx) {
            if (ctx.cursor != &axiom_)
                return;
            if (other_is_live) {
                ctx.witnesses = pud_witness_pair{
                    {nested_, &body_, 1, &other_, &other_},
                    {nested_, &body_, 1, &axiom_, &axiom_}};
                return;
            }
            ctx.witnesses.reset();
        });
    ON_CALL(witness_, resume(_)).WillByDefault(
        [](pud_witness_search_context& edge) {
            edge.current = nullptr;
        });
    queries_.adopt_axiom(&axiom_);
    queries_.adopt_axiom(&other_);
    other_is_live = false;
    EXPECT_CALL(candidate_, resume(_)).Times(::testing::AtLeast(1));
    queries_.replace_unfolded(&other_, 0, {&child_});
    bool saw_other = false;
    for (pud_candidate_search_context* ctx : group_at(&axiom_, 0)) {
        if (!ctx->witnesses.has_value())
            continue;
        if (ctx->witnesses->a.current == &other_ || ctx->witnesses->b.current == &other_)
            saw_other = true;
    }
    EXPECT_FALSE(saw_other);
}

TEST_F(PudQueriesTest, ReplaceUnfoldedRefuteShortCircuitsLaterUnits) {
    ON_CALL(get_added_body_goals_, get(&axiom_)).WillByDefault(ReturnRef(two_goals_));
    ON_CALL(candidate_, resume(_)).WillByDefault(
        [this](pud_candidate_search_context& ctx) {
            if (ctx.body_goal == &body_)
                ctx.cursor = nullptr;
        });
    queries_.adopt_axiom(&drain_);
    queries_.adopt_axiom(&axiom_);
    const std::vector<pud_forced_unfold> yields = drain_forced();
    bool saw_axiom_refuted = false;
    bool saw_axiom_unit = false;
    for (const pud_forced_unfold& yield : yields) {
        if (std::holds_alternative<pud_forced_unfold::refuted>(yield.content)
                && std::get<pud_forced_unfold::refuted>(yield.content).leaf == &axiom_)
            saw_axiom_refuted = true;
        if (std::holds_alternative<pud_forced_unfold::unit>(yield.content)
                && std::get<pud_forced_unfold::unit>(yield.content).leaf == &axiom_)
            saw_axiom_unit = true;
    }
    EXPECT_TRUE(saw_axiom_refuted);
    EXPECT_FALSE(saw_axiom_unit);
}

TEST_F(PudQueriesTest, ReplaceUnfoldedEmitsUnitPerUnaryGoal) {
    ON_CALL(get_added_body_goals_, get(&axiom_)).WillByDefault(ReturnRef(two_goals_));
    ON_CALL(candidate_, resume(_)).WillByDefault(
        [this](pud_candidate_search_context& ctx) {
            if (ctx.cursor == &drain_)
                ctx.cursor = nullptr;
        });
    queries_.adopt_axiom(&drain_);
    queries_.adopt_axiom(&axiom_);
    const std::vector<pud_forced_unfold> yields = drain_forced();
    std::vector<size_t> axiom_unit_idxs;
    for (const pud_forced_unfold& yield : yields) {
        if (!std::holds_alternative<pud_forced_unfold::unit>(yield.content))
            continue;
        const auto unit = std::get<pud_forced_unfold::unit>(yield.content);
        if (unit.leaf != &axiom_)
            continue;
        axiom_unit_idxs.push_back(unit.body_goal_idx);
    }
    ASSERT_EQ(axiom_unit_idxs.size(), 2u);
    EXPECT_EQ(axiom_unit_idxs[0], 0u);
    EXPECT_EQ(axiom_unit_idxs[1], 1u);
}

TEST_F(PudQueriesTest, ReplaceUnfoldedOrdersDirtyLeavesByRuleId) {
    ON_CALL(get_added_body_goals_, get(&other_)).WillByDefault(ReturnRef(axiom_goals_));
    ON_CALL(candidate_, resume(_)).WillByDefault(
        [this](pud_candidate_search_context& ctx) {
            if (ctx.cursor != &drain_)
                ctx.cursor = nullptr;
        });
    queries_.adopt_axiom(&drain_);
    queries_.adopt_axiom(&other_);
    queries_.adopt_axiom(&axiom_);
    const std::vector<pud_forced_unfold> yields = drain_forced();
    std::vector<const pud_rule_id*> unit_leaves;
    for (const pud_forced_unfold& yield : yields) {
        if (!std::holds_alternative<pud_forced_unfold::unit>(yield.content))
            continue;
        const pud_rule_id* leaf = std::get<pud_forced_unfold::unit>(yield.content).leaf;
        if (leaf == &axiom_ || leaf == &other_)
            unit_leaves.push_back(leaf);
    }
    ASSERT_EQ(unit_leaves.size(), 2u);
    EXPECT_EQ(unit_leaves[0], &axiom_);
    EXPECT_EQ(unit_leaves[1], &other_);
}

TEST_F(PudQueriesTest, UnfoldSiteUnknownLeafThrows) {
    EXPECT_THROW(queries_.unfold_site(&axiom_, 0), std::out_of_range);
}

TEST_F(PudQueriesTest, WatchUnwatchAcrossAdoptReplace) {
    queries_.adopt_axiom(&axiom_);
    EXPECT_NO_THROW(queries_.unfold_site(&axiom_, 0));
    queries_.replace_unfolded(&axiom_, 0, {&child_});
    EXPECT_THROW(queries_.unfold_site(&axiom_, 0), std::out_of_range);
    EXPECT_NO_THROW(queries_.unfold_site(&child_, 0));
}

TEST_F(PudQueriesTest, StressManyAxiomsAttachToAllLeaves) {
    struct rec {
        pud_rule_id id;
        std::vector<const expr*> goals;
    };
    std::deque<rec> axioms;
    ON_CALL(get_added_body_goals_, get(_)).WillByDefault(
        [&](const pud_rule_id* id) -> const std::vector<const expr*>& {
            for (const rec& entry : axioms) {
                if (&entry.id == id)
                    return entry.goals;
            }
            return axiom_goals_;
        });
    ON_CALL(get_lvc_, get(_)).WillByDefault(Return(1u));
    for (int idx = 0; idx < 32; ++idx) {
        axioms.push_back(rec{
            pud_rule_id{pud_rule_id::axiom{static_cast<size_t>(idx)}},
            {&body_}});
        queries_.adopt_axiom(&axioms.back().id);
    }
    EXPECT_EQ(group_at(&axioms.front().id, 0).size(), 32u);
    EXPECT_EQ(group_at(&axioms.back().id, 0).size(), 32u);
}

TEST_F(PudQueriesTest, FuzzAdoptThenReplaceUnfoldSite) {
    struct rec {
        pud_rule_id id;
        std::vector<const expr*> goals;
        uint32_t lvc;
    };
    std::deque<rec> store;
    std::vector<const pud_rule_id*> owned;
    ON_CALL(get_added_body_goals_, get(_)).WillByDefault(
        [&](const pud_rule_id* id) -> const std::vector<const expr*>& {
            for (const rec& entry : store) {
                if (&entry.id == id)
                    return entry.goals;
            }
            return axiom_goals_;
        });
    ON_CALL(get_lvc_, get(_)).WillByDefault(
        [&](const pud_rule_id* id) {
            for (const rec& entry : store) {
                if (&entry.id == id)
                    return entry.lvc;
            }
            return 1u;
        });

    constexpr uint32_t k_seed = 42;
    std::mt19937 rng{k_seed};
    std::ostringstream log;
    for (int idx = 0; idx < 16; ++idx) {
        store.push_back(rec{
            pud_rule_id{pud_rule_id::axiom{store.size()}},
            {&body_},
            1});
        const pud_rule_id* id = &store.back().id;
        queries_.adopt_axiom(id);
        owned.push_back(id);
    }
    std::uniform_int_distribution<int> op_dist(0, 1);
    for (int step = 0; step < 64; ++step) {
        const int op = op_dist(rng);
        log << step << ':' << op << ' ';
        switch (op) {
        case 0:
            if (!owned.empty()) {
                const pud_rule_id* leaf = owned[rng() % owned.size()];
                queries_.unfold_site(leaf, 0);
            }
            break;
        case 1:
            if (!owned.empty()) {
                const size_t parent_idx = rng() % owned.size();
                const pud_rule_id* parent = owned[parent_idx];
                store.push_back(rec{
                    pud_rule_id{pud_rule_id::inference{parent, 0, parent}},
                    {&leftover_},
                    2});
                const pud_rule_id* child = &store.back().id;
                queries_.replace_unfolded(parent, 0, {child});
                owned.erase(owned.begin() + static_cast<std::ptrdiff_t>(parent_idx));
                owned.push_back(child);
                EXPECT_THROW(queries_.unfold_site(parent, 0), std::out_of_range)
                    << "seed " << k_seed << " log " << log.str();
            }
            break;
        }
        for (const pud_rule_id* leaf : owned)
            EXPECT_NO_THROW(queries_.unfold_site(leaf, 0))
                << "seed " << k_seed << " log " << log.str();
    }
}
