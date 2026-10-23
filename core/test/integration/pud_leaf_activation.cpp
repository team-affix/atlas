#include <deque>
#include <optional>
#include <vector>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "functor_fixture.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/pud_binder.hpp"
#include "infrastructure/pud_child_activator.hpp"
#include "infrastructure/pud_node_deactivator.hpp"
#include "infrastructure/pud_root_activator.hpp"
#include "infrastructure/pud_axiom_groups.hpp"
#include "infrastructure/pud_data.hpp"
#include "infrastructure/pud_mhcs.hpp"
#include "infrastructure/pud_mhq.hpp"
#include "infrastructure/pud_mhws.hpp"
#include "infrastructure/pud_node_goals.hpp"
#include "infrastructure/pud_node_live_child_counts.hpp"
#include "infrastructure/pud_refuted_leaves.hpp"
#include "infrastructure/pud_refuted_node_checker.hpp"
#include "infrastructure/pud_node_table.hpp"
#include "infrastructure/pud_active_states.hpp"
#include "infrastructure/pud_query_holders.hpp"
#include "infrastructure/pud_replayer.hpp"
#include "infrastructure/pud_structure_watcher.hpp"
#include "infrastructure/pud_query_forker.hpp"
#include "infrastructure/pud_query_opener.hpp"
#include "infrastructure/pud_term_factory.hpp"
#include <immer/map.hpp>

using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::UnorderedElementsAre;

// Leaf activation over the real binder, watcher, query opener, search stack, and explicit query holders.
struct PudLeafActivationIntegrationTest : public ::testing::Test {
    using term_factory_t = pud_term_factory<globalizer, expr_pool, expr_pool>;
    using lvcs_t = pud_node_table<uint32_t>;
    using unit_queries_t = std::deque<pud_mhq_head_id>;
    using refuted_node_checker_t =
        pud_refuted_node_checker<pud_data, pud_refuted_leaves, pud_node_live_child_counts>;
    using replayer_t = pud_replayer<pud_data, pud_data, expr_pool, term_factory_t, term_factory_t, term_factory_t>;
    using mhws_t = pud_mhws<replayer_t, pud_data, refuted_node_checker_t, pud_data>;
    using mhcs_t =
        pud_mhcs<replayer_t, mhws_t, mhws_t, mhws_t, mhws_t, mhws_t, pud_data, refuted_node_checker_t, pud_data>;
    using mhq_t = pud_mhq<replayer_t, pud_axiom_groups, mhcs_t, mhcs_t, mhcs_t, mhcs_t, unit_queries_t>;
    using binder_t = pud_binder<term_factory_t, pud_data>;
    using watcher_t = pud_structure_watcher<term_factory_t, pud_data>;
    using node_axiom_groups_t = pud_node_table<pud_axiom_group_id>;
    using query_opener_t = pud_query_opener<mhq_t, pud_node_goals, lvcs_t, node_axiom_groups_t>;
    using query_forker_t = pud_query_forker<mhq_t, pud_node_goals, lvcs_t>;
    using root_activator_t =
        pud_root_activator<pud_active_states, watcher_t, pud_node_goals, query_opener_t, pud_query_holders>;
    using child_activator_t = pud_child_activator<
        pud_active_states, pud_active_states, binder_t, watcher_t, pud_node_goals, pud_data, query_forker_t,
        query_opener_t, pud_query_holders, mhq_t>;
    using node_deactivator_t =
        pud_node_deactivator<pud_active_states, pud_active_states, pud_query_holders, mhq_t, mhq_t, pud_query_holders>;

    test_functors functors;
    expr_pool exprs;
    globalizer globalize;
    term_factory_t terms{globalize, exprs, exprs};
    pud_data data;
    pud_refuted_leaves refuted_leaves;
    pud_node_live_child_counts live_child_counts;
    lvcs_t lvcs;
    pud_node_goals node_goals;
    pud_axiom_groups axiom_groups;
    node_axiom_groups_t node_axiom_groups;
    pud_axiom_group_id database_group;
    unit_queries_t unit_queries;
    pud_active_states active_states;
    pud_query_holders query_holders;
    refuted_node_checker_t refuted_node_checker{data, refuted_leaves, live_child_counts};
    replayer_t replayer{data, data, exprs, terms, terms, terms};
    mhws_t mhws{replayer, data, refuted_node_checker, data};
    mhcs_t mhcs{replayer, mhws, mhws, mhws, mhws, mhws, data, refuted_node_checker, data};
    mhq_t mhq{replayer, axiom_groups, mhcs, mhcs, mhcs, mhcs, unit_queries};

    PudLeafActivationIntegrationTest()
        : database_group(axiom_groups.create_group()) {}
    binder_t binder{terms, data};
    watcher_t structure_watcher{terms, data};
    query_opener_t query_opener{mhq, node_goals, lvcs, node_axiom_groups};
    query_forker_t query_forker{mhq, node_goals, lvcs};
    root_activator_t root_activator{active_states, structure_watcher, node_goals, query_opener, query_holders};
    child_activator_t child_activator{
        active_states, active_states, binder, structure_watcher, node_goals, data, query_forker, query_opener,
        query_holders, mhq};
    node_deactivator_t node_deactivator{active_states, active_states, query_holders, mhq, mhq, query_holders};

    pud_node_id root = 0;
    pud_node_id child = 0;
    pud_node_id grandchild = 0;
    pud_node_id nephew = 0;
    pud_node_id great_nephew = 0;
    pud_node_id sibling = 0;
    pud_node_id linker = 0;
    pud_node_id linked = 0;

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

    void tag_group(pud_node_id node) { node_axiom_groups.store(node, database_group); }

    pud_node_id make_axiom(const expr* head) {
        const pud_node_id node = data.store_axiom(rule(head, {}, 0));
        axiom_groups.add_member(database_group, node);
        return node;
    }

    pud_node_id infer(
        pud_node_id parent,
        pud_body_goal_id call_site,
        uint32_t lvc,
        pud_goal_list goals,
        std::vector<pud_substitution> substitutions) {
        const pud_node_id node = data.store_inference(pud_node_delta{
            .parent = parent,
            .call_site = call_site,
            .substitutions = std::move(substitutions),
            .body_goals = {},
            .var_count = 0,
        });
        node_goals.store(node, std::move(goals));
        lvcs.store(node, lvc);
        tag_group(node);
        return node;
    }

    void activate(pud_node_id node) {
        if (node == root)
            EXPECT_FALSE(root_activator.activate_root(root));
        else
            EXPECT_FALSE(child_activator.activate_child(data.get_parent(node).value(), node));
    }

    void release(pud_node_id node) {
        node_deactivator.deactivate(node);
    }

    pud_mhq_head_id query_on(pud_node_id leaf, pud_body_goal_id goal) {
        return active_states.get(leaf).queries.at(goal);
    }

    void SetUp() override {
        p0 = functor("p", {var(0)});
        q12 = functor("q", {var(1), var(2)});
        r3 = functor("r", {var(3)});
        r1 = functor("r", {var(1)});
        s = atom("s");
        const pud_goal_list root_goals = pud_goal_list{.goals = {}, .next_id = 0}.adding(p0).adding(q12);
        root = data.store_axiom(rule(p0, {p0, q12}, 3));
        node_goals.store(root, root_goals);
        lvcs.store(root, 3);
        tag_group(root);
        const pud_goal_list child_goals = root_goals.closing(0).adding(r3);
        child = infer(root, 0, 4, child_goals, {pud_substitution{.var_idx = 0, .value = atom("a")}});
        grandchild = infer(child, 2, 4, child_goals.closing(2), {pud_substitution{.var_idx = 3, .value = atom("b")}});
        const pud_goal_list nephew_goals = child_goals.closing(2).adding(s);
        nephew = infer(child, 2, 4, nephew_goals, {pud_substitution{.var_idx = 2, .value = functor("f", {var(3)})}});
        great_nephew = infer(nephew, 3, 4, nephew_goals.closing(3), {pud_substitution{.var_idx = 1, .value = atom("c")}});
        sibling = infer(root, 0, 3, root_goals.closing(0), {pud_substitution{.var_idx = 1, .value = var(2)}});
        const pud_goal_list linker_goals = root_goals.closing(0).adding(r1);
        linker = infer(root, 0, 3, linker_goals, {});
        linked = infer(linker, 2, 3, linker_goals.closing(2), {pud_substitution{.var_idx = 1, .value = atom("a")}});
        make_axiom(functor("p", {var(0)}));
        make_axiom(functor("q", {var(1), var(2)}));
        make_axiom(functor("r", {var(0)}));
        make_axiom(functor("r", {var(1)}));
        make_axiom(atom("s"));
    }
};

TEST_F(PudLeafActivationIntegrationTest, RootAdmitsAQueryPerOpenGoal) {
    activate(root);
    const immer::map<pud_body_goal_id, pud_mhq_head_id>& queries = active_states.get(root).queries;
    EXPECT_NE(queries.find(0), nullptr);
    EXPECT_NE(queries.find(1), nullptr);
    EXPECT_EQ(queries.size(), 2u);
    EXPECT_EQ(query_holders.get_holders(query_on(root, 0)).size(), 1u);
    EXPECT_EQ(query_holders.get_holders(query_on(root, 1)).size(), 1u);
}

TEST_F(PudLeafActivationIntegrationTest, ChildAdmitsAQueryForEachNewGoal) {
    activate(root);
    activate(child);
    EXPECT_TRUE(active_states.get(child).queries.find(2) != nullptr);
    EXPECT_EQ(query_holders.get_holders(query_on(child, 2)).front(), child);
}

TEST_F(PudLeafActivationIntegrationTest, FactChildWithNoGoalsOpensNothingNew) {
    activate(root);
    activate(sibling);
    EXPECT_EQ(active_states.get(sibling).queries.find(2), nullptr);
}

TEST_F(PudLeafActivationIntegrationTest, BindingTheClosedGoalDoesNotForkItsQuery) {
    activate(root);
    const pud_mhq_head_id before = query_on(root, 1);
    activate(child);
    EXPECT_EQ(query_on(child, 1), before);
}

TEST_F(PudLeafActivationIntegrationTest, CollapseForksTheAffectedGoal) {
    activate(root);
    const pud_mhq_head_id before = query_on(root, 1);
    activate(sibling);
    EXPECT_NE(query_on(sibling, 1), before);
}

TEST_F(PudLeafActivationIntegrationTest, InstantiationForksTheAffectedGoal) {
    activate(root);
    activate(child);
    const pud_mhq_head_id before = query_on(child, 1);
    activate(nephew);
    EXPECT_NE(query_on(nephew, 1), before);
}

TEST_F(PudLeafActivationIntegrationTest, ForkedQueryShadowsTheParentsQuery) {
    activate(root);
    activate(child);
    const pud_mhq_head_id at_child = query_on(child, 1);
    activate(nephew);
    const pud_mhq_head_id at_nephew = query_on(nephew, 1);
    EXPECT_NE(at_nephew, at_child);
    activate(great_nephew);
    EXPECT_NE(query_on(great_nephew, 1), at_nephew);
}

TEST_F(PudLeafActivationIntegrationTest, SiblingsForkFromTheSameRootQuery) {
    activate(root);
    const pud_mhq_head_id at_root = query_on(root, 1);
    activate(child);
    activate(sibling);
    const pud_mhq_head_id at_sibling = query_on(sibling, 1);
    EXPECT_NE(at_sibling, at_root);
    activate(nephew);
    EXPECT_NE(query_on(nephew, 1), at_root);
}

TEST_F(PudLeafActivationIntegrationTest, FactBelowClosesTheCallSiteWithoutForking) {
    activate(root);
    activate(child);
    const pud_mhq_head_id before = query_on(child, 1);
    activate(grandchild);
    EXPECT_EQ(query_on(grandchild, 1), before);
}

TEST_F(PudLeafActivationIntegrationTest, UnitForkSurfacesInTheUnitQueue) {
    activate(root);
    unit_queries.clear();
    activate(sibling);
    EXPECT_THAT(unit_queries, ElementsAre(query_on(sibling, 1)));
}

TEST_F(PudLeafActivationIntegrationTest, ReleasingALeafRemovesQueriesOnlyItHeld) {
    activate(root);
    activate(child);
    ASSERT_NE(active_states.get(child).queries.find(2), nullptr);
    const pud_mhq_head_id child_only = query_on(child, 2);
    const pud_mhq_head_id shared = query_on(child, 1);
    release(child);
    EXPECT_FALSE(query_holders.find_holder(child_only).has_value());
    EXPECT_TRUE(query_holders.find_holder(shared).has_value());
    release(root);
    EXPECT_FALSE(query_holders.find_holder(shared).has_value());
}

TEST_F(PudLeafActivationIntegrationTest, RefutedOpenSurfacesThroughActivation) {
    const pud_axiom_group_id isolated = axiom_groups.create_group();
    const pud_node_id lone = data.store_axiom(rule(p0, {p0}, 0));
    node_goals.store(lone, pud_goal_list{.goals = {}, .next_id = 0}.adding(p0));
    lvcs.store(lone, 0);
    node_axiom_groups.store(lone, isolated);
    const pud_node_id s_axiom = data.store_axiom(rule(atom("s"), {}, 0));
    axiom_groups.add_member(isolated, s_axiom);
    EXPECT_TRUE(root_activator.activate_root(lone));
}
