#include <deque>
#include <string>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "functor_fixture.hpp"
#include "reference_pud_module_differences.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/pud_module.hpp"

using ::testing::ElementsAre;
using ::testing::IsEmpty;

// The module comparison the reload tests check reloaded and restarted databases with, checked on
// copies of one hand-built module that are then made to differ in one way each. Module 3 has
// axioms 0 (q :- g) and 1 (a fact), and one inference under axiom 0.
struct ReferencePudModuleDifferencesTest : public ::testing::Test {
    test_functors functors;
    expr_pool exprs;
    const expr* q = exprs.make_functor(functors.id("q"), {});
    const expr* g = exprs.make_functor(functors.id("g"), {});
    const expr* h = exprs.make_functor(functors.id("h"), {});
    std::deque<pud_module> expected;
    std::deque<pud_module> actual;

    void SetUp() override {
        pud_module& module = expected.emplace_back(3, std::vector<pud_module_id>{}, std::vector<rule>{rule(q, {g}, 0), rule(g, {}, 0)});
        module.store_inference(pud_inference{.parent = 0, .call_site = 0, .substitutions = {}, .body_goals = {}, .var_count = 0});
        actual = expected;
    }
};

TEST_F(ReferencePudModuleDifferencesTest, CopiesHaveNoDifferences) {
    EXPECT_THAT(reference_pud_module_differences(expected, actual), IsEmpty());
}

TEST_F(ReferencePudModuleDifferencesTest, AMissingModuleIsADifference) {
    actual.pop_back();
    EXPECT_THAT(reference_pud_module_differences(expected, actual), ElementsAre("module count 0 != 1"));
}

TEST_F(ReferencePudModuleDifferencesTest, ADifferentRefutationIsADifference) {
    actual.front().mark_refuted(2);
    EXPECT_THAT(reference_pud_module_differences(expected, actual), ElementsAre("module 3 node 2: refutation"));
}

TEST_F(ReferencePudModuleDifferencesTest, AnExtraChildIsADifference) {
    actual.front().store_inference(pud_inference{.parent = 0, .call_site = 0, .substitutions = {}, .body_goals = {}, .var_count = 0});
    EXPECT_THAT(reference_pud_module_differences(expected, actual), ElementsAre("module 3 node 0: children"));
}

TEST_F(ReferencePudModuleDifferencesTest, ADifferentInferenceIsADifference) {
    actual.clear();
    pud_module& module = actual.emplace_back(3, std::vector<pud_module_id>{}, std::vector<rule>{rule(q, {g}, 0), rule(g, {}, 0)});
    module.store_inference(pud_inference{.parent = 0, .call_site = 0, .substitutions = {}, .body_goals = {h}, .var_count = 0});
    EXPECT_THAT(reference_pud_module_differences(expected, actual), ElementsAre("module 3 node 2: inference"));
}

TEST_F(ReferencePudModuleDifferencesTest, DifferentAxiomsAreADifference) {
    actual.clear();
    actual.emplace_back(3, std::vector<pud_module_id>{}, std::vector<rule>{rule(q, {h}, 0), rule(g, {}, 0)});
    actual.front().store_inference(pud_inference{.parent = 0, .call_site = 0, .substitutions = {}, .body_goals = {}, .var_count = 0});
    EXPECT_THAT(reference_pud_module_differences(expected, actual), ElementsAre("module 3: axiom 0"));
}

TEST_F(ReferencePudModuleDifferencesTest, DifferentDependenciesAreADifference) {
    actual.clear();
    actual.emplace_back(3, std::vector<pud_module_id>{9}, std::vector<rule>{rule(q, {g}, 0), rule(g, {}, 0)});
    actual.front().store_inference(pud_inference{.parent = 0, .call_site = 0, .substitutions = {}, .body_goals = {}, .var_count = 0});
    EXPECT_THAT(reference_pud_module_differences(expected, actual), ElementsAre("module 3: dependencies"));
}
