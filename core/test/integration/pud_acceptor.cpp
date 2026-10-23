#include <set>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "functor_fixture.hpp"
#include "infrastructure/pud_manifest.hpp"
#include "infrastructure/ubs_bind_map.hpp"

using ::testing::ElementsAre;
using ::testing::IsEmpty;

// The acceptor over real node stores and the real replayer and term machinery. Nodes are written
// straight into the stores, so a test states exactly what each node adds.
struct PudAcceptorIntegrationTest : public ::testing::Test {
    test_functors functors;
    pud_manifest manifest;
    pud_manifest::acceptor_t& acceptor = manifest.acceptor_;
    pud_ubs_state empty_state;
    std::set<pud_node_id> expanded;

    const expr* var(uint32_t index) { return manifest.expr_pool_.make_var(index); }
    const expr* functor(const char* name, std::vector<const expr*> args) {
        return manifest.expr_pool_.make_functor(functors.id(name), args);
    }
    const expr* atom(const char* name) { return functor(name, {}); }

    pud_node_id root(const expr* head, std::vector<const expr*> goals) {
        return manifest.node_maker_.make_root(head, goals, 0);
    }

    pud_node_id child(
        pud_node_id parent,
        size_t parent_call_site,
        std::vector<pud_substitution> child_substitutions,
        std::vector<const expr*> goals) {
        if (expanded.insert(parent).second)
            manifest.call_sites_.store(parent, parent_call_site);
        return manifest.node_maker_.make_child(
            parent, parent_call_site, pud_child_delta{.substitutions = child_substitutions, .body_goals = goals, .var_count = 0});
    }

    pud_ubs_state bound(uint32_t index, const expr* value) {
        ubs_bind_map bind_map{empty_state};
        bind_map.instantiate(bind_map.find(index), framed_expr{value, 0});
        return std::move(bind_map).persist();
    }
};

TEST_F(PudAcceptorIntegrationTest, FactCandidateInstantiatesTheGoalsVariable) {
    const pud_node_id fact = root(functor("color", {atom("red")}), {});
    const pud_child_delta delta = acceptor.accept(empty_state, 1, functor("color", {var(0)}), {fact});
    EXPECT_THAT(delta.substitutions, ElementsAre(pud_substitution{.var_idx = 0, .value = atom("red")}));
    EXPECT_THAT(delta.body_goals, IsEmpty());
    EXPECT_EQ(delta.var_count, 0u);
}

TEST_F(PudAcceptorIntegrationTest, GroundGoalAgainstAMatchingFactNeedsNoSubstitution) {
    const pud_node_id fact = root(functor("color", {atom("red")}), {});
    const pud_child_delta delta = acceptor.accept(empty_state, 0, functor("color", {atom("red")}), {fact});
    EXPECT_THAT(delta.substitutions, IsEmpty());
    EXPECT_THAT(delta.body_goals, IsEmpty());
    EXPECT_EQ(delta.var_count, 0u);
}

TEST_F(PudAcceptorIntegrationTest, RuleCandidateAddsItsBodyOverFreshVariables) {
    const pud_node_id rule_root = root(functor("nat", {functor("s", {var(0)})}), {functor("nat", {var(0)})});
    const pud_child_delta delta = acceptor.accept(empty_state, 1, functor("nat", {var(0)}), {rule_root});
    EXPECT_THAT(delta.substitutions, ElementsAre(pud_substitution{.var_idx = 0, .value = functor("s", {var(1)})}));
    EXPECT_THAT(delta.body_goals, ElementsAre(functor("nat", {var(1)})));
    EXPECT_EQ(delta.var_count, 1u);
}

TEST_F(PudAcceptorIntegrationTest, FreshVariablesAreNumberedFromTheAcceptorsVariableCount) {
    const pud_node_id rule_root = root(
        functor("p", {functor("f", {var(0), var(1)})}),
        {functor("q", {var(1), var(0)})});
    const pud_child_delta delta = acceptor.accept(empty_state, 5, functor("p", {var(0)}), {rule_root});
    EXPECT_THAT(delta.substitutions, ElementsAre(pud_substitution{.var_idx = 0, .value = functor("f", {var(5), var(6)})}));
    EXPECT_THAT(delta.body_goals, ElementsAre(functor("q", {var(6), var(5)})));
    EXPECT_EQ(delta.var_count, 2u);
}

TEST_F(PudAcceptorIntegrationTest, CandidateVariableBoundToTheGoalIsSubstitutedAway) {
    const pud_node_id rule_root = root(functor("p", {var(0)}), {functor("q", {var(0)})});
    const pud_child_delta delta = acceptor.accept(empty_state, 0, functor("p", {atom("a")}), {rule_root});
    EXPECT_THAT(delta.substitutions, IsEmpty());
    EXPECT_THAT(delta.body_goals, ElementsAre(functor("q", {atom("a")})));
    EXPECT_EQ(delta.var_count, 0u);
}

TEST_F(PudAcceptorIntegrationTest, AcceptorVariablesMergedByTheCandidateBecomeAVariableSubstitution) {
    const pud_node_id same = root(functor("eq", {var(0), var(0)}), {});
    const pud_child_delta delta = acceptor.accept(empty_state, 2, functor("eq", {var(0), var(1)}), {same});
    EXPECT_THAT(delta.substitutions, ElementsAre(pud_substitution{.var_idx = 1, .value = var(0)}));
    EXPECT_EQ(delta.var_count, 0u);
}

TEST_F(PudAcceptorIntegrationTest, AcceptorsBindingsAreRespected) {
    const pud_node_id same = root(functor("eq", {var(0), var(0)}), {});
    const pud_child_delta delta = acceptor.accept(
        bound(0, atom("a")), 2, functor("eq", {var(0), var(1)}), {same});
    EXPECT_THAT(delta.substitutions, ElementsAre(pud_substitution{.var_idx = 1, .value = var(0)}));
}

TEST_F(PudAcceptorIntegrationTest, UntouchedAcceptorVariablesAreNotSubstituted) {
    const pud_node_id fact = root(functor("p", {atom("a")}), {});
    const pud_child_delta delta = acceptor.accept(empty_state, 3, functor("p", {var(2)}), {fact});
    EXPECT_THAT(delta.substitutions, ElementsAre(pud_substitution{.var_idx = 2, .value = atom("a")}));
}

TEST_F(PudAcceptorIntegrationTest, CandidateDeepInATreeReplaysItsWholePath) {
    const pud_node_id rule_root = root(functor("nat", {functor("s", {var(0)})}), {functor("nat", {var(0)})});
    const pud_node_id deeper = child(
        rule_root, 0, {pud_substitution{.var_idx = 0, .value = functor("s", {var(1)})}}, {functor("nat", {var(1)})});
    const pud_child_delta delta = acceptor.accept(empty_state, 1, functor("nat", {var(0)}), {rule_root, deeper});
    EXPECT_THAT(
        delta.substitutions,
        ElementsAre(pud_substitution{.var_idx = 0, .value = functor("s", {functor("s", {var(1)})})}));
    EXPECT_THAT(delta.body_goals, ElementsAre(functor("nat", {var(1)})));
    EXPECT_EQ(delta.var_count, 1u);
}

TEST_F(PudAcceptorIntegrationTest, CandidatesClosedCallSiteIsNotAddedAsABodyGoal) {
    const pud_node_id rule_root = root(
        functor("p", {var(0)}),
        {functor("q", {var(0)}), functor("r", {var(0)})});
    const pud_node_id deeper = child(rule_root, 0, {pud_substitution{.var_idx = 0, .value = atom("a")}}, {});
    const pud_child_delta delta = acceptor.accept(empty_state, 0, functor("p", {atom("a")}), {rule_root, deeper});
    EXPECT_THAT(delta.body_goals, ElementsAre(functor("r", {atom("a")})));
}

TEST_F(PudAcceptorIntegrationTest, SharedFreshVariablesAreTranslatedConsistently) {
    const pud_node_id rule_root = root(
        functor("p", {var(0), var(1)}),
        {functor("q", {var(1)}), functor("r", {var(1), var(0)})});
    const pud_child_delta delta = acceptor.accept(
        empty_state, 1, functor("p", {var(0), functor("g", {var(0)})}), {rule_root});
    EXPECT_THAT(delta.substitutions, IsEmpty());
    EXPECT_THAT(
        delta.body_goals,
        ElementsAre(functor("q", {functor("g", {var(0)})}), functor("r", {functor("g", {var(0)}), var(0)})));
    EXPECT_EQ(delta.var_count, 0u);
}

TEST_F(PudAcceptorIntegrationTest, CallerVarsAreExportedWithoutBeingLookedUp) {
    const pud_node_id rule = root(functor("q", {var(0)}), {functor("r", {var(0)})});
    const pud_child_delta delta = acceptor.accept(
        bound(0, functor("s", {var(1)})), 2, functor("q", {var(0)}), {rule});
    EXPECT_THAT(delta.substitutions, IsEmpty());
    EXPECT_THAT(delta.body_goals, ElementsAre(functor("r", {var(0)})));
    EXPECT_EQ(delta.var_count, 0u);
}
