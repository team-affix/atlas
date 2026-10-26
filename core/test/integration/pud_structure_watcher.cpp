#include <cstdint>
#include <vector>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "functor_fixture.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/pud_binder.hpp"
#include "infrastructure/pud_module.hpp"
#include "infrastructure/pud_modules.hpp"
#include "infrastructure/pud_goal_resolver.hpp"
#include "infrastructure/pud_initial_goal_opener.hpp"
#include "infrastructure/pud_structure_watcher.hpp"
#include "infrastructure/pud_bind_map_factory.hpp"
#include "value_objects/pud_goal_list.hpp"

using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::UnorderedElementsAre;

// The structure watcher fed by the real binder: every node closes one goal, its call site, and each
// node here carries a ground `go` goal as the call site its child closes.
struct PudStructureWatcherIntegrationTest : public ::testing::Test {
    using bind_map_factory_t = pud_bind_map_factory;
    using modules_t = pud_modules<pud_module>;

    static constexpr pud_module_id k_module = 0;

    test_functors functors;
    expr_pool exprs;
    globalizer globalize;
    bind_map_factory_t bind_maps;
    pud_module data{k_module, {}, {rule(exprs.make_functor(functors.id("dummy"), {}), {}, 0)}};
    modules_t modules;
    pud_binder<bind_map_factory_t, modules_t> binder{bind_maps, modules};
    pud_structure_watcher<bind_map_factory_t, modules_t, expr_pool> watcher{bind_maps, modules, exprs};
    pud_goal_resolver goal_resolver;
    pud_initial_goal_opener initial_goal_opener;

    const expr* var(uint32_t index) { return exprs.make_var(index); }

    const expr* functor(const char* name, std::vector<const expr*> args) {
        return exprs.make_functor(functors.id(name), std::move(args));
    }

    void SetUp() override {
        modules.register_module(data);
    }
};

TEST_F(PudStructureWatcherIntegrationTest, InstantiatingAWatchedRepChangesIt) {
    pud_goal_list goals = initial_goal_opener.open_goals(std::vector<const expr*>{functor("f", {var(0), var(1)}), functor("go", {})});
    const pud_body_goal_id root_call_site = goals.next_id - 1;
    const pud_structure_watch root_watch = watcher.watch_root(pud_ubs_state{}, goals);
    const pud_global_node_id child = modules.store_inference(k_module, pud_inference{
        .parent = 0,
        .call_site = root_call_site,
        .substitutions = {pud_substitution{.var_idx = 0, .value = functor("a", {})}},
        .body_goals = {},
        .var_count = 0,
    });
    pud_goal_list child_goals = goal_resolver.resolve_goal(goals, root_call_site, std::vector<const expr*>{functor("go", {})});
    const pud_ubs_state child_bindings = binder.bind(pud_ubs_state{}, child);
    const pud_structure_watch_change change = watcher.descend(root_watch, child_bindings, child, root_call_site, child_goals);
    EXPECT_THAT(change.changed_goals, ElementsAre(0u));
}

TEST_F(PudStructureWatcherIntegrationTest, InstantiatingAnOutsideRepChangesNothing) {
    pud_goal_list goals = initial_goal_opener.open_goals(std::vector<const expr*>{functor("f", {var(0), var(1)}), functor("go", {})});
    const pud_body_goal_id root_call_site = goals.next_id - 1;
    const pud_structure_watch root_watch = watcher.watch_root(pud_ubs_state{}, goals);
    const pud_global_node_id child = modules.store_inference(k_module, pud_inference{
        .parent = 0,
        .call_site = root_call_site,
        .substitutions = {pud_substitution{.var_idx = 5, .value = functor("a", {})}},
        .body_goals = {},
        .var_count = 0,
    });
    pud_goal_list child_goals = goal_resolver.resolve_goal(goals, root_call_site, std::vector<const expr*>{functor("go", {})});
    const pud_ubs_state child_bindings = binder.bind(pud_ubs_state{}, child);
    const pud_structure_watch_change change = watcher.descend(root_watch, child_bindings, child, root_call_site, child_goals);
    EXPECT_THAT(change.changed_goals, IsEmpty());
}

TEST_F(PudStructureWatcherIntegrationTest, InstantiationChangesEveryGoalSharingTheRep) {
    pud_goal_list goals = initial_goal_opener.open_goals(std::vector<const expr*>{functor("f", {var(0)}), functor("g", {var(0), var(1)}), functor("h", {var(1)}), functor("go", {})});
    const pud_body_goal_id root_call_site = goals.next_id - 1;
    const pud_structure_watch root_watch = watcher.watch_root(pud_ubs_state{}, goals);
    const pud_global_node_id child = modules.store_inference(k_module, pud_inference{
        .parent = 0,
        .call_site = root_call_site,
        .substitutions = {pud_substitution{.var_idx = 0, .value = functor("a", {})}},
        .body_goals = {},
        .var_count = 0,
    });
    pud_goal_list child_goals = goal_resolver.resolve_goal(goals, root_call_site, std::vector<const expr*>{functor("go", {})});
    const pud_ubs_state child_bindings = binder.bind(pud_ubs_state{}, child);
    const pud_structure_watch_change change = watcher.descend(root_watch, child_bindings, child, root_call_site, child_goals);
    EXPECT_THAT(change.changed_goals, UnorderedElementsAre(0u, 1u));
}

TEST_F(PudStructureWatcherIntegrationTest, RepsIntroducedByAnInstantiationAreWatched) {
    pud_goal_list root_goals = initial_goal_opener.open_goals(std::vector<const expr*>{functor("g", {var(0)}), functor("go", {})});
    const pud_body_goal_id root_call_site = root_goals.next_id - 1;
    const pud_structure_watch root_watch = watcher.watch_root(pud_ubs_state{}, root_goals);
    const pud_global_node_id child = modules.store_inference(k_module, pud_inference{
        .parent = 0,
        .call_site = root_call_site,
        .substitutions = {pud_substitution{.var_idx = 0, .value = functor("f", {var(3)})}},
        .body_goals = {},
        .var_count = 0,
    });
    pud_goal_list child_goals = goal_resolver.resolve_goal(root_goals, root_call_site, std::vector<const expr*>{functor("go", {})});
    const pud_body_goal_id child_call_site = child_goals.next_id - 1;
    const pud_ubs_state child_bindings = binder.bind(pud_ubs_state{}, child);
    const pud_structure_watch_change child_change = watcher.descend(root_watch, child_bindings, child, root_call_site, child_goals);
    ASSERT_THAT(child_change.changed_goals, ElementsAre(0u));
    const pud_global_node_id grandchild = modules.store_inference(k_module, pud_inference{
        .parent = child.node,
        .call_site = child_call_site,
        .substitutions = {pud_substitution{.var_idx = 3, .value = functor("a", {})}},
        .body_goals = {},
        .var_count = 0,
    });
    pud_goal_list grandchild_goals = goal_resolver.resolve_goal(child_goals, child_call_site, std::vector<const expr*>{functor("go", {})});
    const pud_ubs_state grandchild_bindings = binder.bind(child_bindings, grandchild);
    const pud_structure_watch_change grandchild_change =
        watcher.descend(child_change.watch, grandchild_bindings, grandchild, child_call_site, grandchild_goals);
    EXPECT_THAT(grandchild_change.changed_goals, ElementsAre(0u));
}

TEST_F(PudStructureWatcherIntegrationTest, GroundInstantiationStopsWatchingThatRep) {
    pud_goal_list root_goals = initial_goal_opener.open_goals(std::vector<const expr*>{functor("f", {var(0), var(1)}), functor("go", {})});
    const pud_body_goal_id root_call_site = root_goals.next_id - 1;
    const pud_structure_watch root_watch = watcher.watch_root(pud_ubs_state{}, root_goals);
    const pud_global_node_id child = modules.store_inference(k_module, pud_inference{
        .parent = 0,
        .call_site = root_call_site,
        .substitutions = {pud_substitution{.var_idx = 0, .value = functor("a", {})}},
        .body_goals = {},
        .var_count = 0,
    });
    pud_goal_list child_goals = goal_resolver.resolve_goal(root_goals, root_call_site, std::vector<const expr*>{functor("go", {})});
    const pud_body_goal_id child_call_site = child_goals.next_id - 1;
    const pud_ubs_state child_bindings = binder.bind(pud_ubs_state{}, child);
    const pud_structure_watch_change child_change = watcher.descend(root_watch, child_bindings, child, root_call_site, child_goals);
    const pud_global_node_id grandchild = modules.store_inference(k_module, pud_inference{
        .parent = child.node,
        .call_site = child_call_site,
        .substitutions = {pud_substitution{.var_idx = 1, .value = var(5)}},
        .body_goals = {},
        .var_count = 0,
    });
    pud_goal_list grandchild_goals = goal_resolver.resolve_goal(child_goals, child_call_site, std::vector<const expr*>{functor("go", {})});
    const pud_ubs_state grandchild_bindings = binder.bind(child_bindings, grandchild);
    const pud_structure_watch_change grandchild_change =
        watcher.descend(child_change.watch, grandchild_bindings, grandchild, child_call_site, grandchild_goals);
    EXPECT_THAT(grandchild_change.changed_goals, IsEmpty());
}

TEST_F(PudStructureWatcherIntegrationTest, UpdatingTwoRepsOfTheGoalChangesIt) {
    pud_goal_list goals = initial_goal_opener.open_goals(std::vector<const expr*>{functor("f", {var(0), var(1)}), functor("go", {})});
    const pud_body_goal_id root_call_site = goals.next_id - 1;
    const pud_structure_watch root_watch = watcher.watch_root(pud_ubs_state{}, goals);
    const pud_global_node_id child = modules.store_inference(k_module, pud_inference{
        .parent = 0,
        .call_site = root_call_site,
        .substitutions = {pud_substitution{.var_idx = 1, .value = var(0)}},
        .body_goals = {},
        .var_count = 0,
    });
    pud_goal_list child_goals = goal_resolver.resolve_goal(goals, root_call_site, std::vector<const expr*>{functor("go", {})});
    const pud_ubs_state child_bindings = binder.bind(pud_ubs_state{}, child);
    const pud_structure_watch_change change = watcher.descend(root_watch, child_bindings, child, root_call_site, child_goals);
    EXPECT_THAT(change.changed_goals, ElementsAre(0u));
}

TEST_F(PudStructureWatcherIntegrationTest, UpdatingAnOutsideRepChangesNothing) {
    pud_goal_list goals = initial_goal_opener.open_goals(std::vector<const expr*>{functor("f", {var(0), var(1)}), functor("go", {})});
    const pud_body_goal_id root_call_site = goals.next_id - 1;
    const pud_structure_watch root_watch = watcher.watch_root(pud_ubs_state{}, goals);
    const pud_global_node_id child = modules.store_inference(k_module, pud_inference{
        .parent = 0,
        .call_site = root_call_site,
        .substitutions = {pud_substitution{.var_idx = 5, .value = var(1)}},
        .body_goals = {},
        .var_count = 0,
    });
    pud_goal_list child_goals = goal_resolver.resolve_goal(goals, root_call_site, std::vector<const expr*>{functor("go", {})});
    const pud_ubs_state child_bindings = binder.bind(pud_ubs_state{}, child);
    const pud_structure_watch_change change = watcher.descend(root_watch, child_bindings, child, root_call_site, child_goals);
    EXPECT_THAT(change.changed_goals, IsEmpty());
}

TEST_F(PudStructureWatcherIntegrationTest, DemotedRepIsWatchedThroughTheSurvivingRep) {
    pud_goal_list root_goals = initial_goal_opener.open_goals(std::vector<const expr*>{functor("f", {var(4)}), functor("go", {})});
    const pud_body_goal_id root_call_site = root_goals.next_id - 1;
    const pud_structure_watch root_watch = watcher.watch_root(pud_ubs_state{}, root_goals);
    const pud_global_node_id child = modules.store_inference(k_module, pud_inference{
        .parent = 0,
        .call_site = root_call_site,
        .substitutions = {pud_substitution{.var_idx = 4, .value = var(1)}},
        .body_goals = {},
        .var_count = 0,
    });
    pud_goal_list child_goals = goal_resolver.resolve_goal(root_goals, root_call_site, std::vector<const expr*>{functor("go", {})});
    const pud_body_goal_id child_call_site = child_goals.next_id - 1;
    const pud_ubs_state child_bindings = binder.bind(pud_ubs_state{}, child);
    const pud_structure_watch_change child_change = watcher.descend(root_watch, child_bindings, child, root_call_site, child_goals);
    ASSERT_THAT(child_change.changed_goals, IsEmpty());
    const pud_global_node_id grandchild = modules.store_inference(k_module, pud_inference{
        .parent = child.node,
        .call_site = child_call_site,
        .substitutions = {pud_substitution{.var_idx = 1, .value = functor("a", {})}},
        .body_goals = {},
        .var_count = 0,
    });
    pud_goal_list grandchild_goals = goal_resolver.resolve_goal(child_goals, child_call_site, std::vector<const expr*>{functor("go", {})});
    const pud_ubs_state grandchild_bindings = binder.bind(child_bindings, grandchild);
    const pud_structure_watch_change grandchild_change =
        watcher.descend(child_change.watch, grandchild_bindings, grandchild, child_call_site, grandchild_goals);
    EXPECT_THAT(grandchild_change.changed_goals, ElementsAre(0u));
}

TEST_F(PudStructureWatcherIntegrationTest, MergingRepsOfTwoGoalsLinksBothToTheSurvivingRep) {
    pud_goal_list root_goals = initial_goal_opener.open_goals(std::vector<const expr*>{functor("f", {var(0)}), functor("g", {var(1)}), functor("go", {})});
    const pud_body_goal_id root_call_site = root_goals.next_id - 1;
    const pud_structure_watch root_watch = watcher.watch_root(pud_ubs_state{}, root_goals);
    const pud_global_node_id child = modules.store_inference(k_module, pud_inference{
        .parent = 0,
        .call_site = root_call_site,
        .substitutions = {pud_substitution{.var_idx = 1, .value = var(0)}},
        .body_goals = {},
        .var_count = 0,
    });
    pud_goal_list child_goals = goal_resolver.resolve_goal(root_goals, root_call_site, std::vector<const expr*>{functor("go", {})});
    const pud_body_goal_id child_call_site = child_goals.next_id - 1;
    const pud_ubs_state child_bindings = binder.bind(pud_ubs_state{}, child);
    const pud_structure_watch_change child_change = watcher.descend(root_watch, child_bindings, child, root_call_site, child_goals);
    ASSERT_THAT(child_change.changed_goals, IsEmpty());
    const pud_global_node_id grandchild = modules.store_inference(k_module, pud_inference{
        .parent = child.node,
        .call_site = child_call_site,
        .substitutions = {pud_substitution{.var_idx = 0, .value = functor("a", {})}},
        .body_goals = {},
        .var_count = 0,
    });
    pud_goal_list grandchild_goals = goal_resolver.resolve_goal(child_goals, child_call_site, std::vector<const expr*>{functor("go", {})});
    const pud_ubs_state grandchild_bindings = binder.bind(child_bindings, grandchild);
    const pud_structure_watch_change grandchild_change =
        watcher.descend(child_change.watch, grandchild_bindings, grandchild, child_call_site, grandchild_goals);
    EXPECT_THAT(grandchild_change.changed_goals, UnorderedElementsAre(0u, 1u));
}

TEST_F(PudStructureWatcherIntegrationTest, CollapseAcrossTwoLevels) {
    pud_goal_list root_goals = initial_goal_opener.open_goals(std::vector<const expr*>{functor("f", {var(0), var(2)}), functor("go", {})});
    const pud_body_goal_id root_call_site = root_goals.next_id - 1;
    const pud_structure_watch root_watch = watcher.watch_root(pud_ubs_state{}, root_goals);
    const pud_global_node_id child = modules.store_inference(k_module, pud_inference{
        .parent = 0,
        .call_site = root_call_site,
        .substitutions = {pud_substitution{.var_idx = 2, .value = var(1)}},
        .body_goals = {},
        .var_count = 0,
    });
    pud_goal_list child_goals = goal_resolver.resolve_goal(root_goals, root_call_site, std::vector<const expr*>{functor("go", {})});
    const pud_body_goal_id child_call_site = child_goals.next_id - 1;
    const pud_ubs_state child_bindings = binder.bind(pud_ubs_state{}, child);
    const pud_structure_watch_change child_change = watcher.descend(root_watch, child_bindings, child, root_call_site, child_goals);
    ASSERT_THAT(child_change.changed_goals, IsEmpty());
    const pud_global_node_id grandchild = modules.store_inference(k_module, pud_inference{
        .parent = child.node,
        .call_site = child_call_site,
        .substitutions = {pud_substitution{.var_idx = 1, .value = var(0)}},
        .body_goals = {},
        .var_count = 0,
    });
    pud_goal_list grandchild_goals = goal_resolver.resolve_goal(child_goals, child_call_site, std::vector<const expr*>{functor("go", {})});
    const pud_ubs_state grandchild_bindings = binder.bind(child_bindings, grandchild);
    const pud_structure_watch_change grandchild_change =
        watcher.descend(child_change.watch, grandchild_bindings, grandchild, child_call_site, grandchild_goals);
    EXPECT_THAT(grandchild_change.changed_goals, ElementsAre(0u));
}

TEST_F(PudStructureWatcherIntegrationTest, AGoalAddedAtAChildIsWatchedInThatNodesReps) {
    pud_goal_list root_goals = initial_goal_opener.open_goals(std::vector<const expr*>{functor("go", {})});
    const pud_body_goal_id root_call_site = root_goals.next_id - 1;
    const pud_structure_watch root_watch = watcher.watch_root(pud_ubs_state{}, root_goals);
    const pud_global_node_id child = modules.store_inference(k_module, pud_inference{
        .parent = 0,
        .call_site = root_call_site,
        .substitutions = {pud_substitution{.var_idx = 0, .value = functor("f", {var(3)})}},
        .body_goals = {},
        .var_count = 0,
    });
    pud_goal_list child_goals = goal_resolver.resolve_goal(root_goals, root_call_site, std::vector<const expr*>{functor("h", {functor("f", {var(3)})}), functor("go", {})});
    const pud_body_goal_id child_call_site = child_goals.next_id - 1;
    const pud_ubs_state child_bindings = binder.bind(pud_ubs_state{}, child);
    const pud_structure_watch_change child_change = watcher.descend(root_watch, child_bindings, child, root_call_site, child_goals);
    ASSERT_THAT(child_change.changed_goals, IsEmpty());
    const pud_global_node_id grandchild = modules.store_inference(k_module, pud_inference{
        .parent = child.node,
        .call_site = child_call_site,
        .substitutions = {pud_substitution{.var_idx = 3, .value = functor("a", {})}},
        .body_goals = {},
        .var_count = 0,
    });
    pud_goal_list grandchild_goals = goal_resolver.resolve_goal(child_goals, child_call_site, std::vector<const expr*>{functor("go", {})});
    const pud_ubs_state grandchild_bindings = binder.bind(child_bindings, grandchild);
    const pud_structure_watch_change grandchild_change =
        watcher.descend(child_change.watch, grandchild_bindings, grandchild, child_call_site, grandchild_goals);
    EXPECT_THAT(grandchild_change.changed_goals, ElementsAre(1u));
}

TEST_F(PudStructureWatcherIntegrationTest, AGoalAddedAtAChildUsesTheSurvivingRep) {
    pud_goal_list root_goals = initial_goal_opener.open_goals(std::vector<const expr*>{functor("go", {})});
    const pud_body_goal_id root_call_site = root_goals.next_id - 1;
    const pud_structure_watch root_watch = watcher.watch_root(pud_ubs_state{}, root_goals);
    const pud_global_node_id child = modules.store_inference(k_module, pud_inference{
        .parent = 0,
        .call_site = root_call_site,
        .substitutions = {pud_substitution{.var_idx = 4, .value = var(1)}},
        .body_goals = {},
        .var_count = 0,
    });
    pud_goal_list child_goals = goal_resolver.resolve_goal(root_goals, root_call_site, std::vector<const expr*>{functor("h", {var(4), var(0)}), functor("go", {})});
    const pud_body_goal_id child_call_site = child_goals.next_id - 1;
    const pud_ubs_state child_bindings = binder.bind(pud_ubs_state{}, child);
    const pud_structure_watch_change child_change = watcher.descend(root_watch, child_bindings, child, root_call_site, child_goals);
    const pud_global_node_id grandchild = modules.store_inference(k_module, pud_inference{
        .parent = child.node,
        .call_site = child_call_site,
        .substitutions = {pud_substitution{.var_idx = 1, .value = var(0)}},
        .body_goals = {},
        .var_count = 0,
    });
    pud_goal_list grandchild_goals = goal_resolver.resolve_goal(child_goals, child_call_site, std::vector<const expr*>{functor("go", {})});
    const pud_ubs_state grandchild_bindings = binder.bind(child_bindings, grandchild);
    const pud_structure_watch_change grandchild_change =
        watcher.descend(child_change.watch, grandchild_bindings, grandchild, child_call_site, grandchild_goals);
    EXPECT_THAT(grandchild_change.changed_goals, ElementsAre(1u));
}

TEST_F(PudStructureWatcherIntegrationTest, WatchUnwatchAcrossThreeLevels) {
    pud_goal_list root_goals = initial_goal_opener.open_goals(std::vector<const expr*>{var(0), var(1), functor("go", {})});
    const pud_body_goal_id root_call_site = root_goals.next_id - 1;
    const pud_structure_watch root_watch = watcher.watch_root(pud_ubs_state{}, root_goals);
    const pud_global_node_id child = modules.store_inference(k_module, pud_inference{
        .parent = 0,
        .call_site = 1,
        .substitutions = {pud_substitution{.var_idx = 0, .value = functor("f", {var(5)})}},
        .body_goals = {},
        .var_count = 0,
    });
    pud_goal_list child_goals = goal_resolver.resolve_goal(root_goals, 1, std::vector<const expr*>{functor("g", {var(6)})});
    const pud_ubs_state child_bindings = binder.bind(pud_ubs_state{}, child);
    const pud_structure_watch_change child_change = watcher.descend(root_watch, child_bindings, child, 1, child_goals);
    EXPECT_THAT(child_change.changed_goals, ElementsAre(0u));
    const pud_global_node_id grandchild = modules.store_inference(k_module, pud_inference{
        .parent = child.node,
        .call_site = root_call_site,
        .substitutions = {pud_substitution{.var_idx = 6, .value = var(5)}},
        .body_goals = {},
        .var_count = 0,
    });
    pud_goal_list grandchild_goals = goal_resolver.resolve_goal(child_goals, root_call_site, std::vector<const expr*>{var(5), functor("go", {})});
    const pud_body_goal_id grandchild_call_site = grandchild_goals.next_id - 1;
    const pud_ubs_state grandchild_bindings = binder.bind(child_bindings, grandchild);
    const pud_structure_watch_change grandchild_change =
        watcher.descend(child_change.watch, grandchild_bindings, grandchild, root_call_site, grandchild_goals);
    EXPECT_THAT(grandchild_change.changed_goals, IsEmpty());
    const pud_global_node_id great_grandchild = modules.store_inference(k_module, pud_inference{
        .parent = grandchild.node,
        .call_site = grandchild_call_site,
        .substitutions = {pud_substitution{.var_idx = 5, .value = functor("a", {})}},
        .body_goals = {},
        .var_count = 0,
    });
    pud_goal_list great_grandchild_goals = goal_resolver.resolve_goal(grandchild_goals, grandchild_call_site, std::vector<const expr*>{functor("go", {})});
    const pud_ubs_state great_grandchild_bindings = binder.bind(grandchild_bindings, great_grandchild);
    const pud_structure_watch_change great_grandchild_change = watcher.descend(
        grandchild_change.watch, great_grandchild_bindings, great_grandchild, grandchild_call_site, great_grandchild_goals);
    EXPECT_THAT(great_grandchild_change.changed_goals, UnorderedElementsAre(0u, 3u, 4u));
}

TEST_F(PudStructureWatcherIntegrationTest, UnrelatedBindingsLeaveStructureAlone) {
    pud_goal_list goals = initial_goal_opener.open_goals(std::vector<const expr*>{functor("f", {var(0), var(1)}), functor("go", {})});
    const pud_body_goal_id root_call_site = goals.next_id - 1;
    const pud_structure_watch root_watch = watcher.watch_root(pud_ubs_state{}, goals);
    const pud_global_node_id child = modules.store_inference(k_module, pud_inference{
        .parent = 0,
        .call_site = root_call_site,
        .substitutions = {
            pud_substitution{.var_idx = 3, .value = var(2)},
            pud_substitution{.var_idx = 4, .value = functor("a", {})},
            pud_substitution{.var_idx = 5, .value = var(1)},
        },
        .body_goals = {},
        .var_count = 0,
    });
    pud_goal_list child_goals = goal_resolver.resolve_goal(goals, root_call_site, std::vector<const expr*>{functor("go", {})});
    const pud_ubs_state child_bindings = binder.bind(pud_ubs_state{}, child);
    const pud_structure_watch_change change = watcher.descend(root_watch, child_bindings, child, root_call_site, child_goals);
    EXPECT_THAT(change.changed_goals, IsEmpty());
}

TEST_F(PudStructureWatcherIntegrationTest, GoalChangedTwiceAtOneNodeIsReportedOnce) {
    pud_goal_list goals = initial_goal_opener.open_goals(std::vector<const expr*>{functor("f", {var(0), var(1), var(2)}), functor("go", {})});
    const pud_body_goal_id root_call_site = goals.next_id - 1;
    const pud_structure_watch root_watch = watcher.watch_root(pud_ubs_state{}, goals);
    const pud_global_node_id child = modules.store_inference(k_module, pud_inference{
        .parent = 0,
        .call_site = root_call_site,
        .substitutions = {
            pud_substitution{.var_idx = 1, .value = var(0)},
            pud_substitution{.var_idx = 2, .value = functor("a", {})},
        },
        .body_goals = {},
        .var_count = 0,
    });
    pud_goal_list child_goals = goal_resolver.resolve_goal(goals, root_call_site, std::vector<const expr*>{functor("go", {})});
    const pud_ubs_state child_bindings = binder.bind(pud_ubs_state{}, child);
    const pud_structure_watch_change change = watcher.descend(root_watch, child_bindings, child, root_call_site, child_goals);
    EXPECT_THAT(change.changed_goals, ElementsAre(0u));
}

TEST_F(PudStructureWatcherIntegrationTest, InstantiationValueIsWrittenInTheNodesReps) {
    pud_goal_list root_goals = initial_goal_opener.open_goals(std::vector<const expr*>{functor("f", {var(0), var(1)}), functor("go", {})});
    const pud_body_goal_id root_call_site = root_goals.next_id - 1;
    const pud_structure_watch root_watch = watcher.watch_root(pud_ubs_state{}, root_goals);
    const pud_global_node_id child = modules.store_inference(k_module, pud_inference{
        .parent = 0,
        .call_site = root_call_site,
        .substitutions = {pud_substitution{.var_idx = 3, .value = var(1)}},
        .body_goals = {},
        .var_count = 0,
    });
    pud_goal_list child_goals = goal_resolver.resolve_goal(root_goals, root_call_site, std::vector<const expr*>{functor("go", {})});
    const pud_body_goal_id child_call_site = child_goals.next_id - 1;
    const pud_ubs_state child_bindings = binder.bind(pud_ubs_state{}, child);
    const pud_structure_watch_change child_change = watcher.descend(root_watch, child_bindings, child, root_call_site, child_goals);
    const pud_global_node_id grandchild = modules.store_inference(k_module, pud_inference{
        .parent = child.node,
        .call_site = child_call_site,
        .substitutions = {pud_substitution{.var_idx = 0, .value = functor("g", {var(1)})}},
        .body_goals = {},
        .var_count = 0,
    });
    pud_goal_list grandchild_goals = goal_resolver.resolve_goal(child_goals, child_call_site, std::vector<const expr*>{functor("go", {})});
    const pud_body_goal_id grandchild_call_site = grandchild_goals.next_id - 1;
    const pud_ubs_state grandchild_bindings = binder.bind(child_bindings, grandchild);
    const pud_structure_watch_change grandchild_change =
        watcher.descend(child_change.watch, grandchild_bindings, grandchild, child_call_site, grandchild_goals);
    ASSERT_THAT(grandchild_change.changed_goals, ElementsAre(0u));
    const pud_global_node_id great_grandchild = modules.store_inference(k_module, pud_inference{
        .parent = grandchild.node,
        .call_site = grandchild_call_site,
        .substitutions = {pud_substitution{.var_idx = 1, .value = functor("a", {})}},
        .body_goals = {},
        .var_count = 0,
    });
    pud_goal_list great_grandchild_goals = goal_resolver.resolve_goal(grandchild_goals, grandchild_call_site, std::vector<const expr*>{functor("go", {})});
    const pud_ubs_state great_grandchild_bindings = binder.bind(grandchild_bindings, great_grandchild);
    const pud_structure_watch_change great_grandchild_change = watcher.descend(
        grandchild_change.watch, great_grandchild_bindings, great_grandchild, grandchild_call_site, great_grandchild_goals);
    EXPECT_THAT(great_grandchild_change.changed_goals, ElementsAre(0u));
}

TEST_F(PudStructureWatcherIntegrationTest, SiblingBranchesSeeTheirOwnChanges) {
    pud_goal_list root_goals = initial_goal_opener.open_goals(std::vector<const expr*>{functor("f", {var(0)}), functor("g", {var(1)}), functor("go", {})});
    const pud_body_goal_id root_call_site = root_goals.next_id - 1;
    const pud_structure_watch root_watch = watcher.watch_root(pud_ubs_state{}, root_goals);
    const pud_global_node_id child = modules.store_inference(k_module, pud_inference{
        .parent = 0,
        .call_site = root_call_site,
        .substitutions = {pud_substitution{.var_idx = 0, .value = functor("a", {})}},
        .body_goals = {},
        .var_count = 0,
    });
    pud_goal_list child_probe_goals = goal_resolver.resolve_goal(root_goals, root_call_site, std::vector<const expr*>{functor("go", {})});
    const pud_ubs_state child_probe_bindings = binder.bind(pud_ubs_state{}, child);
    const pud_structure_watch_change child_probe_change =
        watcher.descend(root_watch, child_probe_bindings, child, root_call_site, child_probe_goals);
    EXPECT_THAT(child_probe_change.changed_goals, ElementsAre(0u));
    const pud_global_node_id sibling = modules.store_inference(k_module, pud_inference{
        .parent = 0,
        .call_site = root_call_site,
        .substitutions = {pud_substitution{.var_idx = 1, .value = functor("a", {})}},
        .body_goals = {},
        .var_count = 0,
    });
    pud_goal_list sibling_goals = goal_resolver.resolve_goal(root_goals, root_call_site, std::vector<const expr*>{functor("go", {})});
    const pud_ubs_state sibling_bindings = binder.bind(pud_ubs_state{}, sibling);
    const pud_structure_watch_change sibling_change =
        watcher.descend(root_watch, sibling_bindings, sibling, root_call_site, sibling_goals);
    EXPECT_THAT(sibling_change.changed_goals, ElementsAre(1u));
}

TEST_F(PudStructureWatcherIntegrationTest, ChangeInOneBranchDoesNotLeakIntoTheOther) {
    pud_goal_list root_goals = initial_goal_opener.open_goals(std::vector<const expr*>{functor("f", {var(0), var(1)}), functor("go", {})});
    const pud_body_goal_id root_call_site = root_goals.next_id - 1;
    const pud_structure_watch root_watch = watcher.watch_root(pud_ubs_state{}, root_goals);
    const pud_global_node_id child = modules.store_inference(k_module, pud_inference{
        .parent = 0,
        .call_site = root_call_site,
        .substitutions = {pud_substitution{.var_idx = 1, .value = var(0)}},
        .body_goals = {},
        .var_count = 0,
    });
    pud_goal_list child_probe_goals = goal_resolver.resolve_goal(root_goals, root_call_site, std::vector<const expr*>{functor("go", {})});
    const pud_ubs_state child_probe_bindings = binder.bind(pud_ubs_state{}, child);
    const pud_structure_watch_change child_probe_change =
        watcher.descend(root_watch, child_probe_bindings, child, root_call_site, child_probe_goals);
    ASSERT_THAT(child_probe_change.changed_goals, ElementsAre(0u));
    const pud_global_node_id sibling = modules.store_inference(k_module, pud_inference{
        .parent = 0,
        .call_site = root_call_site,
        .substitutions = {pud_substitution{.var_idx = 5, .value = functor("a", {})}},
        .body_goals = {},
        .var_count = 0,
    });
    pud_goal_list sibling_goals = goal_resolver.resolve_goal(root_goals, root_call_site, std::vector<const expr*>{functor("go", {})});
    const pud_body_goal_id sibling_call_site = sibling_goals.next_id - 1;
    const pud_ubs_state sibling_bindings = binder.bind(pud_ubs_state{}, sibling);
    const pud_structure_watch_change sibling_change =
        watcher.descend(root_watch, sibling_bindings, sibling, root_call_site, sibling_goals);
    EXPECT_THAT(sibling_change.changed_goals, IsEmpty());
    const pud_global_node_id grandchild = modules.store_inference(k_module, pud_inference{
        .parent = sibling.node,
        .call_site = sibling_call_site,
        .substitutions = {pud_substitution{.var_idx = 1, .value = var(0)}},
        .body_goals = {},
        .var_count = 0,
    });
    pud_goal_list grandchild_goals = goal_resolver.resolve_goal(sibling_goals, sibling_call_site, std::vector<const expr*>{functor("go", {})});
    const pud_ubs_state grandchild_bindings = binder.bind(sibling_bindings, grandchild);
    const pud_structure_watch_change grandchild_change =
        watcher.descend(sibling_change.watch, grandchild_bindings, grandchild, sibling_call_site, grandchild_goals);
    EXPECT_THAT(grandchild_change.changed_goals, ElementsAre(0u));
}

TEST_F(PudStructureWatcherIntegrationTest, ChangesFollowTheStoredSubstitutionOrder) {
    pud_goal_list goals = initial_goal_opener.open_goals(std::vector<const expr*>{functor("f", {var(0), var(1)}), var(2), var(3), functor("go", {})});
    const pud_body_goal_id root_call_site = goals.next_id - 1;
    const pud_structure_watch root_watch = watcher.watch_root(pud_ubs_state{}, goals);
    const pud_global_node_id child = modules.store_inference(k_module, pud_inference{
        .parent = 0,
        .call_site = root_call_site,
        .substitutions = {
            pud_substitution{.var_idx = 2, .value = functor("a", {})},
            pud_substitution{.var_idx = 1, .value = var(0)},
            pud_substitution{.var_idx = 3, .value = functor("b", {})},
        },
        .body_goals = {},
        .var_count = 0,
    });
    pud_goal_list child_goals = goal_resolver.resolve_goal(goals, root_call_site, std::vector<const expr*>{functor("go", {})});
    const pud_ubs_state child_bindings = binder.bind(pud_ubs_state{}, child);
    const pud_structure_watch_change change = watcher.descend(root_watch, child_bindings, child, root_call_site, child_goals);
    EXPECT_THAT(change.changed_goals, ElementsAre(1u, 0u, 2u));
}

TEST_F(PudStructureWatcherIntegrationTest, InternedDuplicateGoalsAreWatchedSeparately) {
    const expr* first = functor("f", {var(0)});
    const expr* second = functor("f", {var(0)});
    ASSERT_EQ(first, second);
    pud_goal_list goals = initial_goal_opener.open_goals(std::vector<const expr*>{first, second, functor("go", {})});
    const pud_body_goal_id root_call_site = goals.next_id - 1;
    const pud_structure_watch root_watch = watcher.watch_root(pud_ubs_state{}, goals);
    const pud_global_node_id child = modules.store_inference(k_module, pud_inference{
        .parent = 0,
        .call_site = root_call_site,
        .substitutions = {pud_substitution{.var_idx = 0, .value = functor("a", {})}},
        .body_goals = {},
        .var_count = 0,
    });
    pud_goal_list child_goals = goal_resolver.resolve_goal(goals, root_call_site, std::vector<const expr*>{functor("go", {})});
    const pud_ubs_state child_bindings = binder.bind(pud_ubs_state{}, child);
    const pud_structure_watch_change change = watcher.descend(root_watch, child_bindings, child, root_call_site, child_goals);
    EXPECT_THAT(change.changed_goals, UnorderedElementsAre(0u, 1u));
}

TEST_F(PudStructureWatcherIntegrationTest, LongRenamingChainAcrossStoredLevels) {
    const uint32_t chain_length = 200;
    pud_goal_list goals = initial_goal_opener.open_goals(std::vector<const expr*>{functor("f", {var(0), var(chain_length)}), functor("go", {})});
    pud_body_goal_id call_site = goals.next_id - 1;
    pud_ubs_state bindings = pud_ubs_state{};
    pud_structure_watch watch = watcher.watch_root(pud_ubs_state{}, goals);
    std::vector<pud_body_goal_id> changed_goals;
    pud_global_node_id tree_node{.module = k_module, .node = 0};
    for (uint32_t step_index = 0; step_index < chain_length; ++step_index) {
        const uint32_t demoted = chain_length - step_index;
        const pud_body_goal_id closed = call_site;
        const pud_global_node_id level_node = modules.store_inference(k_module, pud_inference{
            .parent = tree_node.node,
            .call_site = closed,
            .substitutions = {pud_substitution{.var_idx = demoted, .value = var(demoted - 1)}},
            .body_goals = {},
            .var_count = 0,
        });
        pud_goal_list step_goals = goal_resolver.resolve_goal(goals, closed, std::vector<const expr*>{functor("go", {})});
        call_site = step_goals.next_id - 1;
        bindings = binder.bind(bindings, level_node);
        const pud_structure_watch_change change = watcher.descend(watch, bindings, level_node, closed, step_goals);
        watch = change.watch;
        goals = step_goals;
        changed_goals = change.changed_goals;
        tree_node = level_node;
        const bool last_step = step_index + 1 == chain_length;
        EXPECT_EQ(changed_goals.size(), last_step ? 1u : 0u) << "step " << step_index;
    }
}

TEST_F(PudStructureWatcherIntegrationTest, AClosedGoalIsNoLongerWatchedButItsReplacementIs) {
    pud_goal_list root_goals = initial_goal_opener.open_goals(std::vector<const expr*>{functor("f", {var(4)}), functor("go", {})});
    const pud_body_goal_id root_call_site = root_goals.next_id - 1;
    const pud_structure_watch root_watch = watcher.watch_root(pud_ubs_state{}, root_goals);
    const pud_global_node_id child = modules.store_inference(k_module, pud_inference{
        .parent = 0,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 4, .value = var(2)}},
        .body_goals = {},
        .var_count = 0,
    });
    pud_goal_list child_goals = goal_resolver.resolve_goal(root_goals, 0, std::vector<const expr*>{functor("f", {var(2)})});
    const pud_ubs_state child_bindings = binder.bind(pud_ubs_state{}, child);
    const pud_structure_watch_change child_change = watcher.descend(root_watch, child_bindings, child, 0, child_goals);
    ASSERT_THAT(child_change.changed_goals, IsEmpty());
    const pud_global_node_id grandchild = modules.store_inference(k_module, pud_inference{
        .parent = child.node,
        .call_site = root_call_site,
        .substitutions = {pud_substitution{.var_idx = 2, .value = functor("a", {})}},
        .body_goals = {},
        .var_count = 0,
    });
    pud_goal_list grandchild_goals = goal_resolver.resolve_goal(child_goals, root_call_site, std::vector<const expr*>{functor("go", {})});
    const pud_ubs_state grandchild_bindings = binder.bind(child_bindings, grandchild);
    const pud_structure_watch_change grandchild_change =
        watcher.descend(child_change.watch, grandchild_bindings, grandchild, root_call_site, grandchild_goals);
    EXPECT_THAT(grandchild_change.changed_goals, ElementsAre(2u));
}

TEST_F(PudStructureWatcherIntegrationTest, ClosingInOneBranchDoesNotAffectTheOther) {
    pud_goal_list root_goals = initial_goal_opener.open_goals(std::vector<const expr*>{functor("f", {var(0)}), functor("g", {var(0)}), functor("go", {})});
    const pud_body_goal_id root_call_site = root_goals.next_id - 1;
    const pud_structure_watch root_watch = watcher.watch_root(pud_ubs_state{}, root_goals);
    const pud_global_node_id mid = modules.store_inference(k_module, pud_inference{
        .parent = 0,
        .call_site = root_call_site,
        .substitutions = {},
        .body_goals = {},
        .var_count = 0,
    });
    pud_goal_list mid_goals = goal_resolver.resolve_goal(root_goals, root_call_site, std::vector<const expr*>{functor("go", {})});
    const pud_ubs_state mid_bindings = binder.bind(pud_ubs_state{}, mid);
    const pud_structure_watch_change mid_change =
        watcher.descend(root_watch, mid_bindings, mid, root_call_site, mid_goals);
    const pud_global_node_id child = modules.store_inference(k_module, pud_inference{
        .parent = mid.node,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 0, .value = functor("a", {})}},
        .body_goals = {},
        .var_count = 0,
    });
    const pud_goal_list child_closing_goals = goal_resolver.resolve_goal(mid_goals, 0, std::vector<const expr*>{});
    const pud_ubs_state child_closing_bindings = binder.bind(mid_bindings, child);
    const pud_structure_watch_change child_closing_change =
        watcher.descend(mid_change.watch, child_closing_bindings, child, 0, child_closing_goals);
    EXPECT_THAT(child_closing_change.changed_goals, ElementsAre(1u));
    const pud_global_node_id sibling = modules.store_inference(k_module, pud_inference{
        .parent = 0,
        .call_site = root_call_site,
        .substitutions = {pud_substitution{.var_idx = 0, .value = functor("a", {})}},
        .body_goals = {},
        .var_count = 0,
    });
    pud_goal_list sibling_goals = goal_resolver.resolve_goal(root_goals, root_call_site, std::vector<const expr*>{functor("go", {})});
    const pud_ubs_state sibling_bindings = binder.bind(pud_ubs_state{}, sibling);
    const pud_structure_watch_change sibling_change =
        watcher.descend(root_watch, sibling_bindings, sibling, root_call_site, sibling_goals);
    EXPECT_THAT(sibling_change.changed_goals, UnorderedElementsAre(0u, 1u));
}

TEST_F(PudStructureWatcherIntegrationTest, AnAddedGoalOverABoundVarIsWatchedThroughItsValue) {
    pud_goal_list root_goals = initial_goal_opener.open_goals(std::vector<const expr*>{functor("go", {})});
    const pud_body_goal_id root_call_site = root_goals.next_id - 1;
    const pud_structure_watch root_watch = watcher.watch_root(pud_ubs_state{}, root_goals);
    const pud_global_node_id child = modules.store_inference(k_module, pud_inference{
        .parent = 0,
        .call_site = root_call_site,
        .substitutions = {pud_substitution{.var_idx = 0, .value = functor("s", {var(3)})}},
        .body_goals = {},
        .var_count = 0,
    });
    pud_goal_list child_goals = goal_resolver.resolve_goal(root_goals, root_call_site, std::vector<const expr*>{functor("g", {var(0)}), functor("go", {})});
    const pud_body_goal_id child_call_site = child_goals.next_id - 1;
    const pud_ubs_state child_bindings = binder.bind(pud_ubs_state{}, child);
    const pud_structure_watch_change child_change = watcher.descend(root_watch, child_bindings, child, root_call_site, child_goals);
    ASSERT_THAT(child_change.changed_goals, IsEmpty());
    const pud_global_node_id grandchild = modules.store_inference(k_module, pud_inference{
        .parent = child.node,
        .call_site = child_call_site,
        .substitutions = {pud_substitution{.var_idx = 3, .value = functor("a", {})}},
        .body_goals = {},
        .var_count = 0,
    });
    pud_goal_list grandchild_goals = goal_resolver.resolve_goal(child_goals, child_call_site, std::vector<const expr*>{functor("go", {})});
    const pud_ubs_state grandchild_bindings = binder.bind(child_bindings, grandchild);
    const pud_structure_watch_change grandchild_change =
        watcher.descend(child_change.watch, grandchild_bindings, grandchild, child_call_site, grandchild_goals);
    EXPECT_THAT(grandchild_change.changed_goals, ElementsAre(1u));
}
