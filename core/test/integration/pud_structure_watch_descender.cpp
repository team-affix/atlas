#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "functor_fixture.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/expr_pool_ubs_factories.hpp"
#include "infrastructure/pud_binding_descender.hpp"
#include "infrastructure/pud_node_added_specializations.hpp"
#include "infrastructure/pud_structure_watch_descender.hpp"
#include "infrastructure/pud_transient_ubs_state.hpp"
#include "value_objects/pud_specialization.hpp"

using ::testing::IsEmpty;
using ::testing::UnorderedElementsAre;

struct PudStructureWatchDescenderIntegrationTest : public ::testing::Test {
    using bind_map_t = ubs_transient_bind_map<expr_pool_ubs_var_factory, expr_pool_ubs_functor_factory>;
    using binding_descender_t = pud_binding_descender<
        bind_map_t,
        pud_node_added_specializations,
        expr_pool_ubs_var_factory,
        expr_pool_ubs_functor_factory>;
    using watch_descender_t = pud_structure_watch_descender<pud_node_added_specializations, bind_map_t>;
    using walk_t = coroutine<pud_mhsw_head_id, pud_structure_watch_descent>;

    struct level {
        pud_binding_descent binding;
        pud_structure_watch_descent watch;
        std::vector<pud_mhsw_head_id> changed;
    };

    test_functors                  functors;
    expr_pool                      exprs;
    expr_pool_ubs_var_factory      ubs_var_factory_{exprs};
    expr_pool_ubs_functor_factory  ubs_functor_factory_{exprs};
    pud_node_added_specializations node_specs_;
    binding_descender_t            binding_{node_specs_, ubs_var_factory_, ubs_functor_factory_};
    watch_descender_t              watcher_{node_specs_};
    pud_node_id root = 0;
    pud_node_id child = 1;
    pud_node_id grandchild = 2;
    pud_node_id sibling = 3;

    const expr* var(uint32_t index) { return exprs.make_var(index); }

    const expr* functor(const char* name, std::vector<const expr*> args) {
        return exprs.make_functor(functors.id(name), std::move(args));
    }

    pud_specialization rep_instantiated(uint32_t rep, const expr* value) {
        return pud_specialization{.var_idx = rep, .value = value};
    }

    pud_specialization rep_updated(uint32_t rep_a, uint32_t rep_b) {
        return pud_specialization{.var_idx = rep_a, .value = var(rep_b)};
    }

    level at_root() {
        return level{
            .binding = binding_.root_descent(root),
            .watch = watcher_.root_descent(root),
            .changed = {},
        };
    }

    level watch(level at, const expr* watched) {
        pud_transient_ubs_state transient{at.binding.ubs_state};
        bind_map_t bind_map = transient.make_bind_map(ubs_var_factory_, ubs_functor_factory_);
        at.watch = watcher_.watch(at.watch, bind_map, watched);
        return at;
    }

    level descend(const level& parent, pud_node_id node) {
        level result{.binding = binding_.descend(parent.binding, node), .watch = {}, .changed = {}};
        pud_transient_ubs_state transient{result.binding.ubs_state};
        bind_map_t bind_map = transient.make_bind_map(ubs_var_factory_, ubs_functor_factory_);
        walk_t walk = watcher_.descend(parent.watch, bind_map, node);
        while (std::optional<pud_mhsw_head_id> head_id = walk.next())
            result.changed.push_back(head_id.value());
        result.watch = walk.result();
        return result;
    }
};

// ── rep instantiated ─────────────────────────────────────────────────────────

TEST_F(PudStructureWatchDescenderIntegrationTest, InstantiatingAWatchedRepYields) {
    level root_level = watch(at_root(), functor("f", {var(0), var(1)}));
    node_specs_.store(child, {rep_instantiated(0, functor("a", {}))});
    EXPECT_THAT(descend(root_level, child).changed, UnorderedElementsAre(0u));
}

TEST_F(PudStructureWatchDescenderIntegrationTest, InstantiatingAnOutsideRepDoesNotYield) {
    level root_level = watch(at_root(), functor("f", {var(0), var(1)}));
    node_specs_.store(child, {rep_instantiated(5, functor("a", {}))});
    EXPECT_THAT(descend(root_level, child).changed, IsEmpty());
}

TEST_F(PudStructureWatchDescenderIntegrationTest, InstantiationYieldsEveryExpressionSharingTheRep) {
    level root_level = at_root();
    root_level = watch(root_level, functor("f", {var(0)}));
    root_level = watch(root_level, functor("g", {var(0), var(1)}));
    root_level = watch(root_level, functor("h", {var(1)}));
    node_specs_.store(child, {rep_instantiated(0, functor("a", {}))});
    EXPECT_THAT(descend(root_level, child).changed, UnorderedElementsAre(0u, 1u));
}

TEST_F(PudStructureWatchDescenderIntegrationTest, RepsIntroducedByAnInstantiationAreWatched) {
    level root_level = watch(at_root(), functor("g", {var(0)}));
    node_specs_.store(child, {rep_instantiated(0, functor("f", {var(3)}))});
    node_specs_.store(grandchild, {rep_instantiated(3, functor("a", {}))});
    level child_level = descend(root_level, child);
    ASSERT_THAT(child_level.changed, UnorderedElementsAre(0u));
    EXPECT_THAT(descend(child_level, grandchild).changed, UnorderedElementsAre(0u));
}

TEST_F(PudStructureWatchDescenderIntegrationTest, GroundInstantiationStopsWatchingThatRep) {
    level root_level = watch(at_root(), functor("f", {var(0), var(1)}));
    node_specs_.store(child, {rep_instantiated(0, functor("a", {}))});
    node_specs_.store(grandchild, {rep_instantiated(4, functor("b", {}))});
    level child_level = descend(root_level, child);
    EXPECT_THAT(descend(child_level, grandchild).changed, IsEmpty());
}

// ── rep updated ──────────────────────────────────────────────────────────────

TEST_F(PudStructureWatchDescenderIntegrationTest, UpdatingTwoRepsOfTheExpressionYields) {
    level root_level = watch(at_root(), functor("f", {var(0), var(1)}));
    node_specs_.store(child, {rep_updated(1, 0)});
    EXPECT_THAT(descend(root_level, child).changed, UnorderedElementsAre(0u));
}

TEST_F(PudStructureWatchDescenderIntegrationTest, UpdatingWithAnOutsideRepDoesNotYield) {
    level root_level = watch(at_root(), functor("f", {var(0), var(1)}));
    node_specs_.store(child, {rep_updated(1, 5)});
    EXPECT_THAT(descend(root_level, child).changed, IsEmpty());
}

TEST_F(PudStructureWatchDescenderIntegrationTest, DemotedRepIsWatchedThroughTheSurvivingRep) {
    level root_level = watch(at_root(), functor("f", {var(4)}));
    node_specs_.store(child, {rep_updated(4, 1)});
    node_specs_.store(grandchild, {rep_instantiated(1, functor("a", {}))});
    level child_level = descend(root_level, child);
    ASSERT_THAT(child_level.changed, IsEmpty());
    EXPECT_THAT(descend(child_level, grandchild).changed, UnorderedElementsAre(0u));
}

TEST_F(PudStructureWatchDescenderIntegrationTest, MergingRepsOfTwoExpressionsLinksBothToTheSurvivingRep) {
    level root_level = at_root();
    root_level = watch(root_level, functor("f", {var(0)}));
    root_level = watch(root_level, functor("g", {var(1)}));
    node_specs_.store(child, {rep_updated(1, 0)});
    node_specs_.store(grandchild, {rep_instantiated(0, functor("a", {}))});
    level child_level = descend(root_level, child);
    ASSERT_THAT(child_level.changed, IsEmpty());
    EXPECT_THAT(descend(child_level, grandchild).changed, UnorderedElementsAre(0u, 1u));
}

TEST_F(PudStructureWatchDescenderIntegrationTest, CollapseAcrossTwoLevels) {
    level root_level = watch(at_root(), functor("f", {var(0), var(2)}));
    node_specs_.store(child, {rep_updated(2, 1)});
    node_specs_.store(grandchild, {rep_updated(1, 0)});
    level child_level = descend(root_level, child);
    ASSERT_THAT(child_level.changed, IsEmpty());
    EXPECT_THAT(descend(child_level, grandchild).changed, UnorderedElementsAre(0u));
}

// ── watching below the root ──────────────────────────────────────────────────

TEST_F(PudStructureWatchDescenderIntegrationTest, WatchUsesTheBindingsAtThatNode) {
    node_specs_.store(child, {rep_instantiated(0, functor("f", {var(3)}))});
    node_specs_.store(grandchild, {rep_instantiated(3, functor("a", {}))});
    level child_level = watch(descend(at_root(), child), functor("h", {var(0)}));
    EXPECT_THAT(descend(child_level, grandchild).changed, UnorderedElementsAre(0u));
}

TEST_F(PudStructureWatchDescenderIntegrationTest, WatchAfterRepUpdateUsesTheSurvivingRep) {
    node_specs_.store(child, {rep_updated(4, 1)});
    node_specs_.store(grandchild, {rep_updated(1, 0)});
    level child_level = watch(descend(at_root(), child), functor("h", {var(4), var(0)}));
    EXPECT_THAT(descend(child_level, grandchild).changed, UnorderedElementsAre(0u));
}

// ── many specializations at one node ─────────────────────────────────────────

TEST_F(PudStructureWatchDescenderIntegrationTest, UnrelatedBindingsLeaveStructureAlone) {
    level root_level = watch(at_root(), functor("f", {var(0), var(1)}));
    node_specs_.store(child, {
        rep_updated(3, 2),
        rep_instantiated(4, functor("a", {})),
        rep_updated(1, 5),
    });
    EXPECT_THAT(descend(root_level, child).changed, IsEmpty());
}

TEST_F(PudStructureWatchDescenderIntegrationTest, ExpressionChangedTwiceAtOneNodeIsYieldedOnce) {
    level root_level = watch(at_root(), functor("f", {var(0), var(1), var(2)}));
    node_specs_.store(child, {rep_updated(1, 0), rep_instantiated(2, functor("a", {}))});
    EXPECT_THAT(descend(root_level, child).changed, UnorderedElementsAre(0u));
}

TEST_F(PudStructureWatchDescenderIntegrationTest, InstantiationArgsResolveThroughEarlierBindings) {
    level root_level = watch(at_root(), functor("f", {var(0), var(1)}));
    node_specs_.store(child, {rep_updated(3, 1)});
    node_specs_.store(grandchild, {rep_instantiated(0, functor("g", {var(3)}))});
    level child_level = descend(root_level, child);
    level grandchild_level = descend(child_level, grandchild);
    ASSERT_THAT(grandchild_level.changed, UnorderedElementsAre(0u));
    const immer::set<uint32_t>* reps = grandchild_level.watch.head_to_reps.find(0);
    ASSERT_NE(reps, nullptr);
    EXPECT_EQ(reps->size(), 1u);
    EXPECT_EQ(reps->count(1), 1u);
}

// ── structure sets hold only reps ────────────────────────────────────────────

TEST_F(PudStructureWatchDescenderIntegrationTest, EveryStructureSetHoldsOnlyRepsAfterDescending) {
    level root_level = at_root();
    root_level = watch(root_level, functor("f", {var(0), var(4), var(6)}));
    root_level = watch(root_level, functor("g", {var(2), var(5)}));
    node_specs_.store(child, {rep_updated(4, 2), rep_instantiated(6, functor("h", {var(7), var(8)}))});
    node_specs_.store(grandchild, {rep_updated(8, 0), rep_updated(5, 7)});
    level grandchild_level = descend(descend(root_level, child), grandchild);

    pud_transient_ubs_state transient{grandchild_level.binding.ubs_state};
    bind_map_t bind_map = transient.make_bind_map(ubs_var_factory_, ubs_functor_factory_);
    for (const auto& [head_id, reps] : grandchild_level.watch.head_to_reps)
        for (uint32_t rep : reps)
            EXPECT_EQ(bind_map.resolve(rep), var(rep)) << "head " << head_id << " holds non-rep " << rep;
}

TEST_F(PudStructureWatchDescenderIntegrationTest, RepUpdateWithANonRepSideBreaksTheInvariant) {
    level root_level = watch(at_root(), functor("f", {var(2)}));
    node_specs_.store(child, {rep_updated(1, 0)});
    node_specs_.store(grandchild, {rep_updated(2, 1)});
    level child_level = descend(root_level, child);
    EXPECT_THROW(descend(child_level, grandchild), std::logic_error);
}

// ── branching ────────────────────────────────────────────────────────────────

TEST_F(PudStructureWatchDescenderIntegrationTest, SiblingBranchesSeeTheirOwnChanges) {
    level root_level = at_root();
    root_level = watch(root_level, functor("f", {var(0)}));
    root_level = watch(root_level, functor("g", {var(1)}));
    node_specs_.store(child, {rep_instantiated(0, functor("a", {}))});
    node_specs_.store(sibling, {rep_instantiated(1, functor("a", {}))});
    EXPECT_THAT(descend(root_level, child).changed, UnorderedElementsAre(0u));
    EXPECT_THAT(descend(root_level, sibling).changed, UnorderedElementsAre(1u));
}

TEST_F(PudStructureWatchDescenderIntegrationTest, ChangeInOneBranchDoesNotLeakIntoTheOther) {
    level root_level = watch(at_root(), functor("f", {var(0), var(1)}));
    node_specs_.store(child, {rep_updated(1, 0)});
    node_specs_.store(sibling, {rep_instantiated(5, functor("a", {}))});
    ASSERT_THAT(descend(root_level, child).changed, UnorderedElementsAre(0u));
    level sibling_level = descend(root_level, sibling);
    EXPECT_THAT(sibling_level.changed, IsEmpty());
    const immer::set<uint32_t>* reps = sibling_level.watch.head_to_reps.find(0);
    ASSERT_NE(reps, nullptr);
    EXPECT_EQ(reps->size(), 2u);
}

TEST_F(PudStructureWatchDescenderIntegrationTest, UnwatchedExpressionIsNotYielded) {
    level root_level = at_root();
    root_level = watch(root_level, functor("f", {var(0)}));
    root_level = watch(root_level, functor("g", {var(0)}));
    root_level.watch = watcher_.unwatch(root_level.watch, 0);
    node_specs_.store(child, {rep_instantiated(0, functor("a", {}))});
    EXPECT_THAT(descend(root_level, child).changed, UnorderedElementsAre(1u));
}
