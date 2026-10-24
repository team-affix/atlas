#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "functor_fixture.hpp"
#include "reference_pud_inference_violations.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/pud_module.hpp"

using ::testing::ElementsAre;
using ::testing::IsEmpty;

struct ReferencePudInferenceViolationsTest : public ::testing::Test {
    test_functors functors;
    expr_pool exprs;

    const expr* var(uint32_t index) { return exprs.make_var(index); }
    const expr* atom(const char* name) { return exprs.make_functor(functors.id(name), {}); }
    const expr* functor(const char* name, std::vector<const expr*> args) {
        return exprs.make_functor(functors.id(name), std::move(args));
    }
};

TEST_F(ReferencePudInferenceViolationsTest, ValidHandBuiltModuleHasNoViolations) {
    pud_module module{
        1,
        {},
        {rule(functor("q", {var(0)}), {functor("p", {var(0)})}, 1)}};
    module.store_inference(pud_inference{
        .parent = 0,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 0, .value = atom("a")}},
        .body_goals = {},
        .var_count = 0});
    EXPECT_THAT(reference_pud_inference_violations(module), IsEmpty());
}

TEST_F(ReferencePudInferenceViolationsTest, CallSiteNotAnOpenGoalOfParent) {
    pud_module module{
        1,
        {},
        {rule(functor("q", {var(0)}), {functor("p", {var(0)})}, 1)}};
    module.store_inference(pud_inference{
        .parent = 0,
        .call_site = 1,
        .substitutions = {},
        .body_goals = {},
        .var_count = 0});
    EXPECT_THAT(
        reference_pud_inference_violations(module),
        ElementsAre("inference node 1: call site 1 is not an open goal of parent 0"));
}

TEST_F(ReferencePudInferenceViolationsTest, SiblingCallSitesMustMatchAtStoreTime) {
    pud_module module{
        1,
        {},
        {rule(functor("q", {var(0)}), {functor("p", {var(0)}), functor("r", {var(0)})}, 1)}};
    module.store_inference(pud_inference{
        .parent = 0,
        .call_site = 0,
        .substitutions = {},
        .body_goals = {functor("s", {})},
        .var_count = 0});
    EXPECT_THROW(
        module.store_inference(pud_inference{
            .parent = 0,
            .call_site = 1,
            .substitutions = {},
            .body_goals = {},
            .var_count = 0}),
        std::logic_error);
    EXPECT_THAT(reference_pud_inference_violations(module), IsEmpty());
}

TEST_F(ReferencePudInferenceViolationsTest, SubstitutionVarIdxMustBeLessThanParentLvc) {
    pud_module module{
        1,
        {},
        {rule(functor("q", {var(0)}), {functor("p", {var(0)})}, 1)}};
    module.store_inference(pud_inference{
        .parent = 0,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 1, .value = atom("a")}},
        .body_goals = {},
        .var_count = 0});
    EXPECT_THAT(
        reference_pud_inference_violations(module),
        ElementsAre("inference node 1: substitution var_idx 1 is not less than parent lvc 1"));
}

TEST_F(ReferencePudInferenceViolationsTest, SubstitutionMustTargetUnboundRepresentative) {
    pud_module module{
        1,
        {},
        {rule(functor("q", {var(0), var(1)}), {functor("p", {var(0)}), functor("r", {var(1)})}, 2)}};
    module.store_inference(pud_inference{
        .parent = 0,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 0, .value = atom("a")}},
        .body_goals = {},
        .var_count = 0});
    module.store_inference(pud_inference{
        .parent = 1,
        .call_site = 1,
        .substitutions = {pud_substitution{.var_idx = 0, .value = atom("b")}},
        .body_goals = {},
        .var_count = 0});
    EXPECT_THAT(
        reference_pud_inference_violations(module),
        ElementsAre(
            "inference node 2: substitution var_idx 0 is not an unbound representative in the parent bindings"));
}

TEST_F(ReferencePudInferenceViolationsTest, NoVariableSubstitutedTwiceInOneInference) {
    pud_module module{
        1,
        {},
        {rule(functor("q", {var(0), var(1)}), {functor("p", {var(0), var(1)})}, 2)}};
    module.store_inference(pud_inference{
        .parent = 0,
        .call_site = 0,
        .substitutions = {
            pud_substitution{.var_idx = 0, .value = var(1)},
            pud_substitution{.var_idx = 0, .value = var(2)},
        },
        .body_goals = {},
        .var_count = 1});
    EXPECT_THAT(
        reference_pud_inference_violations(module),
        ElementsAre("inference node 1: variable 0 is substituted twice"));
}

TEST_F(ReferencePudInferenceViolationsTest, SubstitutionValueVariablesMustBeLessThanNodeLvc) {
    pud_module module{
        1,
        {},
        {rule(functor("q", {var(0)}), {functor("p", {var(0)})}, 1)}};
    module.store_inference(pud_inference{
        .parent = 0,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 0, .value = var(1)}},
        .body_goals = {},
        .var_count = 0});
    EXPECT_THAT(
        reference_pud_inference_violations(module),
        ElementsAre(
            "inference node 1: substitution value uses variable 1 not less than node lvc 1"));
}

TEST_F(ReferencePudInferenceViolationsTest, BodyGoalVariablesMustBeLessThanNodeLvc) {
    pud_module module{
        1,
        {},
        {rule(functor("q", {var(0)}), {functor("p", {var(0)})}, 1)}};
    module.store_inference(pud_inference{
        .parent = 0,
        .call_site = 0,
        .substitutions = {},
        .body_goals = {functor("g", {var(1)})},
        .var_count = 0});
    EXPECT_THAT(
        reference_pud_inference_violations(module),
        ElementsAre("inference node 1: body goal uses variable 1 not less than node lvc 1"));
}

TEST_F(ReferencePudInferenceViolationsTest, AxiomVariablesMustBeLessThanVarCount) {
    pud_module module{
        1,
        {},
        {rule(functor("q", {var(1)}), {functor("p", {var(0)})}, 1)}};
    EXPECT_THAT(
        reference_pud_inference_violations(module),
        ElementsAre("axiom 0: head uses variable 1 not less than var_count 1"));
}
