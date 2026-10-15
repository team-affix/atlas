#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "functor_fixture.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/pud_node_added_specializations.hpp"
#include "infrastructure/pud_structure_watch_descent.hpp"
#include "value_objects/pud_specialization.hpp"

using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::UnorderedElementsAre;

struct PudStructureWatchDescentIntegrationTest : public ::testing::Test {
    using watch_descent_t = pud_structure_watch_descent<pud_node_added_specializations>;
    using result_t = watch_descent_t::descend_result;

    test_functors                  functors;
    expr_pool                      exprs;
    pud_node_added_specializations node_specs_;
    pud_node_id child = 1;
    pud_node_id grandchild = 2;
    pud_node_id sibling = 3;
    pud_node_id great_grandchild = 4;

    const expr* var(uint32_t index) { return exprs.make_var(index); }

    const expr* functor(const char* name, std::vector<const expr*> args) {
        return exprs.make_functor(functors.id(name), std::move(args));
    }

    pud_specialization rep_instantiated(uint32_t rep, const expr* value) {
        return pud_specialization{.var_idx = rep, .value = value};
    }

    pud_specialization rep_updated(uint32_t demoted_rep, uint32_t surviving_rep) {
        return pud_specialization{.var_idx = demoted_rep, .value = var(surviving_rep)};
    }

    watch_descent_t at_root(const std::vector<const expr*>& watched) {
        return watch_descent_t{node_specs_, watched};
    }

    static std::vector<pud_mhsw_head_id> changes_at(const watch_descent_t& parent, pud_node_id node) {
        return parent.descend(node, {}, {}).structure_changes;
    }
};

// ── rep instantiated ─────────────────────────────────────────────────────────

TEST_F(PudStructureWatchDescentIntegrationTest, InstantiatingAWatchedRepChangesIt) {
    node_specs_.store(child, {rep_instantiated(0, functor("a", {}))});
    EXPECT_THAT(changes_at(at_root({functor("f", {var(0), var(1)})}), child), ElementsAre(0u));
}

TEST_F(PudStructureWatchDescentIntegrationTest, InstantiatingAnOutsideRepChangesNothing) {
    node_specs_.store(child, {rep_instantiated(5, functor("a", {}))});
    EXPECT_THAT(changes_at(at_root({functor("f", {var(0), var(1)})}), child), IsEmpty());
}

TEST_F(PudStructureWatchDescentIntegrationTest, InstantiationChangesEveryExpressionSharingTheRep) {
    node_specs_.store(child, {rep_instantiated(0, functor("a", {}))});
    watch_descent_t root = at_root({functor("f", {var(0)}), functor("g", {var(0), var(1)}), functor("h", {var(1)})});
    EXPECT_THAT(changes_at(root, child), UnorderedElementsAre(0u, 1u));
}

TEST_F(PudStructureWatchDescentIntegrationTest, RepsIntroducedByAnInstantiationAreWatched) {
    node_specs_.store(child, {rep_instantiated(0, functor("f", {var(3)}))});
    node_specs_.store(grandchild, {rep_instantiated(3, functor("a", {}))});
    result_t at_child = at_root({functor("g", {var(0)})}).descend(child, {}, {});
    ASSERT_THAT(at_child.structure_changes, ElementsAre(0u));
    EXPECT_THAT(changes_at(at_child.child_descent, grandchild), ElementsAre(0u));
}

TEST_F(PudStructureWatchDescentIntegrationTest, GroundInstantiationStopsWatchingThatRep) {
    node_specs_.store(child, {rep_instantiated(0, functor("a", {}))});
    node_specs_.store(grandchild, {rep_updated(1, 5)});
    result_t at_child = at_root({functor("f", {var(0), var(1)})}).descend(child, {}, {});
    EXPECT_THAT(changes_at(at_child.child_descent, grandchild), IsEmpty());
}

// ── rep updated ──────────────────────────────────────────────────────────────

TEST_F(PudStructureWatchDescentIntegrationTest, UpdatingTwoRepsOfTheExpressionChangesIt) {
    node_specs_.store(child, {rep_updated(1, 0)});
    EXPECT_THAT(changes_at(at_root({functor("f", {var(0), var(1)})}), child), ElementsAre(0u));
}

TEST_F(PudStructureWatchDescentIntegrationTest, UpdatingAnOutsideRepChangesNothing) {
    node_specs_.store(child, {rep_updated(5, 1)});
    EXPECT_THAT(changes_at(at_root({functor("f", {var(0), var(1)})}), child), IsEmpty());
}

TEST_F(PudStructureWatchDescentIntegrationTest, DemotedRepIsWatchedThroughTheSurvivingRep) {
    node_specs_.store(child, {rep_updated(4, 1)});
    node_specs_.store(grandchild, {rep_instantiated(1, functor("a", {}))});
    result_t at_child = at_root({functor("f", {var(4)})}).descend(child, {}, {});
    ASSERT_THAT(at_child.structure_changes, IsEmpty());
    EXPECT_THAT(changes_at(at_child.child_descent, grandchild), ElementsAre(0u));
}

TEST_F(PudStructureWatchDescentIntegrationTest, MergingRepsOfTwoExpressionsLinksBothToTheSurvivingRep) {
    node_specs_.store(child, {rep_updated(1, 0)});
    node_specs_.store(grandchild, {rep_instantiated(0, functor("a", {}))});
    result_t at_child = at_root({functor("f", {var(0)}), functor("g", {var(1)})}).descend(child, {}, {});
    ASSERT_THAT(at_child.structure_changes, IsEmpty());
    EXPECT_THAT(changes_at(at_child.child_descent, grandchild), UnorderedElementsAre(0u, 1u));
}

TEST_F(PudStructureWatchDescentIntegrationTest, CollapseAcrossTwoLevels) {
    node_specs_.store(child, {rep_updated(2, 1)});
    node_specs_.store(grandchild, {rep_updated(1, 0)});
    result_t at_child = at_root({functor("f", {var(0), var(2)})}).descend(child, {}, {});
    ASSERT_THAT(at_child.structure_changes, IsEmpty());
    EXPECT_THAT(changes_at(at_child.child_descent, grandchild), ElementsAre(0u));
}

// ── watching and unwatching below the root ───────────────────────────────────

TEST_F(PudStructureWatchDescentIntegrationTest, WatchAtAChildTakesTheExpressionInThatNodesReps) {
    node_specs_.store(child, {rep_instantiated(0, functor("f", {var(3)}))});
    node_specs_.store(grandchild, {rep_instantiated(3, functor("a", {}))});
    result_t at_child = at_root({}).descend(child, {}, {functor("h", {functor("f", {var(3)})})});
    ASSERT_THAT(at_child.added_head_ids, ElementsAre(0u));
    EXPECT_THAT(changes_at(at_child.child_descent, grandchild), ElementsAre(0u));
}

TEST_F(PudStructureWatchDescentIntegrationTest, WatchAtAChildUsesTheSurvivingRep) {
    node_specs_.store(child, {rep_updated(4, 1)});
    node_specs_.store(grandchild, {rep_updated(1, 0)});
    result_t at_child = at_root({}).descend(child, {}, {functor("h", {var(1), var(0)})});
    EXPECT_THAT(changes_at(at_child.child_descent, grandchild), ElementsAre(0u));
}

TEST_F(PudStructureWatchDescentIntegrationTest, WatchUnwatchAcrossThreeLevels) {
    node_specs_.store(child, {rep_instantiated(0, functor("f", {var(5)}))});
    node_specs_.store(grandchild, {rep_updated(6, 5)});
    node_specs_.store(great_grandchild, {rep_instantiated(5, functor("a", {}))});
    result_t at_child = at_root({var(0), var(1)}).descend(child, {1}, {functor("g", {var(6)})});
    EXPECT_THAT(at_child.structure_changes, ElementsAre(0u));
    EXPECT_THAT(at_child.added_head_ids, ElementsAre(2u));
    result_t at_grandchild = at_child.child_descent.descend(grandchild, {}, {var(5)});
    EXPECT_THAT(at_grandchild.structure_changes, IsEmpty());
    EXPECT_THAT(at_grandchild.added_head_ids, ElementsAre(3u));
    EXPECT_THAT(changes_at(at_grandchild.child_descent, great_grandchild), UnorderedElementsAre(0u, 2u, 3u));
}

// ── many specializations at one node ─────────────────────────────────────────

TEST_F(PudStructureWatchDescentIntegrationTest, UnrelatedBindingsLeaveStructureAlone) {
    node_specs_.store(child, {
        rep_updated(3, 2),
        rep_instantiated(4, functor("a", {})),
        rep_updated(5, 1),
    });
    EXPECT_THAT(changes_at(at_root({functor("f", {var(0), var(1)})}), child), IsEmpty());
}

TEST_F(PudStructureWatchDescentIntegrationTest, ExpressionChangedTwiceAtOneNodeIsReportedOnce) {
    node_specs_.store(child, {rep_updated(1, 0), rep_instantiated(2, functor("a", {}))});
    EXPECT_THAT(changes_at(at_root({functor("f", {var(0), var(1), var(2)})}), child), ElementsAre(0u));
}

TEST_F(PudStructureWatchDescentIntegrationTest, InstantiationValueIsWrittenInTheNodesReps) {
    node_specs_.store(child, {rep_updated(3, 1)});
    node_specs_.store(grandchild, {rep_instantiated(0, functor("g", {var(1)}))});
    node_specs_.store(great_grandchild, {rep_instantiated(1, functor("a", {}))});
    result_t at_child = at_root({functor("f", {var(0), var(1)})}).descend(child, {}, {});
    result_t at_grandchild = at_child.child_descent.descend(grandchild, {}, {});
    ASSERT_THAT(at_grandchild.structure_changes, ElementsAre(0u));
    EXPECT_THAT(changes_at(at_grandchild.child_descent, great_grandchild), ElementsAre(0u));
}

// ── branching ────────────────────────────────────────────────────────────────

TEST_F(PudStructureWatchDescentIntegrationTest, SiblingBranchesSeeTheirOwnChanges) {
    node_specs_.store(child, {rep_instantiated(0, functor("a", {}))});
    node_specs_.store(sibling, {rep_instantiated(1, functor("a", {}))});
    watch_descent_t root = at_root({functor("f", {var(0)}), functor("g", {var(1)})});
    EXPECT_THAT(changes_at(root, child), ElementsAre(0u));
    EXPECT_THAT(changes_at(root, sibling), ElementsAre(1u));
}

TEST_F(PudStructureWatchDescentIntegrationTest, ChangeInOneBranchDoesNotLeakIntoTheOther) {
    node_specs_.store(child, {rep_updated(1, 0)});
    node_specs_.store(sibling, {rep_instantiated(5, functor("a", {}))});
    node_specs_.store(grandchild, {rep_updated(1, 0)});
    watch_descent_t root = at_root({functor("f", {var(0), var(1)})});
    ASSERT_THAT(changes_at(root, child), ElementsAre(0u));
    result_t at_sibling = root.descend(sibling, {}, {});
    EXPECT_THAT(at_sibling.structure_changes, IsEmpty());
    EXPECT_THAT(changes_at(at_sibling.child_descent, grandchild), ElementsAre(0u));
}

TEST_F(PudStructureWatchDescentIntegrationTest, UnwatchingInOneBranchDoesNotAffectTheOther) {
    node_specs_.store(child, {rep_instantiated(0, functor("a", {}))});
    node_specs_.store(sibling, {rep_instantiated(0, functor("a", {}))});
    watch_descent_t root = at_root({functor("f", {var(0)}), functor("g", {var(0)})});
    EXPECT_THAT(root.descend(child, {0}, {}).structure_changes, ElementsAre(1u));
    EXPECT_THAT(changes_at(root, sibling), UnorderedElementsAre(0u, 1u));
}
