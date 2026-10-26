#include <deque>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "functor_fixture.hpp"
#include "pud_test_node.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/pud_module.hpp"
#include "infrastructure/pud_manifest.hpp"
#include "value_objects/rule.hpp"
#include "infrastructure/ubs_bind_map.hpp"

using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::UnorderedElementsAre;

// The acceptor over real node stores and the real replayer and term machinery. Every root is the one
// axiom of its own module and its children are stored straight into that module, so a test states
// exactly what each node adds. Every node is activated as it is made, which records its goals.
struct PudAcceptorIntegrationTest : public ::testing::Test {
    using manifest_t = pud_manifest<pud_module, expr_pool>;

    test_functors functors;
    expr_pool exprs;
    std::deque<pud_module> module_data;
    manifest_t manifest{exprs};
    manifest_t::acceptor_t& acceptor = manifest.acceptor_;
    pud_ubs_state empty_state;
    pud_global_node_id caller = pud_test_node(99);
    pud_body_goal_id call_site = 0;

    const expr* var(uint32_t index) { return exprs.make_var(index); }
    const expr* functor(const char* name, std::vector<const expr*> args) {
        return exprs.make_functor(functors.id(name), args);
    }
    const expr* atom(const char* name) { return functor(name, {}); }
};

TEST_F(PudAcceptorIntegrationTest, FactCandidateInstantiatesTheGoalsVariable) {
    const pud_module_id module = module_data.size();
    module_data.emplace_back(
        module,
        std::vector<pud_module_id>{},
        std::vector<rule>{rule(functor("color", {atom("red")}), {}, 0)});
    manifest.modules_.register_module(module_data.back());
    const pud_global_node_id fact = manifest.modules_.list_axioms(module).front();
    manifest.root_activator_.activate_root(fact);
    const pud_inference delta = acceptor.accept(caller, call_site, empty_state, 1, functor("color", {var(0)}), {fact});
    EXPECT_THAT(delta.substitutions, ElementsAre(pud_substitution{.var_idx = 0, .value = atom("red")}));
    EXPECT_THAT(delta.body_goals, IsEmpty());
    EXPECT_EQ(delta.var_count, 0u);
}

TEST_F(PudAcceptorIntegrationTest, GroundGoalAgainstAMatchingFactNeedsNoSubstitution) {
    const pud_module_id module = module_data.size();
    module_data.emplace_back(
        module,
        std::vector<pud_module_id>{},
        std::vector<rule>{rule(functor("color", {atom("red")}), {}, 0)});
    manifest.modules_.register_module(module_data.back());
    const pud_global_node_id fact = manifest.modules_.list_axioms(module).front();
    manifest.root_activator_.activate_root(fact);
    const pud_inference delta = acceptor.accept(caller, call_site, empty_state, 0, functor("color", {atom("red")}), {fact});
    EXPECT_THAT(delta.substitutions, IsEmpty());
    EXPECT_THAT(delta.body_goals, IsEmpty());
    EXPECT_EQ(delta.var_count, 0u);
}

TEST_F(PudAcceptorIntegrationTest, RuleCandidateAddsItsBodyOverFreshVariables) {
    const pud_module_id module = module_data.size();
    module_data.emplace_back(
        module,
        std::vector<pud_module_id>{},
        std::vector<rule>{rule(functor("nat", {functor("s", {var(0)})}), {functor("nat", {var(0)})}, 0)});
    manifest.modules_.register_module(module_data.back());
    const pud_global_node_id rule_root = manifest.modules_.list_axioms(module).front();
    manifest.root_activator_.activate_root(rule_root);
    const pud_inference delta = acceptor.accept(caller, call_site, empty_state, 1, functor("nat", {var(0)}), {rule_root});
    EXPECT_THAT(delta.substitutions, ElementsAre(pud_substitution{.var_idx = 0, .value = functor("s", {var(1)})}));
    EXPECT_THAT(delta.body_goals, ElementsAre(functor("nat", {var(1)})));
    EXPECT_EQ(delta.var_count, 1u);
}

TEST_F(PudAcceptorIntegrationTest, FreshVariablesAreNumberedFromTheAcceptorsVariableCount) {
    const pud_module_id module = module_data.size();
    module_data.emplace_back(
        module,
        std::vector<pud_module_id>{},
        std::vector<rule>{rule(
            functor("p", {functor("f", {var(0), var(1)})}),
            {functor("q", {var(1), var(0)})},
            0)});
    manifest.modules_.register_module(module_data.back());
    const pud_global_node_id rule_root = manifest.modules_.list_axioms(module).front();
    manifest.root_activator_.activate_root(rule_root);
    const pud_inference delta = acceptor.accept(caller, call_site, empty_state, 5, functor("p", {var(0)}), {rule_root});
    EXPECT_THAT(delta.substitutions, ElementsAre(pud_substitution{.var_idx = 0, .value = functor("f", {var(5), var(6)})}));
    EXPECT_THAT(delta.body_goals, ElementsAre(functor("q", {var(6), var(5)})));
    EXPECT_EQ(delta.var_count, 2u);
}

TEST_F(PudAcceptorIntegrationTest, CandidateVariableBoundToTheGoalIsSubstitutedAway) {
    const pud_module_id module = module_data.size();
    module_data.emplace_back(
        module,
        std::vector<pud_module_id>{},
        std::vector<rule>{rule(functor("p", {var(0)}), {functor("q", {var(0)})}, 0)});
    manifest.modules_.register_module(module_data.back());
    const pud_global_node_id rule_root = manifest.modules_.list_axioms(module).front();
    manifest.root_activator_.activate_root(rule_root);
    const pud_inference delta = acceptor.accept(caller, call_site, empty_state, 0, functor("p", {atom("a")}), {rule_root});
    EXPECT_THAT(delta.substitutions, IsEmpty());
    EXPECT_THAT(delta.body_goals, ElementsAre(functor("q", {atom("a")})));
    EXPECT_EQ(delta.var_count, 0u);
}

TEST_F(PudAcceptorIntegrationTest, AcceptorVariablesMergedByTheCandidateBecomeAVariableSubstitution) {
    const pud_module_id module = module_data.size();
    module_data.emplace_back(
        module,
        std::vector<pud_module_id>{},
        std::vector<rule>{rule(functor("eq", {var(0), var(0)}), {}, 0)});
    manifest.modules_.register_module(module_data.back());
    const pud_global_node_id same = manifest.modules_.list_axioms(module).front();
    manifest.root_activator_.activate_root(same);
    const pud_inference delta = acceptor.accept(caller, call_site, empty_state, 2, functor("eq", {var(0), var(1)}), {same});
    EXPECT_THAT(delta.substitutions, ElementsAre(pud_substitution{.var_idx = 1, .value = var(0)}));
    EXPECT_EQ(delta.var_count, 0u);
}

TEST_F(PudAcceptorIntegrationTest, AcceptorsBindingsAreRespected) {
    const pud_module_id module = module_data.size();
    module_data.emplace_back(
        module,
        std::vector<pud_module_id>{},
        std::vector<rule>{rule(functor("eq", {var(0), var(0)}), {}, 0)});
    manifest.modules_.register_module(module_data.back());
    const pud_global_node_id same = manifest.modules_.list_axioms(module).front();
    manifest.root_activator_.activate_root(same);
    ubs_bind_map bind_map{empty_state};
    bind_map.instantiate(bind_map.find(0), framed_functor_from_framed_expr(framed_expr{atom("a"), 0}));
    const pud_ubs_state caller_state = std::move(bind_map).persist();
    const pud_inference delta = acceptor.accept(
        caller, call_site, caller_state, 2, functor("eq", {var(0), var(1)}), {same});
    EXPECT_THAT(delta.substitutions, ElementsAre(pud_substitution{.var_idx = 1, .value = var(0)}));
}

TEST_F(PudAcceptorIntegrationTest, UntouchedAcceptorVariablesAreNotSubstituted) {
    const pud_module_id module = module_data.size();
    module_data.emplace_back(
        module,
        std::vector<pud_module_id>{},
        std::vector<rule>{rule(functor("p", {atom("a")}), {}, 0)});
    manifest.modules_.register_module(module_data.back());
    const pud_global_node_id fact = manifest.modules_.list_axioms(module).front();
    manifest.root_activator_.activate_root(fact);
    const pud_inference delta = acceptor.accept(caller, call_site, empty_state, 3, functor("p", {var(2)}), {fact});
    EXPECT_THAT(delta.substitutions, ElementsAre(pud_substitution{.var_idx = 2, .value = atom("a")}));
}

TEST_F(PudAcceptorIntegrationTest, CandidateDeepInATreeReplaysItsWholePath) {
    const pud_module_id module = module_data.size();
    module_data.emplace_back(
        module,
        std::vector<pud_module_id>{},
        std::vector<rule>{rule(functor("nat", {functor("s", {var(0)})}), {functor("nat", {var(0)})}, 0)});
    manifest.modules_.register_module(module_data.back());
    const pud_global_node_id rule_root = manifest.modules_.list_axioms(module).front();
    manifest.root_activator_.activate_root(rule_root);
    const pud_global_node_id deeper = manifest.modules_.store_inference(rule_root.module, pud_inference{
        .parent = rule_root.node,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 0, .value = functor("s", {var(1)})}},
        .body_goals = {functor("nat", {var(1)})},
        .var_count = 0,
    });
    manifest.child_activator_.activate_child(rule_root, deeper);
    const pud_inference delta = acceptor.accept(caller, call_site, empty_state, 1, functor("nat", {var(0)}), {rule_root, deeper});
    EXPECT_THAT(
        delta.substitutions,
        ElementsAre(pud_substitution{.var_idx = 0, .value = functor("s", {functor("s", {var(1)})})}));
    EXPECT_THAT(delta.body_goals, ElementsAre(functor("nat", {var(1)})));
    EXPECT_EQ(delta.var_count, 1u);
}

TEST_F(PudAcceptorIntegrationTest, CandidatesClosedCallSiteIsNotAddedAsABodyGoal) {
    const pud_module_id module = module_data.size();
    module_data.emplace_back(
        module,
        std::vector<pud_module_id>{},
        std::vector<rule>{rule(
            functor("p", {var(0)}),
            {functor("q", {var(0)}), functor("r", {var(0)})},
            0)});
    manifest.modules_.register_module(module_data.back());
    const pud_global_node_id rule_root = manifest.modules_.list_axioms(module).front();
    manifest.root_activator_.activate_root(rule_root);
    const pud_global_node_id deeper = manifest.modules_.store_inference(rule_root.module, pud_inference{
        .parent = rule_root.node,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 0, .value = atom("a")}},
        .body_goals = {},
        .var_count = 0,
    });
    manifest.child_activator_.activate_child(rule_root, deeper);
    const pud_inference delta = acceptor.accept(caller, call_site, empty_state, 0, functor("p", {atom("a")}), {rule_root, deeper});
    EXPECT_THAT(delta.body_goals, ElementsAre(functor("r", {atom("a")})));
}

TEST_F(PudAcceptorIntegrationTest, SharedFreshVariablesAreTranslatedConsistently) {
    const pud_module_id module = module_data.size();
    module_data.emplace_back(
        module,
        std::vector<pud_module_id>{},
        std::vector<rule>{rule(
            functor("p", {var(0), var(1)}),
            {functor("q", {var(1)}), functor("r", {var(1), var(0)})},
            0)});
    manifest.modules_.register_module(module_data.back());
    const pud_global_node_id rule_root = manifest.modules_.list_axioms(module).front();
    manifest.root_activator_.activate_root(rule_root);
    const pud_inference delta = acceptor.accept(
        caller, call_site, empty_state, 1, functor("p", {var(0), functor("g", {var(0)})}), {rule_root});
    EXPECT_THAT(delta.substitutions, IsEmpty());
    EXPECT_THAT(
        delta.body_goals,
        UnorderedElementsAre(functor("q", {functor("g", {var(0)})}), functor("r", {functor("g", {var(0)}), var(0)})));
    EXPECT_EQ(delta.var_count, 0u);
}

TEST_F(PudAcceptorIntegrationTest, CallerVarsAreExportedWithoutBeingLookedUp) {
    const pud_module_id module = module_data.size();
    module_data.emplace_back(
        module,
        std::vector<pud_module_id>{},
        std::vector<rule>{rule(functor("q", {var(0)}), {functor("r", {var(0)})}, 0)});
    manifest.modules_.register_module(module_data.back());
    const pud_global_node_id rule = manifest.modules_.list_axioms(module).front();
    manifest.root_activator_.activate_root(rule);
    ubs_bind_map bind_map{empty_state};
    bind_map.instantiate(bind_map.find(0), framed_functor_from_framed_expr(framed_expr{functor("s", {var(1)}), 0}));
    const pud_ubs_state caller_state = std::move(bind_map).persist();
    const pud_inference delta = acceptor.accept(
        caller, call_site, caller_state, 2, functor("q", {var(0)}), {rule});
    EXPECT_THAT(delta.substitutions, IsEmpty());
    EXPECT_THAT(delta.body_goals, ElementsAre(functor("r", {var(0)})));
    EXPECT_EQ(delta.var_count, 0u);
}
