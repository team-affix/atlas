#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "functor_fixture.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/pud_node_added_substitutions.hpp"
#include "infrastructure/pud_structure_watch_descent.hpp"
#include "infrastructure/pud_transient_ubs_state.hpp"
#include "value_objects/pud_substitution.hpp"

using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::UnorderedElementsAre;

struct PudStructureWatchDescentIntegrationTest : public ::testing::Test {
    using watch_descent_t = pud_structure_watch_descent<pud_node_added_substitutions>;
    using result_t = pud_structure_watch_result<watch_descent_t>;

    test_functors                  functors;
    expr_pool                      exprs;
    pud_node_added_substitutions node_specs_;
    pud_node_id child = 1;
    pud_node_id grandchild = 2;
    pud_node_id sibling = 3;
    pud_node_id great_grandchild = 4;

    const expr* var(uint32_t index) { return exprs.make_var(index); }

    const expr* functor(const char* name, std::vector<const expr*> args) {
        return exprs.make_functor(functors.id(name), std::move(args));
    }

    pud_substitution rep_instantiated(uint32_t rep, const expr* value) {
        return pud_substitution{.var_idx = rep, .value = value};
    }

    pud_substitution rep_updated(uint32_t demoted_rep, uint32_t surviving_rep) {
        return pud_substitution{.var_idx = demoted_rep, .value = var(surviving_rep)};
    }

    watch_descent_t at_root(const std::vector<const expr*>& watched) {
        return watch_descent_t{node_specs_, watched};
    }

    static std::vector<pud_watch_head_id> changes_at(const watch_descent_t& parent, pud_node_id node) {
        return parent.descend(node, {}, {}, pud_ubs_state{}).structure_changes;
    }
};

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
    result_t at_child = at_root({functor("g", {var(0)})}).descend(child, {}, {}, pud_ubs_state{});
    ASSERT_THAT(at_child.structure_changes, ElementsAre(0u));
    EXPECT_THAT(changes_at(at_child.child_descent, grandchild), ElementsAre(0u));
}

TEST_F(PudStructureWatchDescentIntegrationTest, GroundInstantiationStopsWatchingThatRep) {
    node_specs_.store(child, {rep_instantiated(0, functor("a", {}))});
    node_specs_.store(grandchild, {rep_updated(1, 5)});
    result_t at_child = at_root({functor("f", {var(0), var(1)})}).descend(child, {}, {}, pud_ubs_state{});
    EXPECT_THAT(changes_at(at_child.child_descent, grandchild), IsEmpty());
}

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
    result_t at_child = at_root({functor("f", {var(4)})}).descend(child, {}, {}, pud_ubs_state{});
    ASSERT_THAT(at_child.structure_changes, IsEmpty());
    EXPECT_THAT(changes_at(at_child.child_descent, grandchild), ElementsAre(0u));
}

TEST_F(PudStructureWatchDescentIntegrationTest, MergingRepsOfTwoExpressionsLinksBothToTheSurvivingRep) {
    node_specs_.store(child, {rep_updated(1, 0)});
    node_specs_.store(grandchild, {rep_instantiated(0, functor("a", {}))});
    result_t at_child = at_root({functor("f", {var(0)}), functor("g", {var(1)})}).descend(child, {}, {}, pud_ubs_state{});
    ASSERT_THAT(at_child.structure_changes, IsEmpty());
    EXPECT_THAT(changes_at(at_child.child_descent, grandchild), UnorderedElementsAre(0u, 1u));
}

TEST_F(PudStructureWatchDescentIntegrationTest, CollapseAcrossTwoLevels) {
    node_specs_.store(child, {rep_updated(2, 1)});
    node_specs_.store(grandchild, {rep_updated(1, 0)});
    result_t at_child = at_root({functor("f", {var(0), var(2)})}).descend(child, {}, {}, pud_ubs_state{});
    ASSERT_THAT(at_child.structure_changes, IsEmpty());
    EXPECT_THAT(changes_at(at_child.child_descent, grandchild), ElementsAre(0u));
}

TEST_F(PudStructureWatchDescentIntegrationTest, WatchAtAChildTakesTheExpressionInThatNodesReps) {
    node_specs_.store(child, {rep_instantiated(0, functor("f", {var(3)}))});
    node_specs_.store(grandchild, {rep_instantiated(3, functor("a", {}))});
    result_t at_child = at_root({}).descend(child, {}, {functor("h", {functor("f", {var(3)})})}, pud_ubs_state{});
    ASSERT_THAT(at_child.added_head_ids, ElementsAre(0u));
    EXPECT_THAT(changes_at(at_child.child_descent, grandchild), ElementsAre(0u));
}

TEST_F(PudStructureWatchDescentIntegrationTest, WatchAtAChildUsesTheSurvivingRep) {
    node_specs_.store(child, {rep_updated(4, 1)});
    node_specs_.store(grandchild, {rep_updated(1, 0)});
    result_t at_child = at_root({}).descend(child, {}, {functor("h", {var(1), var(0)})}, pud_ubs_state{});
    EXPECT_THAT(changes_at(at_child.child_descent, grandchild), ElementsAre(0u));
}

TEST_F(PudStructureWatchDescentIntegrationTest, WatchUnwatchAcrossThreeLevels) {
    node_specs_.store(child, {rep_instantiated(0, functor("f", {var(5)}))});
    node_specs_.store(grandchild, {rep_updated(6, 5)});
    node_specs_.store(great_grandchild, {rep_instantiated(5, functor("a", {}))});
    result_t at_child = at_root({var(0), var(1)}).descend(child, {1}, {functor("g", {var(6)})}, pud_ubs_state{});
    EXPECT_THAT(at_child.structure_changes, ElementsAre(0u));
    EXPECT_THAT(at_child.added_head_ids, ElementsAre(2u));
    result_t at_grandchild = at_child.child_descent.descend(grandchild, {}, {var(5)}, pud_ubs_state{});
    EXPECT_THAT(at_grandchild.structure_changes, IsEmpty());
    EXPECT_THAT(at_grandchild.added_head_ids, ElementsAre(3u));
    EXPECT_THAT(changes_at(at_grandchild.child_descent, great_grandchild), UnorderedElementsAre(0u, 2u, 3u));
}

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
    result_t at_child = at_root({functor("f", {var(0), var(1)})}).descend(child, {}, {}, pud_ubs_state{});
    result_t at_grandchild = at_child.child_descent.descend(grandchild, {}, {}, pud_ubs_state{});
    ASSERT_THAT(at_grandchild.structure_changes, ElementsAre(0u));
    EXPECT_THAT(changes_at(at_grandchild.child_descent, great_grandchild), ElementsAre(0u));
}

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
    result_t at_sibling = root.descend(sibling, {}, {}, pud_ubs_state{});
    EXPECT_THAT(at_sibling.structure_changes, IsEmpty());
    EXPECT_THAT(changes_at(at_sibling.child_descent, grandchild), ElementsAre(0u));
}

TEST_F(PudStructureWatchDescentIntegrationTest, ChangesFollowTheStoredSubstitutionOrder) {
    node_specs_.store(child, {
        rep_instantiated(2, functor("a", {})),
        rep_updated(1, 0),
        rep_instantiated(3, functor("b", {})),
    });
    watch_descent_t root = at_root({functor("f", {var(0), var(1)}), var(2), var(3)});
    EXPECT_THAT(changes_at(root, child), ElementsAre(1u, 0u, 2u));
}

TEST_F(PudStructureWatchDescentIntegrationTest, InternedDuplicateExpressionsAreSeparateHeads) {
    node_specs_.store(child, {rep_instantiated(0, functor("a", {}))});
    const expr* first = functor("f", {var(0)});
    const expr* second = functor("f", {var(0)});
    ASSERT_EQ(first, second);
    EXPECT_THAT(changes_at(at_root({first, second}), child), UnorderedElementsAre(0u, 1u));
}

TEST_F(PudStructureWatchDescentIntegrationTest, LongRenamingChainAcrossStoredLevels) {
    const uint32_t chain_length = 200;
    watch_descent_t at = at_root({functor("f", {var(0), var(chain_length)})});
    for (uint32_t step = 0; step < chain_length; ++step) {
        const uint32_t demoted = chain_length - step;
        const pud_node_id level_node = 1000 + step;
        node_specs_.store(level_node, {rep_updated(demoted, demoted - 1)});
        result_t result = at.descend(level_node, {}, {}, pud_ubs_state{});
        const bool last_step = step + 1 == chain_length;
        EXPECT_EQ(result.structure_changes.size(), last_step ? 1u : 0u) << "step " << step;
        at = result.child_descent;
    }
}

TEST_F(PudStructureWatchDescentIntegrationTest, UnwatchedAndRewatchedExpressionAcrossLevels) {
    node_specs_.store(child, {rep_updated(4, 2)});
    node_specs_.store(grandchild, {rep_instantiated(2, functor("a", {}))});
    result_t at_child = at_root({functor("f", {var(4)})}).descend(child, {0}, {functor("f", {var(2)})}, pud_ubs_state{});
    ASSERT_THAT(at_child.structure_changes, IsEmpty());
    ASSERT_THAT(at_child.added_head_ids, ElementsAre(1u));
    EXPECT_THAT(changes_at(at_child.child_descent, grandchild), ElementsAre(1u));
}

TEST_F(PudStructureWatchDescentIntegrationTest, UnwatchingInOneBranchDoesNotAffectTheOther) {
    node_specs_.store(child, {rep_instantiated(0, functor("a", {}))});
    node_specs_.store(sibling, {rep_instantiated(0, functor("a", {}))});
    watch_descent_t root = at_root({functor("f", {var(0)}), functor("g", {var(0)})});
    EXPECT_THAT(root.descend(child, {0}, {}, pud_ubs_state{}).structure_changes, ElementsAre(1u));
    EXPECT_THAT(changes_at(root, sibling), UnorderedElementsAre(0u, 1u));
}

TEST_F(PudStructureWatchDescentIntegrationTest, AWatchedVarBoundInTheChildIsSeenThroughItsValue) {
    pud_transient_ubs_state transient_ubs_state{pud_ubs_state{}};
    transient_ubs_state.make_bind_map().instantiate(0, framed_expr{functor("s", {var(3)}), 0});
    const pud_ubs_state child_ubs_state = transient_ubs_state.to_persistent_state();
    node_specs_.store(child, {});
    node_specs_.store(grandchild, {rep_instantiated(3, functor("a", {}))});
    result_t at_child = at_root({}).descend(child, {}, {functor("g", {var(0)})}, child_ubs_state);
    EXPECT_EQ(at_child.added_head_ids, (std::vector<pud_watch_head_id>{0}));
    result_t at_grandchild = at_child.child_descent.descend(grandchild, {}, {}, pud_ubs_state{});
    EXPECT_THAT(at_grandchild.structure_changes, ElementsAre(0u));
}
