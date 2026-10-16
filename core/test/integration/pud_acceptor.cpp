#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "functor_fixture.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/pud_acceptor.hpp"
#include "infrastructure/pud_call_sites.hpp"
#include "infrastructure/pud_node_added_body_goals.hpp"
#include "infrastructure/pud_node_added_substitutions.hpp"
#include "infrastructure/pud_node_heads.hpp"
#include "infrastructure/pud_node_parents.hpp"
#include "infrastructure/pud_node_path_finder.hpp"
#include "infrastructure/pud_normalizer_factory.hpp"
#include "infrastructure/pud_open_body_goal_finder.hpp"
#include "infrastructure/pud_root_nodes.hpp"
#include "infrastructure/pud_transient_ubs_state.hpp"
#include "infrastructure/pud_translating_normalizer.hpp"
#include "infrastructure/pud_unifier.hpp"
#include "infrastructure/representative_change_logger.hpp"

using ::testing::ElementsAre;
using ::testing::IsEmpty;

struct PudAcceptorIntegrationTest : public ::testing::Test {
    using bind_map_t = ubs_transient_bind_map<expr_pool, expr_pool>;
    using normalizer_factory_t = pud_normalizer_factory<globalizer, expr_pool, bind_map_t>;
    using normalizer_t = normalizer_factory_t::normalizer_t;
    using logger_t = representative_change_logger<bind_map_t, bind_map_t, std::vector<uint32_t>>;
    using unifier_t = pud_unifier<globalizer, normalizer_t, bind_map_t, logger_t, logger_t>;
    using translating_normalizer_t = pud_translating_normalizer<globalizer, expr_pool, expr_pool, bind_map_t>;
    using path_finder_t = pud_node_path_finder<pud_root_nodes, pud_node_parents>;
    using open_goal_finder_t = pud_open_body_goal_finder<pud_node_added_body_goals, pud_call_sites>;
    using acceptor_t = pud_acceptor<
        bind_map_t, normalizer_t, unifier_t, logger_t, translating_normalizer_t, path_finder_t,
        open_goal_finder_t, pud_node_heads, pud_node_added_substitutions, globalizer, expr_pool,
        expr_pool, normalizer_factory_t>;

    test_functors functors;
    expr_pool exprs;
    globalizer globalize;
    pud_root_nodes roots;
    pud_node_parents parents;
    pud_node_heads heads;
    pud_node_added_body_goals body_goals;
    pud_node_added_substitutions substitutions;
    pud_call_sites call_sites;
    normalizer_factory_t normalizer_factory{globalize, exprs};
    path_finder_t path_finder{roots, parents};
    open_goal_finder_t open_goal_finder{body_goals, call_sites};
    acceptor_t acceptor{
        path_finder, open_goal_finder, heads, substitutions, globalize, exprs, exprs, normalizer_factory};
    pud_ubs_state empty_state;
    pud_node_id next_node = 1;

    const expr* var(uint32_t index) { return exprs.make_var(index); }
    const expr* functor(const char* name, std::vector<const expr*> args) {
        return exprs.make_functor(functors.id(name), args);
    }
    const expr* atom(const char* name) { return functor(name, {}); }

    pud_node_id root(const expr* head, std::vector<const expr*> goals) {
        const pud_node_id node = next_node++;
        roots.mark_root(node);
        heads.store(node, head);
        body_goals.store(node, std::move(goals));
        return node;
    }

    pud_node_id child(
        pud_node_id parent,
        size_t parent_call_site,
        std::vector<pud_substitution> child_substitutions,
        std::vector<const expr*> goals) {
        const pud_node_id node = next_node++;
        parents.store(node, parent);
        call_sites.store(parent, parent_call_site);
        substitutions.store(node, std::move(child_substitutions));
        body_goals.store(node, std::move(goals));
        return node;
    }

    pud_ubs_state bound(uint32_t index, const expr* value) {
        pud_transient_ubs_state transient_ubs_state{empty_state};
        auto bind_map = transient_ubs_state.make_bind_map(exprs, exprs);
        const expr::functor& value_functor = std::get<expr::functor>(value->content);
        bind_map.instantiate(index, value_functor.id, value_functor.args);
        return transient_ubs_state.to_persistent_state();
    }
};

TEST_F(PudAcceptorIntegrationTest, FactCandidateInstantiatesTheGoalsVariable) {
    const pud_node_id fact = root(functor("color", {atom("red")}), {});
    const pud_child_delta delta = acceptor.accept(empty_state, 1, functor("color", {var(0)}), fact);
    EXPECT_THAT(delta.substitutions, ElementsAre(pud_substitution{.var_idx = 0, .value = atom("red")}));
    EXPECT_THAT(delta.body_goals, IsEmpty());
    EXPECT_EQ(delta.var_count, 0u);
}

TEST_F(PudAcceptorIntegrationTest, GroundGoalAgainstAMatchingFactNeedsNoSubstitution) {
    const pud_node_id fact = root(functor("color", {atom("red")}), {});
    const pud_child_delta delta = acceptor.accept(empty_state, 0, functor("color", {atom("red")}), fact);
    EXPECT_THAT(delta.substitutions, IsEmpty());
    EXPECT_THAT(delta.body_goals, IsEmpty());
    EXPECT_EQ(delta.var_count, 0u);
}

TEST_F(PudAcceptorIntegrationTest, RuleCandidateAddsItsBodyOverFreshVariables) {
    const pud_node_id rule_root = root(functor("nat", {functor("s", {var(0)})}), {functor("nat", {var(0)})});
    const pud_child_delta delta = acceptor.accept(empty_state, 1, functor("nat", {var(0)}), rule_root);
    EXPECT_THAT(delta.substitutions, ElementsAre(pud_substitution{.var_idx = 0, .value = functor("s", {var(1)})}));
    EXPECT_THAT(delta.body_goals, ElementsAre(functor("nat", {var(1)})));
    EXPECT_EQ(delta.var_count, 1u);
}

TEST_F(PudAcceptorIntegrationTest, FreshVariablesAreNumberedFromTheAcceptorsVariableCount) {
    const pud_node_id rule_root = root(
        functor("p", {functor("f", {var(0), var(1)})}),
        {functor("q", {var(1), var(0)})});
    const pud_child_delta delta = acceptor.accept(empty_state, 5, functor("p", {var(0)}), rule_root);
    EXPECT_THAT(delta.substitutions, ElementsAre(pud_substitution{.var_idx = 0, .value = functor("f", {var(5), var(6)})}));
    EXPECT_THAT(delta.body_goals, ElementsAre(functor("q", {var(6), var(5)})));
    EXPECT_EQ(delta.var_count, 2u);
}

TEST_F(PudAcceptorIntegrationTest, CandidateVariableBoundToTheGoalIsSubstitutedAway) {
    const pud_node_id rule_root = root(functor("p", {var(0)}), {functor("q", {var(0)})});
    const pud_child_delta delta = acceptor.accept(empty_state, 0, functor("p", {atom("a")}), rule_root);
    EXPECT_THAT(delta.substitutions, IsEmpty());
    EXPECT_THAT(delta.body_goals, ElementsAre(functor("q", {atom("a")})));
    EXPECT_EQ(delta.var_count, 0u);
}

TEST_F(PudAcceptorIntegrationTest, AcceptorVariablesMergedByTheCandidateBecomeAVariableSubstitution) {
    const pud_node_id same = root(functor("eq", {var(0), var(0)}), {});
    const pud_child_delta delta = acceptor.accept(empty_state, 2, functor("eq", {var(0), var(1)}), same);
    EXPECT_THAT(delta.substitutions, ElementsAre(pud_substitution{.var_idx = 1, .value = var(0)}));
    EXPECT_EQ(delta.var_count, 0u);
}

TEST_F(PudAcceptorIntegrationTest, AcceptorsBindingsAreRespected) {
    const pud_node_id same = root(functor("eq", {var(0), var(0)}), {});
    const pud_child_delta delta = acceptor.accept(
        bound(0, atom("a")), 2, functor("eq", {var(0), var(1)}), same);
    EXPECT_THAT(delta.substitutions, ElementsAre(pud_substitution{.var_idx = 1, .value = atom("a")}));
}

TEST_F(PudAcceptorIntegrationTest, UntouchedAcceptorVariablesAreNotSubstituted) {
    const pud_node_id fact = root(functor("p", {atom("a")}), {});
    const pud_child_delta delta = acceptor.accept(empty_state, 3, functor("p", {var(2)}), fact);
    EXPECT_THAT(delta.substitutions, ElementsAre(pud_substitution{.var_idx = 2, .value = atom("a")}));
}

TEST_F(PudAcceptorIntegrationTest, CandidateDeepInATreeReplaysItsWholePath) {
    // nat(s(V0)) :- nat(V0), expanded once along its recursive branch: V0 := s(V1), goal nat(V1).
    const pud_node_id rule_root = root(functor("nat", {functor("s", {var(0)})}), {functor("nat", {var(0)})});
    const pud_node_id deeper = child(
        rule_root, 0, {pud_substitution{.var_idx = 0, .value = functor("s", {var(1)})}}, {functor("nat", {var(1)})});
    const pud_child_delta delta = acceptor.accept(empty_state, 1, functor("nat", {var(0)}), deeper);
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
    const pud_child_delta delta = acceptor.accept(empty_state, 0, functor("p", {atom("a")}), deeper);
    EXPECT_THAT(delta.body_goals, ElementsAre(functor("r", {atom("a")})));
}

TEST_F(PudAcceptorIntegrationTest, SharedFreshVariablesAreTranslatedConsistently) {
    const pud_node_id rule_root = root(
        functor("p", {var(0), var(1)}),
        {functor("q", {var(1)}), functor("r", {var(1), var(0)})});
    const pud_child_delta delta = acceptor.accept(
        empty_state, 1, functor("p", {var(0), functor("g", {var(0)})}), rule_root);
    EXPECT_THAT(delta.substitutions, IsEmpty());
    EXPECT_THAT(
        delta.body_goals,
        ElementsAre(functor("q", {functor("g", {var(0)})}), functor("r", {functor("g", {var(0)}), var(0)})));
    EXPECT_EQ(delta.var_count, 0u);
}
