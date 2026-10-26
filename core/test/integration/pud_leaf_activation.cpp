#include <deque>
#include <optional>
#include <vector>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "functor_fixture.hpp"
#include "pud_test_node.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/pud_binder.hpp"
#include "infrastructure/pud_child_activator.hpp"
#include "infrastructure/pud_goal_resolver.hpp"
#include "infrastructure/pud_initial_goal_opener.hpp"
#include "infrastructure/pud_node_deactivator.hpp"
#include "infrastructure/pud_root_activator.hpp"
#include "infrastructure/pud_module.hpp"
#include "infrastructure/pud_mhcs.hpp"
#include "infrastructure/pud_mhq.hpp"
#include "infrastructure/pud_mhws.hpp"
#include "infrastructure/pud_modules.hpp"
#include "infrastructure/pud_node_axioms.hpp"
#include "infrastructure/pud_node_goals.hpp"
#include "infrastructure/pud_node_live_child_counts.hpp"
#include "infrastructure/pud_active_states.hpp"
#include "infrastructure/pud_query_holders.hpp"
#include "infrastructure/pud_replayer.hpp"
#include "infrastructure/pud_structure_watcher.hpp"
#include "infrastructure/pud_term_factory.hpp"
#include "value_objects/rule.hpp"
#include <immer/map.hpp>

using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::UnorderedElementsAre;

// Leaf activation over the real binder, watcher, search stack, and explicit query holders. The tree's
// root and the facts its goals resolve against are the axioms of one module.
struct PudLeafActivationIntegrationTest : public ::testing::Test {
    using term_factory_t = pud_term_factory<globalizer, expr_pool, expr_pool>;
    using unit_queries_t = std::deque<pud_mhq_head_id>;
    using modules_t = pud_modules<pud_module>;
    using replayer_t = pud_replayer<modules_t, modules_t, expr_pool, term_factory_t, term_factory_t, term_factory_t>;
    using mhws_t = pud_mhws<replayer_t, modules_t, modules_t, modules_t>;
    using mhcs_t =
        pud_mhcs<replayer_t, mhws_t, mhws_t, mhws_t, mhws_t, mhws_t, modules_t, modules_t, modules_t>;
    using mhq_t = pud_mhq<replayer_t, modules_t, mhcs_t, mhcs_t, mhcs_t, mhcs_t, unit_queries_t>;
    using binder_t = pud_binder<term_factory_t, modules_t>;
    using watcher_t = pud_structure_watcher<term_factory_t, modules_t, expr_pool>;
    using root_activator_t = pud_root_activator<
        modules_t, pud_initial_goal_opener, pud_node_goals, pud_node_axioms, pud_active_states, watcher_t, mhq_t,
        pud_query_holders>;
    using child_activator_t = pud_child_activator<
        pud_active_states, pud_active_states, modules_t, pud_node_goals, pud_node_axioms, pud_goal_resolver,
        pud_node_goals, pud_node_goals, pud_node_axioms, pud_node_live_child_counts, binder_t, watcher_t, mhq_t,
        mhq_t, pud_query_holders, mhq_t>;
    using node_deactivator_t =
        pud_node_deactivator<pud_active_states, pud_active_states, pud_query_holders, mhq_t, mhq_t, pud_query_holders>;

    static constexpr pud_module_id k_module = 0;
    static constexpr pud_module_id k_lone_module = 1;

    test_functors functors;
    expr_pool exprs;
    globalizer globalize;
    term_factory_t terms{globalize, exprs, exprs};
    std::deque<pud_module> module_data;
    modules_t modules;
    pud_node_goals node_goals;
    pud_node_axioms node_axioms;
    pud_goal_resolver goal_resolver;
    pud_initial_goal_opener initial_goal_opener;
    pud_node_live_child_counts live_child_counts;
    unit_queries_t unit_queries;
    pud_active_states active_states;
    pud_query_holders query_holders;
    replayer_t replayer{modules, modules, exprs, terms, terms, terms};
    mhws_t mhws{replayer, modules, modules, modules};
    mhcs_t mhcs{replayer, mhws, mhws, mhws, mhws, mhws, modules, modules, modules};
    mhq_t mhq{replayer, modules, mhcs, mhcs, mhcs, mhcs, unit_queries};
    binder_t binder{terms, modules};
    watcher_t structure_watcher{terms, modules, exprs};
    root_activator_t root_activator{
        modules, initial_goal_opener, node_goals, node_axioms, active_states, structure_watcher, mhq, query_holders};
    child_activator_t child_activator{
        active_states, active_states, modules, node_goals, node_axioms, goal_resolver, node_goals, node_goals,
        node_axioms, live_child_counts, binder, structure_watcher, mhq, mhq, query_holders, mhq};
    node_deactivator_t node_deactivator{active_states, active_states, query_holders, mhq, mhq, query_holders};

    pud_global_node_id root{};
    pud_global_node_id child{};
    pud_global_node_id grandchild{};
    pud_global_node_id nephew{};
    pud_global_node_id great_nephew{};
    pud_global_node_id sibling{};
    pud_global_node_id linker{};
    pud_global_node_id linked{};

    const expr* p0 = nullptr;
    const expr* q12 = nullptr;
    const expr* r3 = nullptr;
    const expr* r1 = nullptr;
    const expr* s = nullptr;

    const expr* var(uint32_t index) { return exprs.make_var(index); }

    const expr* functor(const char* name, std::vector<const expr*> args) {
        return exprs.make_functor(functors.id(name), std::move(args));
    }

    const expr* atom(const char* name) { return functor(name, {}); }

    void SetUp() override {
        p0 = functor("p", {var(0)});
        q12 = functor("q", {var(1), var(2)});
        r3 = functor("r", {var(3)});
        r1 = functor("r", {var(1)});
        s = atom("s");
        module_data.emplace_back(
            k_module,
            std::vector<pud_module_id>{},
            std::vector<rule>{
                rule(p0, {p0, q12}, 3),
                rule(functor("p", {var(0)}), {}, 0),
                rule(functor("q", {var(1), var(2)}), {}, 0),
                rule(functor("r", {var(0)}), {}, 0),
                rule(functor("r", {var(1)}), {}, 0),
                rule(atom("s"), {}, 0),
            });
        modules.register_module(module_data.back());
        root = modules.list_axioms(k_module).front();
        child = modules.store_inference(root.module, pud_inference{
            .parent = root.node,
            .call_site = 0,
            .substitutions = {pud_substitution{.var_idx = 0, .value = atom("a")}},
            .body_goals = {r3},
            .var_count = 1,
        });
        grandchild = modules.store_inference(child.module, pud_inference{
            .parent = child.node,
            .call_site = 2,
            .substitutions = {pud_substitution{.var_idx = 3, .value = atom("b")}},
            .body_goals = {},
            .var_count = 0,
        });
        nephew = modules.store_inference(child.module, pud_inference{
            .parent = child.node,
            .call_site = 2,
            .substitutions = {pud_substitution{.var_idx = 2, .value = functor("f", {var(3)})}},
            .body_goals = {s},
            .var_count = 0,
        });
        great_nephew = modules.store_inference(nephew.module, pud_inference{
            .parent = nephew.node,
            .call_site = 3,
            .substitutions = {pud_substitution{.var_idx = 1, .value = atom("c")}},
            .body_goals = {},
            .var_count = 0,
        });
        sibling = modules.store_inference(root.module, pud_inference{
            .parent = root.node,
            .call_site = 0,
            .substitutions = {pud_substitution{.var_idx = 1, .value = var(2)}},
            .body_goals = {},
            .var_count = 0,
        });
        linker = modules.store_inference(root.module, pud_inference{
            .parent = root.node,
            .call_site = 0,
            .substitutions = {},
            .body_goals = {r1},
            .var_count = 0,
        });
        linked = modules.store_inference(linker.module, pud_inference{
            .parent = linker.node,
            .call_site = 2,
            .substitutions = {pud_substitution{.var_idx = 1, .value = atom("a")}},
            .body_goals = {},
            .var_count = 0,
        });
    }
};

TEST_F(PudLeafActivationIntegrationTest, RootAdmitsAQueryPerOpenGoal) {
    EXPECT_FALSE(root_activator.activate_root(root));
    const immer::map<pud_body_goal_id, pud_mhq_head_id>& queries = active_states.get(root).queries;
    EXPECT_NE(queries.find(0), nullptr);
    EXPECT_NE(queries.find(1), nullptr);
    EXPECT_EQ(queries.size(), 2u);
    EXPECT_EQ(query_holders.get_holders(active_states.get(root).queries.at(0)).size(), 1u);
    EXPECT_EQ(query_holders.get_holders(active_states.get(root).queries.at(1)).size(), 1u);
}

TEST_F(PudLeafActivationIntegrationTest, ChildAdmitsAQueryForEachNewGoal) {
    EXPECT_FALSE(root_activator.activate_root(root));
    EXPECT_FALSE(child_activator.activate_child(modules.get_parent(child).value(), child));
    EXPECT_TRUE(active_states.get(child).queries.find(2) != nullptr);
    EXPECT_EQ(query_holders.get_holders(active_states.get(child).queries.at(2)).front(), child);
}

TEST_F(PudLeafActivationIntegrationTest, FactChildWithNoGoalsOpensNothingNew) {
    EXPECT_FALSE(root_activator.activate_root(root));
    EXPECT_FALSE(child_activator.activate_child(modules.get_parent(sibling).value(), sibling));
    EXPECT_EQ(active_states.get(sibling).queries.find(2), nullptr);
}

TEST_F(PudLeafActivationIntegrationTest, BindingTheClosedGoalDoesNotForkItsQuery) {
    EXPECT_FALSE(root_activator.activate_root(root));
    const pud_mhq_head_id before = active_states.get(root).queries.at(1);
    EXPECT_FALSE(child_activator.activate_child(modules.get_parent(child).value(), child));
    EXPECT_EQ(active_states.get(child).queries.at(1), before);
}

TEST_F(PudLeafActivationIntegrationTest, CollapseForksTheAffectedGoal) {
    EXPECT_FALSE(root_activator.activate_root(root));
    const pud_mhq_head_id before = active_states.get(root).queries.at(1);
    EXPECT_FALSE(child_activator.activate_child(modules.get_parent(sibling).value(), sibling));
    EXPECT_NE(active_states.get(sibling).queries.at(1), before);
}

TEST_F(PudLeafActivationIntegrationTest, InstantiationForksTheAffectedGoal) {
    EXPECT_FALSE(root_activator.activate_root(root));
    EXPECT_FALSE(child_activator.activate_child(modules.get_parent(child).value(), child));
    const pud_mhq_head_id before = active_states.get(child).queries.at(1);
    EXPECT_FALSE(child_activator.activate_child(modules.get_parent(nephew).value(), nephew));
    EXPECT_NE(active_states.get(nephew).queries.at(1), before);
}

TEST_F(PudLeafActivationIntegrationTest, ForkedQueryShadowsTheParentsQuery) {
    EXPECT_FALSE(root_activator.activate_root(root));
    EXPECT_FALSE(child_activator.activate_child(modules.get_parent(child).value(), child));
    const pud_mhq_head_id at_child = active_states.get(child).queries.at(1);
    EXPECT_FALSE(child_activator.activate_child(modules.get_parent(nephew).value(), nephew));
    const pud_mhq_head_id at_nephew = active_states.get(nephew).queries.at(1);
    EXPECT_NE(at_nephew, at_child);
    EXPECT_FALSE(child_activator.activate_child(modules.get_parent(great_nephew).value(), great_nephew));
    EXPECT_NE(active_states.get(great_nephew).queries.at(1), at_nephew);
}

TEST_F(PudLeafActivationIntegrationTest, SiblingsForkFromTheSameRootQuery) {
    EXPECT_FALSE(root_activator.activate_root(root));
    const pud_mhq_head_id at_root = active_states.get(root).queries.at(1);
    EXPECT_FALSE(child_activator.activate_child(modules.get_parent(child).value(), child));
    EXPECT_FALSE(child_activator.activate_child(modules.get_parent(sibling).value(), sibling));
    const pud_mhq_head_id at_sibling = active_states.get(sibling).queries.at(1);
    EXPECT_NE(at_sibling, at_root);
    EXPECT_FALSE(child_activator.activate_child(modules.get_parent(nephew).value(), nephew));
    EXPECT_NE(active_states.get(nephew).queries.at(1), at_root);
}

TEST_F(PudLeafActivationIntegrationTest, FactBelowClosesTheCallSiteWithoutForking) {
    EXPECT_FALSE(root_activator.activate_root(root));
    EXPECT_FALSE(child_activator.activate_child(modules.get_parent(child).value(), child));
    const pud_mhq_head_id before = active_states.get(child).queries.at(1);
    EXPECT_FALSE(child_activator.activate_child(modules.get_parent(grandchild).value(), grandchild));
    EXPECT_EQ(active_states.get(grandchild).queries.at(1), before);
}

TEST_F(PudLeafActivationIntegrationTest, UnitForkSurfacesInTheUnitQueue) {
    EXPECT_FALSE(root_activator.activate_root(root));
    unit_queries.clear();
    EXPECT_FALSE(child_activator.activate_child(modules.get_parent(sibling).value(), sibling));
    EXPECT_THAT(unit_queries, ElementsAre(active_states.get(sibling).queries.at(1)));
}

TEST_F(PudLeafActivationIntegrationTest, ReleasingALeafRemovesQueriesOnlyItHeld) {
    EXPECT_FALSE(root_activator.activate_root(root));
    EXPECT_FALSE(child_activator.activate_child(modules.get_parent(child).value(), child));
    ASSERT_NE(active_states.get(child).queries.find(2), nullptr);
    const pud_mhq_head_id child_only = active_states.get(child).queries.at(2);
    const pud_mhq_head_id shared = active_states.get(child).queries.at(1);
    node_deactivator.deactivate(child);
    EXPECT_FALSE(query_holders.find_holder(child_only).has_value());
    EXPECT_TRUE(query_holders.find_holder(shared).has_value());
    node_deactivator.deactivate(root);
    EXPECT_FALSE(query_holders.find_holder(shared).has_value());
}

TEST_F(PudLeafActivationIntegrationTest, RefutedOpenSurfacesThroughActivation) {
    module_data.emplace_back(
        k_lone_module,
        std::vector<pud_module_id>{},
        std::vector<rule>{rule(atom("lone"), {p0}, 0), rule(atom("s"), {}, 0)});
    modules.register_module(module_data.back());
    const pud_global_node_id lone = modules.list_axioms(k_lone_module).front();
    EXPECT_TRUE(root_activator.activate_root(lone));
}

TEST_F(PudLeafActivationIntegrationTest, ActivationRecordsEachNodesGoalsFromItsParents) {
    EXPECT_FALSE(root_activator.activate_root(root));
    EXPECT_FALSE(child_activator.activate_child(modules.get_parent(child).value(), child));
    EXPECT_FALSE(child_activator.activate_child(modules.get_parent(nephew).value(), nephew));
    const pud_goal_list root_goals = initial_goal_opener.open_goals({p0, q12});
    const pud_goal_list child_goals = goal_resolver.resolve_goal(root_goals, 0, {r3});
    EXPECT_EQ(node_goals.get(root), root_goals);
    EXPECT_EQ(node_goals.get(child), child_goals);
    EXPECT_EQ(node_goals.get(nephew), goal_resolver.resolve_goal(child_goals, 2, {s}));
}

TEST_F(PudLeafActivationIntegrationTest, ActivationGivesEachNodeItsParentsLvcPlusItsOwnVariables) {
    EXPECT_FALSE(root_activator.activate_root(root));
    EXPECT_FALSE(child_activator.activate_child(modules.get_parent(child).value(), child));
    EXPECT_FALSE(child_activator.activate_child(modules.get_parent(grandchild).value(), grandchild));
    EXPECT_EQ(active_states.get(root).lvc, 3u);
    EXPECT_EQ(active_states.get(child).lvc, 4u);
    EXPECT_EQ(active_states.get(grandchild).lvc, 4u);
}

TEST_F(PudLeafActivationIntegrationTest, ADeactivatedNodesGoalsAreKept) {
    EXPECT_FALSE(root_activator.activate_root(root));
    EXPECT_FALSE(child_activator.activate_child(modules.get_parent(child).value(), child));
    node_deactivator.deactivate(root);
    EXPECT_EQ(node_goals.get(root), initial_goal_opener.open_goals({p0, q12}));
}

TEST_F(PudLeafActivationIntegrationTest, EveryActivatedChildCountsAsALiveChildOfItsParent) {
    EXPECT_FALSE(root_activator.activate_root(root));
    EXPECT_FALSE(child_activator.activate_child(modules.get_parent(child).value(), child));
    EXPECT_FALSE(child_activator.activate_child(modules.get_parent(sibling).value(), sibling));
    EXPECT_EQ(live_child_counts.decrement(root), 1u);
    EXPECT_EQ(live_child_counts.decrement(root), 0u);
}
