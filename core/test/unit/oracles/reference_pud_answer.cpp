#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "functor_fixture.hpp"
#include "reference_pud_answer.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/pud_module.hpp"

// The answer reader the answer-set tests compare against, checked on hand-built modules.
struct ReferencePudAnswerTest : public ::testing::Test {
    test_functors functors;
    expr_pool exprs;

    const expr* var(uint32_t index) { return exprs.make_var(index); }
    const expr* atom(const char* name) { return exprs.make_functor(functors.id(name), {}); }
    const expr* functor(const char* name, std::vector<const expr*> args) {
        return exprs.make_functor(functors.id(name), std::move(args));
    }
};

TEST_F(ReferencePudAnswerTest, AnAxiomWithNoInferenceAnswersItsOwnHead) {
    pud_module module{
        1,
        {},
        {rule(functor("q", {var(0)}), {}, 1)}};
    EXPECT_EQ(reference_pud_answer(module, 0, exprs), functor("q", {var(0)}));
}

TEST_F(ReferencePudAnswerTest, OneInstantiationBindsTheHeadVariable) {
    pud_module module{
        1,
        {},
        {rule(functor("q", {var(0)}), {}, 1)}};
    const pud_node_id child = module.store_inference(pud_inference{
        .parent = 0,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 0, .value = atom("a")}},
        .body_goals = {},
        .var_count = 0});
    EXPECT_EQ(reference_pud_answer(module, child, exprs), functor("q", {atom("a")}));
}

TEST_F(ReferencePudAnswerTest, EquateThenInstantiateThroughTheOtherVariable) {
    pud_module module{
        1,
        {},
        {rule(functor("q", {var(0)}), {}, 2)}};
    const pud_node_id equated = module.store_inference(pud_inference{
        .parent = 0,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 0, .value = var(1)}},
        .body_goals = {},
        .var_count = 0});
    const pud_node_id bound = module.store_inference(pud_inference{
        .parent = equated,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 1, .value = atom("a")}},
        .body_goals = {},
        .var_count = 0});
    EXPECT_EQ(reference_pud_answer(module, bound, exprs), functor("q", {atom("a")}));
}

TEST_F(ReferencePudAnswerTest, AChainOfTwoInferencesAppliesBothSubstitutions) {
    pud_module module{
        1,
        {},
        {rule(functor("q", {var(0), var(1)}), {}, 2)}};
    const pud_node_id first = module.store_inference(pud_inference{
        .parent = 0,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 0, .value = atom("a")}},
        .body_goals = {},
        .var_count = 0});
    const pud_node_id second = module.store_inference(pud_inference{
        .parent = first,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 1, .value = atom("b")}},
        .body_goals = {},
        .var_count = 0});
    EXPECT_EQ(reference_pud_answer(module, second, exprs), functor("q", {atom("a"), atom("b")}));
}

TEST_F(ReferencePudAnswerTest, AVariableLeftUnboundReadsAsItsRepresentative) {
    pud_module module{
        1,
        {},
        {rule(functor("pair", {var(0), var(1)}), {}, 2)}};
    const pud_node_id child = module.store_inference(pud_inference{
        .parent = 0,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 0, .value = atom("a")}},
        .body_goals = {},
        .var_count = 0});
    EXPECT_EQ(reference_pud_answer(module, child, exprs), functor("pair", {atom("a"), var(1)}));
}

TEST_F(ReferencePudAnswerTest, ANestedFunctorValueIsNormalizedWhenABoundVariableIsAppliedLater) {
    pud_module module{
        1,
        {},
        {rule(functor("q", {var(0)}), {}, 2)}};
    const pud_node_id nested = module.store_inference(pud_inference{
        .parent = 0,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 0, .value = functor("f", {var(1)})}},
        .body_goals = {},
        .var_count = 0});
    const pud_node_id bound = module.store_inference(pud_inference{
        .parent = nested,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 1, .value = atom("a")}},
        .body_goals = {},
        .var_count = 0});
    EXPECT_EQ(reference_pud_answer(module, bound, exprs), functor("q", {functor("f", {atom("a")})}));
}
