#include <deque>
#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_structure_watch_descender.hpp"

using ::testing::_;
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::UnorderedElementsAre;

namespace {

struct MockGetNodeAddedSpecializations {
    MOCK_METHOD((const std::vector<pud_specialization>&), get, (pud_node_id));
};

struct MockResolve {
    MOCK_METHOD(const expr*, resolve, (uint32_t));
};

using test_descender_t = pud_structure_watch_descender<MockGetNodeAddedSpecializations, MockResolve>;
using walk_t = coroutine<pud_mhsw_head_id, pud_structure_watch_descent>;

struct PudStructureWatchDescenderTest : public ::testing::Test {
    NiceMock<MockGetNodeAddedSpecializations> get_specs;
    NiceMock<MockResolve> resolve;
    test_descender_t descender{get_specs};
    std::deque<expr> pool;
    std::vector<pud_specialization> child_specs;
    std::vector<pud_specialization> grandchild_specs;
    pud_node_id root = 0;
    pud_node_id child = 1;
    pud_node_id grandchild = 2;
    uint32_t f = 100;
    uint32_t g = 101;
    uint32_t a = 102;

    void SetUp() override {
        ON_CALL(get_specs, get(child)).WillByDefault(ReturnRef(child_specs));
        ON_CALL(get_specs, get(grandchild)).WillByDefault(ReturnRef(grandchild_specs));
        for (uint32_t index = 0; index < 10; ++index)
            unbound(index);
    }

    const expr* var(uint32_t index) {
        pool.push_back(expr{.content = expr::var{.index = index}});
        return &pool.back();
    }

    const expr* functor(uint32_t id, std::vector<const expr*> args) {
        pool.push_back(expr{.content = expr::functor{.id = id, .args = std::move(args)}});
        return &pool.back();
    }

    void unbound(uint32_t index) {
        ON_CALL(resolve, resolve(index)).WillByDefault(Return(var(index)));
    }

    void resolves_to(uint32_t index, const expr* value) {
        ON_CALL(resolve, resolve(index)).WillByDefault(Return(value));
    }

    pud_specialization rep_instantiated(uint32_t rep, const expr* value) {
        return pud_specialization{.var_idx = rep, .value = value};
    }

    pud_specialization rep_updated(uint32_t rep_a, uint32_t rep_b) {
        return pud_specialization{.var_idx = rep_a, .value = var(rep_b)};
    }

    static std::vector<pud_mhsw_head_id> drain(walk_t& walk) {
        std::vector<pud_mhsw_head_id> changed;
        while (std::optional<pud_mhsw_head_id> head_id = walk.next())
            changed.push_back(head_id.value());
        return changed;
    }

    static std::vector<uint32_t> reps_of(const pud_structure_watch_descent& descent, pud_mhsw_head_id head_id) {
        std::vector<uint32_t> reps;
        const immer::set<uint32_t>* found = descent.head_to_reps.find(head_id);
        if (!found)
            return reps;
        for (uint32_t rep : *found)
            reps.push_back(rep);
        return reps;
    }

    static std::vector<pud_mhsw_head_id> heads_of(const pud_structure_watch_descent& descent, uint32_t rep) {
        std::vector<pud_mhsw_head_id> heads;
        const immer::set<pud_mhsw_head_id>* found = descent.rep_to_heads.find(rep);
        if (!found)
            return heads;
        for (pud_mhsw_head_id head_id : *found)
            heads.push_back(head_id);
        return heads;
    }

    pud_structure_watch_descent watched(const std::vector<const expr*>& exprs) {
        pud_structure_watch_descent descent = descender.root_descent(root);
        for (const expr* watched_expr : exprs)
            descent = descender.watch(descent, resolve, watched_expr);
        return descent;
    }
};

// ── root_descent ─────────────────────────────────────────────────────────────

TEST_F(PudStructureWatchDescenderTest, RootDescentWatchesNothing) {
    pud_structure_watch_descent descent = descender.root_descent(root);
    EXPECT_EQ(descent.next_head_id, 0u);
    EXPECT_TRUE(descent.rep_to_heads.empty());
    EXPECT_TRUE(descent.head_to_reps.empty());
}

// ── watch ────────────────────────────────────────────────────────────────────

TEST_F(PudStructureWatchDescenderTest, WatchAssignsSequentialHeadIds) {
    pud_structure_watch_descent descent = descender.root_descent(root);
    descent = descender.watch(descent, resolve, var(0));
    EXPECT_EQ(descent.next_head_id, 1u);
    descent = descender.watch(descent, resolve, var(1));
    EXPECT_EQ(descent.next_head_id, 2u);
    EXPECT_THAT(reps_of(descent, 0), UnorderedElementsAre(0u));
    EXPECT_THAT(reps_of(descent, 1), UnorderedElementsAre(1u));
}

TEST_F(PudStructureWatchDescenderTest, WatchRecordsEveryVarOfTheExpression) {
    pud_structure_watch_descent descent = watched({functor(f, {var(0), functor(g, {var(1), var(2)})})});
    EXPECT_THAT(reps_of(descent, 0), UnorderedElementsAre(0u, 1u, 2u));
    EXPECT_THAT(heads_of(descent, 0), UnorderedElementsAre(0u));
    EXPECT_THAT(heads_of(descent, 1), UnorderedElementsAre(0u));
    EXPECT_THAT(heads_of(descent, 2), UnorderedElementsAre(0u));
}

TEST_F(PudStructureWatchDescenderTest, WatchRecordsRepeatedVarOnce) {
    pud_structure_watch_descent descent = watched({functor(f, {var(3), var(3)})});
    EXPECT_THAT(reps_of(descent, 0), UnorderedElementsAre(3u));
}

TEST_F(PudStructureWatchDescenderTest, WatchRecordsTheRepresentativeNotTheVar) {
    resolves_to(5, var(4));
    pud_structure_watch_descent descent = watched({functor(f, {var(5)})});
    EXPECT_THAT(reps_of(descent, 0), UnorderedElementsAre(4u));
    EXPECT_THAT(heads_of(descent, 5), IsEmpty());
}

TEST_F(PudStructureWatchDescenderTest, WatchLooksThroughBoundVars) {
    resolves_to(2, functor(g, {var(3), var(6)}));
    pud_structure_watch_descent descent = watched({functor(f, {var(2), var(1)})});
    EXPECT_THAT(reps_of(descent, 0), UnorderedElementsAre(1u, 3u, 6u));
    EXPECT_THAT(heads_of(descent, 2), IsEmpty());
}

TEST_F(PudStructureWatchDescenderTest, WatchLooksThroughNestedBindings) {
    resolves_to(2, functor(g, {var(3)}));
    resolves_to(3, functor(g, {var(7)}));
    pud_structure_watch_descent descent = watched({var(2)});
    EXPECT_THAT(reps_of(descent, 0), UnorderedElementsAre(7u));
}

TEST_F(PudStructureWatchDescenderTest, WatchOfGroundExpressionHasNoReps) {
    pud_structure_watch_descent descent = watched({functor(f, {functor(a, {})})});
    EXPECT_EQ(descent.next_head_id, 1u);
    EXPECT_THAT(reps_of(descent, 0), IsEmpty());
    EXPECT_TRUE(descent.rep_to_heads.empty());
}

TEST_F(PudStructureWatchDescenderTest, WatchSharedVarLinksBothHeads) {
    pud_structure_watch_descent descent = watched({functor(f, {var(0)}), functor(g, {var(0), var(1)})});
    EXPECT_THAT(heads_of(descent, 0), UnorderedElementsAre(0u, 1u));
    EXPECT_THAT(heads_of(descent, 1), UnorderedElementsAre(1u));
}

TEST_F(PudStructureWatchDescenderTest, WatchLeavesTheInputDescentUntouched) {
    pud_structure_watch_descent before = descender.root_descent(root);
    pud_structure_watch_descent after = descender.watch(before, resolve, var(0));
    EXPECT_EQ(before.next_head_id, 0u);
    EXPECT_TRUE(before.head_to_reps.empty());
    EXPECT_EQ(after.next_head_id, 1u);
}

// ── unwatch ──────────────────────────────────────────────────────────────────

TEST_F(PudStructureWatchDescenderTest, UnwatchForgetsTheHead) {
    pud_structure_watch_descent descent = watched({functor(f, {var(0), var(1)})});
    descent = descender.unwatch(descent, 0);
    EXPECT_TRUE(descent.head_to_reps.empty());
    EXPECT_TRUE(descent.rep_to_heads.empty());
}

TEST_F(PudStructureWatchDescenderTest, UnwatchKeepsOtherHeadsOnSharedVars) {
    pud_structure_watch_descent descent = watched({functor(f, {var(0)}), functor(g, {var(0), var(1)})});
    descent = descender.unwatch(descent, 1);
    EXPECT_THAT(heads_of(descent, 0), UnorderedElementsAre(0u));
    EXPECT_THAT(heads_of(descent, 1), IsEmpty());
    EXPECT_THAT(reps_of(descent, 0), UnorderedElementsAre(0u));
}

TEST_F(PudStructureWatchDescenderTest, UnwatchDoesNotReuseTheHeadId) {
    pud_structure_watch_descent descent = watched({var(0)});
    descent = descender.unwatch(descent, 0);
    EXPECT_EQ(descent.next_head_id, 1u);
}

TEST_F(PudStructureWatchDescenderTest, UnwatchedHeadIsNeverYielded) {
    pud_structure_watch_descent descent = watched({var(0), var(0)});
    descent = descender.unwatch(descent, 0);
    child_specs = {rep_instantiated(0, functor(a, {}))};
    walk_t walk = descender.descend(descent, resolve, child);
    EXPECT_THAT(drain(walk), UnorderedElementsAre(1u));
}

// ── descend: no change ───────────────────────────────────────────────────────

TEST_F(PudStructureWatchDescenderTest, DescendLooksUpTheChildSpecializations) {
    EXPECT_CALL(get_specs, get(child)).WillOnce(ReturnRef(child_specs));
    walk_t walk = descender.descend(watched({var(0)}), resolve, child);
    drain(walk);
}

TEST_F(PudStructureWatchDescenderTest, NoSpecializationsYieldsNothingAndKeepsTheDescent) {
    pud_structure_watch_descent descent = watched({functor(f, {var(0), var(1)})});
    walk_t walk = descender.descend(descent, resolve, child);
    EXPECT_THAT(drain(walk), IsEmpty());
    EXPECT_EQ(walk.result().next_head_id, descent.next_head_id);
    EXPECT_THAT(reps_of(walk.result(), 0), UnorderedElementsAre(0u, 1u));
}

TEST_F(PudStructureWatchDescenderTest, InstantiatingAnUnwatchedRepYieldsNothing) {
    child_specs = {rep_instantiated(5, functor(a, {}))};
    walk_t walk = descender.descend(watched({functor(f, {var(0), var(1)})}), resolve, child);
    EXPECT_THAT(drain(walk), IsEmpty());
}

TEST_F(PudStructureWatchDescenderTest, UpdatingTwoUnwatchedRepsYieldsNothing) {
    child_specs = {rep_updated(5, 6)};
    walk_t walk = descender.descend(watched({functor(f, {var(0), var(1)})}), resolve, child);
    EXPECT_THAT(drain(walk), IsEmpty());
}

TEST_F(PudStructureWatchDescenderTest, UpdatingAWatchedRepWithAnOutsideRepYieldsNothing) {
    child_specs = {rep_updated(1, 5)};
    walk_t walk = descender.descend(watched({functor(f, {var(0), var(1)})}), resolve, child);
    EXPECT_THAT(drain(walk), IsEmpty());
}

TEST_F(PudStructureWatchDescenderTest, UpdatingRepsOfDifferentHeadsYieldsNothing) {
    child_specs = {rep_updated(0, 1)};
    walk_t walk = descender.descend(watched({var(0), var(1)}), resolve, child);
    EXPECT_THAT(drain(walk), IsEmpty());
}

// ── descend: rep instantiated ────────────────────────────────────────────────

TEST_F(PudStructureWatchDescenderTest, InstantiatingAWatchedRepYieldsItsHead) {
    child_specs = {rep_instantiated(1, functor(a, {}))};
    walk_t walk = descender.descend(watched({functor(f, {var(0), var(1)})}), resolve, child);
    EXPECT_THAT(drain(walk), UnorderedElementsAre(0u));
}

TEST_F(PudStructureWatchDescenderTest, InstantiatingYieldsEveryHeadWatchingThatRep) {
    child_specs = {rep_instantiated(0, functor(a, {}))};
    walk_t walk = descender.descend(
        watched({functor(f, {var(0)}), functor(g, {var(0), var(1)}), var(1)}), resolve, child);
    EXPECT_THAT(drain(walk), UnorderedElementsAre(0u, 1u));
}

TEST_F(PudStructureWatchDescenderTest, GroundInstantiationDropsTheRep) {
    child_specs = {rep_instantiated(0, functor(a, {}))};
    walk_t walk = descender.descend(watched({functor(f, {var(0), var(1)})}), resolve, child);
    drain(walk);
    EXPECT_THAT(reps_of(walk.result(), 0), UnorderedElementsAre(1u));
    EXPECT_THAT(heads_of(walk.result(), 0), IsEmpty());
}

TEST_F(PudStructureWatchDescenderTest, InstantiationReplacesTheRepWithTheIntroducedReps) {
    child_specs = {rep_instantiated(0, functor(g, {var(3), var(4)}))};
    walk_t walk = descender.descend(watched({functor(f, {var(0), var(1)})}), resolve, child);
    drain(walk);
    EXPECT_THAT(reps_of(walk.result(), 0), UnorderedElementsAre(1u, 3u, 4u));
    EXPECT_THAT(heads_of(walk.result(), 3), UnorderedElementsAre(0u));
    EXPECT_THAT(heads_of(walk.result(), 4), UnorderedElementsAre(0u));
}

TEST_F(PudStructureWatchDescenderTest, InstantiationIntroducesRepresentativesOfItsArgs) {
    resolves_to(4, var(3));
    child_specs = {rep_instantiated(0, functor(g, {var(4)}))};
    walk_t walk = descender.descend(watched({var(0)}), resolve, child);
    drain(walk);
    EXPECT_THAT(reps_of(walk.result(), 0), UnorderedElementsAre(3u));
}

TEST_F(PudStructureWatchDescenderTest, InstantiationIntroducingAnAlreadyWatchedRepKeepsItOnce) {
    child_specs = {rep_instantiated(0, functor(g, {var(1)}))};
    walk_t walk = descender.descend(watched({functor(f, {var(0), var(1)})}), resolve, child);
    EXPECT_THAT(drain(walk), UnorderedElementsAre(0u));
    EXPECT_THAT(reps_of(walk.result(), 0), UnorderedElementsAre(1u));
    EXPECT_THAT(heads_of(walk.result(), 1), UnorderedElementsAre(0u));
}

TEST_F(PudStructureWatchDescenderTest, IntroducedRepsAreWatchedAtTheNextLevel) {
    child_specs = {rep_instantiated(0, functor(g, {var(3)}))};
    walk_t child_walk = descender.descend(watched({var(0)}), resolve, child);
    ASSERT_THAT(drain(child_walk), UnorderedElementsAre(0u));

    grandchild_specs = {rep_instantiated(3, functor(a, {}))};
    walk_t grandchild_walk = descender.descend(child_walk.result(), resolve, grandchild);
    EXPECT_THAT(drain(grandchild_walk), UnorderedElementsAre(0u));
}

TEST_F(PudStructureWatchDescenderTest, InstantiatedRepIsNoLongerWatched) {
    child_specs = {rep_instantiated(0, functor(a, {}))};
    walk_t child_walk = descender.descend(watched({functor(f, {var(0), var(1)})}), resolve, child);
    drain(child_walk);

    grandchild_specs = {rep_updated(0, 1)};
    walk_t grandchild_walk = descender.descend(child_walk.result(), resolve, grandchild);
    EXPECT_THAT(drain(grandchild_walk), IsEmpty());
}

// ── descend: rep updated ─────────────────────────────────────────────────────

TEST_F(PudStructureWatchDescenderTest, UpdatingTwoRepsOfTheSameHeadYieldsIt) {
    child_specs = {rep_updated(1, 0)};
    walk_t walk = descender.descend(watched({functor(f, {var(0), var(1)})}), resolve, child);
    EXPECT_THAT(drain(walk), UnorderedElementsAre(0u));
}

TEST_F(PudStructureWatchDescenderTest, RepUpdateDirectionDoesNotMatter) {
    child_specs = {rep_updated(0, 1)};
    walk_t walk = descender.descend(watched({functor(f, {var(0), var(1)})}), resolve, child);
    EXPECT_THAT(drain(walk), UnorderedElementsAre(0u));
}

TEST_F(PudStructureWatchDescenderTest, CollapsedHeadIsWatchedUnderTheSurvivingRep) {
    child_specs = {rep_updated(1, 0)};
    walk_t walk = descender.descend(watched({functor(f, {var(0), var(1)})}), resolve, child);
    drain(walk);
    EXPECT_THAT(reps_of(walk.result(), 0), UnorderedElementsAre(0u));
    EXPECT_THAT(heads_of(walk.result(), 0), UnorderedElementsAre(0u));
    EXPECT_THAT(heads_of(walk.result(), 1), IsEmpty());
}

TEST_F(PudStructureWatchDescenderTest, RepUpdateMovesDemotedHeadsToTheSurvivingRep) {
    child_specs = {rep_updated(4, 1)};
    walk_t walk = descender.descend(watched({functor(f, {var(4)})}), resolve, child);
    EXPECT_THAT(drain(walk), IsEmpty());
    EXPECT_THAT(reps_of(walk.result(), 0), UnorderedElementsAre(1u));
    EXPECT_THAT(heads_of(walk.result(), 1), UnorderedElementsAre(0u));
    EXPECT_THAT(heads_of(walk.result(), 4), IsEmpty());
}

TEST_F(PudStructureWatchDescenderTest, RepUpdateKeepsSurvivingRepHeadsInPlace) {
    child_specs = {rep_updated(1, 4)};
    walk_t walk = descender.descend(watched({functor(f, {var(1)})}), resolve, child);
    EXPECT_THAT(drain(walk), IsEmpty());
    EXPECT_THAT(reps_of(walk.result(), 0), UnorderedElementsAre(1u));
    EXPECT_THAT(heads_of(walk.result(), 1), UnorderedElementsAre(0u));
}

TEST_F(PudStructureWatchDescenderTest, RepUpdateMergesHeadsOfBothClasses) {
    child_specs = {rep_updated(0, 1)};
    walk_t walk = descender.descend(watched({var(0), var(1)}), resolve, child);
    drain(walk);
    EXPECT_THAT(heads_of(walk.result(), 0), UnorderedElementsAre(0u, 1u));
}

TEST_F(PudStructureWatchDescenderTest, MergedClassInstantiationYieldsHeadsOfBothSides) {
    child_specs = {rep_updated(0, 1)};
    walk_t child_walk = descender.descend(watched({var(0), var(1)}), resolve, child);
    ASSERT_THAT(drain(child_walk), IsEmpty());

    grandchild_specs = {rep_instantiated(0, functor(a, {}))};
    walk_t grandchild_walk = descender.descend(child_walk.result(), resolve, grandchild);
    EXPECT_THAT(drain(grandchild_walk), UnorderedElementsAre(0u, 1u));
}

TEST_F(PudStructureWatchDescenderTest, RepUpdateYieldsOnlyTheHeadsThatCollapse) {
    child_specs = {rep_updated(0, 1)};
    walk_t walk = descender.descend(
        watched({functor(f, {var(0), var(1)}), var(1), functor(g, {var(0), var(2)})}), resolve, child);
    EXPECT_THAT(drain(walk), UnorderedElementsAre(0u));
}

TEST_F(PudStructureWatchDescenderTest, ChainedRepUpdatesAtOneNodeCollapseAcrossThreeReps) {
    child_specs = {rep_updated(2, 1), rep_updated(1, 0)};
    walk_t walk = descender.descend(watched({functor(f, {var(0), var(2)})}), resolve, child);
    EXPECT_THAT(drain(walk), UnorderedElementsAre(0u));
    EXPECT_THAT(reps_of(walk.result(), 0), UnorderedElementsAre(0u));
}

// ── descend: structure sets hold only reps ───────────────────────────────────

TEST_F(PudStructureWatchDescenderTest, RepUpdateWithANonRepSideBreaksTheInvariant) {
    pud_structure_watch_descent descent = watched({var(1)});
    resolves_to(1, var(0));
    resolves_to(2, var(0));
    child_specs = {rep_updated(2, 1)};
    walk_t walk = descender.descend(descent, resolve, child);
    EXPECT_THROW(drain(walk), std::logic_error);
}

TEST_F(PudStructureWatchDescenderTest, SurvivingRepInstantiatedAtTheSameNodeKeepsTheInvariant) {
    pud_structure_watch_descent descent = watched({functor(f, {var(0), var(1)})});
    resolves_to(0, functor(a, {}));
    resolves_to(1, functor(a, {}));
    child_specs = {rep_updated(1, 0), rep_instantiated(0, functor(a, {}))};
    walk_t walk = descender.descend(descent, resolve, child);
    EXPECT_THAT(drain(walk), UnorderedElementsAre(0u));
    EXPECT_THAT(reps_of(walk.result(), 0), IsEmpty());
}

TEST_F(PudStructureWatchDescenderTest, IntroducedRepsAreRepresentatives) {
    resolves_to(4, var(3));
    child_specs = {rep_instantiated(0, functor(g, {var(4), var(5)}))};
    walk_t walk = descender.descend(watched({var(0)}), resolve, child);
    drain(walk);
    EXPECT_THAT(reps_of(walk.result(), 0), UnorderedElementsAre(3u, 5u));
}

// ── descend: yielding ────────────────────────────────────────────────────────

TEST_F(PudStructureWatchDescenderTest, HeadIsYieldedOncePerDescend) {
    child_specs = {rep_instantiated(0, functor(a, {})), rep_instantiated(1, functor(a, {}))};
    walk_t walk = descender.descend(watched({functor(f, {var(0), var(1)})}), resolve, child);
    EXPECT_THAT(drain(walk), UnorderedElementsAre(0u));
}

TEST_F(PudStructureWatchDescenderTest, HeadChangedByRepUpdateAndRepInstantiationIsYieldedOnce) {
    child_specs = {rep_updated(1, 0), rep_instantiated(0, functor(a, {}))};
    walk_t walk = descender.descend(watched({functor(f, {var(0), var(1)})}), resolve, child);
    EXPECT_THAT(drain(walk), UnorderedElementsAre(0u));
}

TEST_F(PudStructureWatchDescenderTest, HeadYieldedAtChildCanBeYieldedAgainAtGrandchild) {
    child_specs = {rep_instantiated(0, functor(a, {}))};
    walk_t child_walk = descender.descend(watched({functor(f, {var(0), var(1)})}), resolve, child);
    ASSERT_THAT(drain(child_walk), UnorderedElementsAre(0u));

    grandchild_specs = {rep_instantiated(1, functor(a, {}))};
    walk_t grandchild_walk = descender.descend(child_walk.result(), resolve, grandchild);
    EXPECT_THAT(drain(grandchild_walk), UnorderedElementsAre(0u));
}

TEST_F(PudStructureWatchDescenderTest, DescentIsReturnedAfterAllYields) {
    child_specs = {rep_instantiated(0, functor(a, {}))};
    walk_t walk = descender.descend(watched({var(0)}), resolve, child);
    EXPECT_THROW(walk.result(), std::logic_error);
    drain(walk);
    EXPECT_TRUE(walk.done());
    EXPECT_EQ(walk.result().next_head_id, 1u);
}

// ── descend: persistence ─────────────────────────────────────────────────────

TEST_F(PudStructureWatchDescenderTest, ParentDescentIsUntouchedByDescend) {
    pud_structure_watch_descent parent = watched({functor(f, {var(0), var(1)})});
    child_specs = {rep_instantiated(0, functor(g, {var(5)})), rep_updated(1, 2)};
    walk_t walk = descender.descend(parent, resolve, child);
    drain(walk);
    EXPECT_THAT(reps_of(parent, 0), UnorderedElementsAre(0u, 1u));
    EXPECT_THAT(heads_of(parent, 0), UnorderedElementsAre(0u));
    EXPECT_THAT(heads_of(parent, 5), IsEmpty());
}

TEST_F(PudStructureWatchDescenderTest, SiblingsDescendIndependently) {
    pud_structure_watch_descent parent = watched({var(0), var(1)});
    child_specs = {rep_instantiated(0, functor(a, {}))};
    grandchild_specs = {rep_instantiated(1, functor(a, {}))};

    walk_t left = descender.descend(parent, resolve, child);
    walk_t right = descender.descend(parent, resolve, grandchild);

    EXPECT_THAT(drain(left), UnorderedElementsAre(0u));
    EXPECT_THAT(drain(right), UnorderedElementsAre(1u));
    EXPECT_THAT(reps_of(left.result(), 1), UnorderedElementsAre(1u));
    EXPECT_THAT(reps_of(right.result(), 0), UnorderedElementsAre(0u));
}

TEST_F(PudStructureWatchDescenderTest, WatchAfterDescendGetsTheNextId) {
    child_specs = {rep_instantiated(0, functor(a, {}))};
    walk_t walk = descender.descend(watched({var(0)}), resolve, child);
    drain(walk);
    pud_structure_watch_descent descent = descender.watch(walk.result(), resolve, var(2));
    EXPECT_EQ(descent.next_head_id, 2u);
    EXPECT_THAT(reps_of(descent, 1), UnorderedElementsAre(2u));
}

}  // namespace
