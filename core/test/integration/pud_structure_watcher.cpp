#include <cstdint>
#include <vector>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "functor_fixture.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/pud_binder.hpp"
#include "infrastructure/pud_data.hpp"
#include "infrastructure/pud_structure_watcher.hpp"
#include "infrastructure/pud_term_factory.hpp"
#include "value_objects/pud_goal_list.hpp"

using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::UnorderedElementsAre;

// The structure watcher fed by the real binder: every node closes one goal, its call site, and each
// node here carries a ground `go` goal as the call site its child closes.
struct PudStructureWatcherIntegrationTest : public ::testing::Test {
    using term_factory_t = pud_term_factory<globalizer, expr_pool, expr_pool>;

    struct at_node {
        pud_ubs_state bindings;
        pud_goal_list goals;
        pud_structure_watch watch;
        pud_body_goal_id call_site;
        std::vector<pud_body_goal_id> changed_goals;
    };

    test_functors functors;
    expr_pool exprs;
    globalizer globalize;
    term_factory_t terms{globalize, exprs, exprs};
    pud_data data;
    pud_binder<term_factory_t, pud_data> binder{terms, data};
    pud_structure_watcher<term_factory_t, pud_data> watcher{terms, data};
    pud_node_id dummy_root = 0;

    const expr* var(uint32_t index) { return exprs.make_var(index); }

    const expr* functor(const char* name, std::vector<const expr*> args) {
        return exprs.make_functor(functors.id(name), std::move(args));
    }

    void SetUp() override {
        dummy_root = data.store_axiom(rule(functor("dummy", {}), {}, 0));
    }

    pud_node_id infer(std::vector<pud_substitution> substitutions) {
        return data.store_inference(pud_node_delta{
            .parent = dummy_root,
            .call_site = 0,
            .substitutions = std::move(substitutions),
            .body_goals = {},
            .var_count = 0,
        });
    }

    pud_substitution rep_instantiated(uint32_t rep, const expr* value) {
        return pud_substitution{.var_idx = rep, .value = value};
    }

    pud_substitution rep_updated(uint32_t demoted_rep, uint32_t surviving_rep) {
        return pud_substitution{.var_idx = demoted_rep, .value = var(surviving_rep)};
    }

    at_node at_root(const std::vector<const expr*>& watched) {
        pud_goal_list goals{.goals = {}, .next_id = 0};
        for (const expr* watched_goal : watched)
            goals = goals.adding(watched_goal);
        goals = goals.adding(functor("go", {}));
        return at_node{
            .bindings = {},
            .goals = goals,
            .watch = watcher.watch_root(pud_ubs_state{}, goals),
            .call_site = goals.next_id - 1,
            .changed_goals = {},
        };
    }

    at_node descend(const at_node& parent, pud_node_id node, const std::vector<const expr*>& added) {
        pud_goal_list goals = parent.goals.closing(parent.call_site);
        for (const expr* added_goal : added)
            goals = goals.adding(added_goal);
        goals = goals.adding(functor("go", {}));
        return step(parent, node, parent.call_site, goals, goals.next_id - 1);
    }

    at_node descend_closing(
        const at_node& parent, pud_node_id node, pud_body_goal_id closed, const std::vector<const expr*>& added) {
        pud_goal_list goals = parent.goals.closing(closed);
        for (const expr* added_goal : added)
            goals = goals.adding(added_goal);
        return step(parent, node, closed, goals, parent.call_site);
    }

    at_node step(
        const at_node& parent,
        pud_node_id node,
        pud_body_goal_id closed,
        const pud_goal_list& goals,
        pud_body_goal_id call_site) {
        const pud_ubs_state bindings = binder.bind(parent.bindings, node);
        const pud_structure_watch_change change = watcher.descend(parent.watch, bindings, node, closed, goals);
        return at_node{
            .bindings = bindings,
            .goals = goals,
            .watch = change.watch,
            .call_site = call_site,
            .changed_goals = change.changed_goals,
        };
    }

    std::vector<pud_body_goal_id> changes_at(const at_node& parent, pud_node_id node) {
        return descend(parent, node, {}).changed_goals;
    }
};

TEST_F(PudStructureWatcherIntegrationTest, InstantiatingAWatchedRepChangesIt) {
    const pud_node_id child = infer({rep_instantiated(0, functor("a", {}))});
    EXPECT_THAT(changes_at(at_root({functor("f", {var(0), var(1)})}), child), ElementsAre(0u));
}

TEST_F(PudStructureWatcherIntegrationTest, InstantiatingAnOutsideRepChangesNothing) {
    const pud_node_id child = infer({rep_instantiated(5, functor("a", {}))});
    EXPECT_THAT(changes_at(at_root({functor("f", {var(0), var(1)})}), child), IsEmpty());
}

TEST_F(PudStructureWatcherIntegrationTest, InstantiationChangesEveryGoalSharingTheRep) {
    const pud_node_id child = infer({rep_instantiated(0, functor("a", {}))});
    const at_node root = at_root({functor("f", {var(0)}), functor("g", {var(0), var(1)}), functor("h", {var(1)})});
    EXPECT_THAT(changes_at(root, child), UnorderedElementsAre(0u, 1u));
}

TEST_F(PudStructureWatcherIntegrationTest, RepsIntroducedByAnInstantiationAreWatched) {
    const pud_node_id child = infer( {rep_instantiated(0, functor("f", {var(3)}))});
    const pud_node_id grandchild = infer( {rep_instantiated(3, functor("a", {}))});
    const at_node at_child = descend(at_root({functor("g", {var(0)})}), child, {});
    ASSERT_THAT(at_child.changed_goals, ElementsAre(0u));
    EXPECT_THAT(changes_at(at_child, grandchild), ElementsAre(0u));
}

TEST_F(PudStructureWatcherIntegrationTest, GroundInstantiationStopsWatchingThatRep) {
    const pud_node_id child = infer( {rep_instantiated(0, functor("a", {}))});
    const pud_node_id grandchild = infer( {rep_updated(1, 5)});
    const at_node at_child = descend(at_root({functor("f", {var(0), var(1)})}), child, {});
    EXPECT_THAT(changes_at(at_child, grandchild), IsEmpty());
}

TEST_F(PudStructureWatcherIntegrationTest, UpdatingTwoRepsOfTheGoalChangesIt) {
    const pud_node_id child = infer( {rep_updated(1, 0)});
    EXPECT_THAT(changes_at(at_root({functor("f", {var(0), var(1)})}), child), ElementsAre(0u));
}

TEST_F(PudStructureWatcherIntegrationTest, UpdatingAnOutsideRepChangesNothing) {
    const pud_node_id child = infer( {rep_updated(5, 1)});
    EXPECT_THAT(changes_at(at_root({functor("f", {var(0), var(1)})}), child), IsEmpty());
}

TEST_F(PudStructureWatcherIntegrationTest, DemotedRepIsWatchedThroughTheSurvivingRep) {
    const pud_node_id child = infer( {rep_updated(4, 1)});
    const pud_node_id grandchild = infer( {rep_instantiated(1, functor("a", {}))});
    const at_node at_child = descend(at_root({functor("f", {var(4)})}), child, {});
    ASSERT_THAT(at_child.changed_goals, IsEmpty());
    EXPECT_THAT(changes_at(at_child, grandchild), ElementsAre(0u));
}

TEST_F(PudStructureWatcherIntegrationTest, MergingRepsOfTwoGoalsLinksBothToTheSurvivingRep) {
    const pud_node_id child = infer( {rep_updated(1, 0)});
    const pud_node_id grandchild = infer( {rep_instantiated(0, functor("a", {}))});
    const at_node at_child = descend(at_root({functor("f", {var(0)}), functor("g", {var(1)})}), child, {});
    ASSERT_THAT(at_child.changed_goals, IsEmpty());
    EXPECT_THAT(changes_at(at_child, grandchild), UnorderedElementsAre(0u, 1u));
}

TEST_F(PudStructureWatcherIntegrationTest, CollapseAcrossTwoLevels) {
    const pud_node_id child = infer( {rep_updated(2, 1)});
    const pud_node_id grandchild = infer( {rep_updated(1, 0)});
    const at_node at_child = descend(at_root({functor("f", {var(0), var(2)})}), child, {});
    ASSERT_THAT(at_child.changed_goals, IsEmpty());
    EXPECT_THAT(changes_at(at_child, grandchild), ElementsAre(0u));
}

TEST_F(PudStructureWatcherIntegrationTest, AGoalAddedAtAChildIsWatchedInThatNodesReps) {
    const pud_node_id child = infer( {rep_instantiated(0, functor("f", {var(3)}))});
    const pud_node_id grandchild = infer( {rep_instantiated(3, functor("a", {}))});
    const at_node at_child = descend(at_root({}), child, {functor("h", {functor("f", {var(3)})})});
    ASSERT_THAT(at_child.changed_goals, IsEmpty());
    EXPECT_THAT(changes_at(at_child, grandchild), ElementsAre(1u));
}

TEST_F(PudStructureWatcherIntegrationTest, AGoalAddedAtAChildUsesTheSurvivingRep) {
    const pud_node_id child = infer( {rep_updated(4, 1)});
    const pud_node_id grandchild = infer( {rep_updated(1, 0)});
    const at_node at_child = descend(at_root({}), child, {functor("h", {var(4), var(0)})});
    EXPECT_THAT(changes_at(at_child, grandchild), ElementsAre(1u));
}

TEST_F(PudStructureWatcherIntegrationTest, WatchUnwatchAcrossThreeLevels) {
    const pud_node_id child = infer( {rep_instantiated(0, functor("f", {var(5)}))});
    const pud_node_id grandchild = infer( {rep_updated(6, 5)});
    const pud_node_id great_grandchild = infer( {rep_instantiated(5, functor("a", {}))});
    const at_node at_child = descend_closing(at_root({var(0), var(1)}), child, 1, {functor("g", {var(6)})});
    EXPECT_THAT(at_child.changed_goals, ElementsAre(0u));
    const at_node at_grandchild = descend(at_child, grandchild, {var(5)});
    EXPECT_THAT(at_grandchild.changed_goals, IsEmpty());
    EXPECT_THAT(changes_at(at_grandchild, great_grandchild), UnorderedElementsAre(0u, 3u, 4u));
}

TEST_F(PudStructureWatcherIntegrationTest, UnrelatedBindingsLeaveStructureAlone) {
    const pud_node_id child = infer( {
        rep_updated(3, 2),
        rep_instantiated(4, functor("a", {})),
        rep_updated(5, 1),
    });
    EXPECT_THAT(changes_at(at_root({functor("f", {var(0), var(1)})}), child), IsEmpty());
}

TEST_F(PudStructureWatcherIntegrationTest, GoalChangedTwiceAtOneNodeIsReportedOnce) {
    const pud_node_id child = infer( {rep_updated(1, 0), rep_instantiated(2, functor("a", {}))});
    EXPECT_THAT(changes_at(at_root({functor("f", {var(0), var(1), var(2)})}), child), ElementsAre(0u));
}

TEST_F(PudStructureWatcherIntegrationTest, InstantiationValueIsWrittenInTheNodesReps) {
    const pud_node_id child = infer( {rep_updated(3, 1)});
    const pud_node_id grandchild = infer( {rep_instantiated(0, functor("g", {var(1)}))});
    const pud_node_id great_grandchild = infer( {rep_instantiated(1, functor("a", {}))});
    const at_node at_child = descend(at_root({functor("f", {var(0), var(1)})}), child, {});
    const at_node at_grandchild = descend(at_child, grandchild, {});
    ASSERT_THAT(at_grandchild.changed_goals, ElementsAre(0u));
    EXPECT_THAT(changes_at(at_grandchild, great_grandchild), ElementsAre(0u));
}

TEST_F(PudStructureWatcherIntegrationTest, SiblingBranchesSeeTheirOwnChanges) {
    const pud_node_id child = infer( {rep_instantiated(0, functor("a", {}))});
    const pud_node_id sibling = infer( {rep_instantiated(1, functor("a", {}))});
    const at_node root = at_root({functor("f", {var(0)}), functor("g", {var(1)})});
    EXPECT_THAT(changes_at(root, child), ElementsAre(0u));
    EXPECT_THAT(changes_at(root, sibling), ElementsAre(1u));
}

TEST_F(PudStructureWatcherIntegrationTest, ChangeInOneBranchDoesNotLeakIntoTheOther) {
    const pud_node_id child = infer( {rep_updated(1, 0)});
    const pud_node_id sibling = infer( {rep_instantiated(5, functor("a", {}))});
    const pud_node_id grandchild = infer( {rep_updated(1, 0)});
    const at_node root = at_root({functor("f", {var(0), var(1)})});
    ASSERT_THAT(changes_at(root, child), ElementsAre(0u));
    const at_node at_sibling = descend(root, sibling, {});
    EXPECT_THAT(at_sibling.changed_goals, IsEmpty());
    EXPECT_THAT(changes_at(at_sibling, grandchild), ElementsAre(0u));
}

TEST_F(PudStructureWatcherIntegrationTest, ChangesFollowTheStoredSubstitutionOrder) {
    const pud_node_id child = infer( {
        rep_instantiated(2, functor("a", {})),
        rep_updated(1, 0),
        rep_instantiated(3, functor("b", {})),
    });
    const at_node root = at_root({functor("f", {var(0), var(1)}), var(2), var(3)});
    EXPECT_THAT(changes_at(root, child), ElementsAre(1u, 0u, 2u));
}

TEST_F(PudStructureWatcherIntegrationTest, InternedDuplicateGoalsAreWatchedSeparately) {
    const pud_node_id child = infer( {rep_instantiated(0, functor("a", {}))});
    const expr* first = functor("f", {var(0)});
    const expr* second = functor("f", {var(0)});
    ASSERT_EQ(first, second);
    EXPECT_THAT(changes_at(at_root({first, second}), child), UnorderedElementsAre(0u, 1u));
}

TEST_F(PudStructureWatcherIntegrationTest, LongRenamingChainAcrossStoredLevels) {
    const uint32_t chain_length = 200;
    at_node at = at_root({functor("f", {var(0), var(chain_length)})});
    for (uint32_t step_index = 0; step_index < chain_length; ++step_index) {
        const uint32_t demoted = chain_length - step_index;
        const pud_node_id level_node = infer({rep_updated(demoted, demoted - 1)});
        at = descend(at, level_node, {});
        const bool last_step = step_index + 1 == chain_length;
        EXPECT_EQ(at.changed_goals.size(), last_step ? 1u : 0u) << "step " << step_index;
    }
}

TEST_F(PudStructureWatcherIntegrationTest, AClosedGoalIsNoLongerWatchedButItsReplacementIs) {
    const pud_node_id child = infer( {rep_updated(4, 2)});
    const pud_node_id grandchild = infer( {rep_instantiated(2, functor("a", {}))});
    const at_node at_child = descend_closing(at_root({functor("f", {var(4)})}), child, 0, {functor("f", {var(2)})});
    ASSERT_THAT(at_child.changed_goals, IsEmpty());
    EXPECT_THAT(changes_at(at_child, grandchild), ElementsAre(2u));
}

TEST_F(PudStructureWatcherIntegrationTest, ClosingInOneBranchDoesNotAffectTheOther) {
    const pud_node_id child = infer( {rep_instantiated(0, functor("a", {}))});
    const pud_node_id sibling = infer( {rep_instantiated(0, functor("a", {}))});
    const at_node root = at_root({functor("f", {var(0)}), functor("g", {var(0)})});
    EXPECT_THAT(descend_closing(root, child, 0, {}).changed_goals, ElementsAre(1u));
    EXPECT_THAT(changes_at(root, sibling), UnorderedElementsAre(0u, 1u));
}

TEST_F(PudStructureWatcherIntegrationTest, AnAddedGoalOverABoundVarIsWatchedThroughItsValue) {
    const pud_node_id child = infer( {rep_instantiated(0, functor("s", {var(3)}))});
    const pud_node_id grandchild = infer( {rep_instantiated(3, functor("a", {}))});
    const at_node at_child = descend(at_root({}), child, {functor("g", {var(0)})});
    ASSERT_THAT(at_child.changed_goals, IsEmpty());
    EXPECT_THAT(changes_at(at_child, grandchild), ElementsAre(1u));
}
