#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "pud_frontier_test_fixture.hpp"

using ::testing::Contains;
using ::testing::ElementsAre;
using ::testing::IsEmpty;

// Which axioms a module's goals may resolve against: its own and those of every module it depends
// on, directly or not, never a dependent's or a sibling's. A goal with no visible candidate refutes
// its leaf as the module is registered.
struct PudModuleVisibilityIntegrationTest : public ::testing::Test {
    pud_frontier_test_fixture h;

    const expr* bound_value(pud_global_node_id child, uint32_t var_index) {
        for (const pud_substitution& substitution : h.data_of(child.module).get_inference(child.node).substitutions) {
            if (substitution.var_idx == var_index)
                return substitution.value;
        }
        return nullptr;
    }
};

TEST_F(PudModuleVisibilityIntegrationTest, ADependentModulesGoalsResolveAgainstTheDependencysAxioms) {
    h.axiom(h.functor("p", {h.atom("a")}), {}, 0);
    const pud_module_id base = h.register_module().offspring.front().module;
    h.depend_on(base);
    const pud_global_node_id query = h.axiom(h.functor("q", {h.var(0)}), {h.functor("p", {h.var(0)})}, 1);
    EXPECT_THAT(h.register_module().offspring, ElementsAre(query));
    const pud_frontier_change change = h.expand(query, 0);
    ASSERT_EQ(change.offspring.size(), 1u);
    EXPECT_EQ(change.offspring.front().module, query.module);
    EXPECT_EQ(bound_value(change.offspring.front(), 0), h.atom("a"));
}

TEST_F(PudModuleVisibilityIntegrationTest, AModuleWithoutTheDependencyCannotResolveAgainstIt) {
    h.axiom(h.functor("p", {h.atom("a")}), {}, 0);
    h.register_module();
    const pud_global_node_id query = h.axiom(h.functor("q", {h.var(0)}), {h.functor("p", {h.var(0)})}, 1);
    const pud_frontier_change change = h.register_module();
    EXPECT_THAT(change.refuted, ElementsAre(query));
    EXPECT_TRUE(h.refuted(query));
}

TEST_F(PudModuleVisibilityIntegrationTest, ADependencyNeverSeesItsDependentsAxioms) {
    const pud_global_node_id query = h.axiom(h.atom("q"), {h.functor("l", {h.atom("a")})}, 0);
    EXPECT_THAT(h.register_module().refuted, ElementsAre(query));
    h.depend_on(query.module);
    h.axiom(h.functor("l", {h.atom("a")}), {}, 0);
    h.register_module();
    EXPECT_TRUE(h.refuted(query));
}

TEST_F(PudModuleVisibilityIntegrationTest, SiblingModulesDoNotSeeEachOthersAxioms) {
    const pud_global_node_id base_fact = h.axiom(h.functor("p", {h.atom("a")}), {}, 0);
    h.register_module();
    h.depend_on(base_fact.module);
    h.axiom(h.functor("l", {h.atom("x")}), {}, 0);
    h.register_module();
    h.depend_on(base_fact.module);
    const pud_global_node_id query = h.axiom(h.functor("q", {h.var(0)}), {h.functor("l", {h.var(0)})}, 1);
    EXPECT_THAT(h.register_module().refuted, ElementsAre(query));
}

TEST_F(PudModuleVisibilityIntegrationTest, IndirectDependenciesAreVisible) {
    const pud_global_node_id base_fact = h.axiom(h.functor("p", {h.atom("a")}), {}, 0);
    h.register_module();
    h.depend_on(base_fact.module);
    const pud_global_node_id middle_fact = h.axiom(h.functor("r", {h.atom("a")}), {}, 0);
    h.register_module();
    h.depend_on(middle_fact.module);
    const pud_global_node_id query =
        h.axiom(h.functor("q", {h.var(0)}), {h.functor("p", {h.var(0)}), h.functor("r", {h.var(0)})}, 1);
    EXPECT_THAT(h.register_module().offspring, ElementsAre(query));
    const pud_frontier_change change = h.expand(query, 0);
    ASSERT_EQ(change.offspring.size(), 1u);
    EXPECT_EQ(h.expand(change.offspring.front(), 1).offspring.size(), 1u);
}

TEST_F(PudModuleVisibilityIntegrationTest, AxiomsOfTheSameModuleAreCandidatesForEachOther) {
    h.axiom(h.functor("p", {h.atom("a")}), {}, 0);
    const pud_global_node_id query = h.axiom(h.functor("q", {h.var(0)}), {h.functor("p", {h.var(0)})}, 1);
    h.register_module();
    EXPECT_EQ(h.expand(query, 0).offspring.size(), 1u);
}

TEST_F(PudModuleVisibilityIntegrationTest, CandidatesComeFromDependenciesFirstThenTheModulesOwnAxioms) {
    const pud_global_node_id red = h.axiom(h.functor("color", {h.atom("red")}), {}, 0);
    h.axiom(h.functor("color", {h.atom("blue")}), {}, 0);
    h.register_module();
    h.depend_on(red.module);
    h.axiom(h.functor("color", {h.atom("green")}), {}, 0);
    const pud_global_node_id query = h.axiom(h.functor("q", {h.var(0)}), {h.functor("color", {h.var(0)})}, 1);
    h.register_module();
    const std::vector<pud_global_node_id> children = h.expand(query, 0).offspring;
    ASSERT_EQ(children.size(), 3u);
    EXPECT_EQ(bound_value(children.at(0), 0), h.atom("red"));
    EXPECT_EQ(bound_value(children.at(1), 0), h.atom("blue"));
    EXPECT_EQ(bound_value(children.at(2), 0), h.atom("green"));
}

TEST_F(PudModuleVisibilityIntegrationTest, AnExpandedChildKeepsItsModulesVisibility) {
    const pud_global_node_id red = h.axiom(h.functor("color", {h.atom("red")}), {}, 0);
    h.axiom(h.functor("color", {h.atom("blue")}), {}, 0);
    h.register_module();
    h.depend_on(red.module);
    h.axiom(h.functor("warm", {h.atom("red")}), {}, 0);
    const pud_global_node_id query =
        h.axiom(h.functor("q", {h.var(0)}), {h.functor("color", {h.var(0)}), h.functor("warm", {h.var(0)})}, 1);
    h.register_module();
    const pud_frontier_change change = h.expand(query, 0);
    ASSERT_EQ(change.offspring.size(), 1u);
    EXPECT_EQ(bound_value(change.offspring.front(), 0), h.atom("red"));
    EXPECT_EQ(change.refuted.size(), 1u);
    EXPECT_EQ(h.expand(change.offspring.front(), 1).offspring.size(), 1u);
}

TEST_F(PudModuleVisibilityIntegrationTest, TwoModulesOnOneDependencyStayIsolated) {
    const pud_global_node_id base_fact = h.axiom(h.functor("p", {h.atom("a")}), {}, 0);
    h.register_module();
    h.depend_on(base_fact.module);
    h.axiom(h.functor("f", {h.atom("a")}), {}, 0);
    const pud_global_node_id first_query =
        h.axiom(h.functor("q1", {h.var(0)}), {h.functor("p", {h.var(0)}), h.functor("f", {h.var(0)})}, 1);
    h.register_module();
    h.depend_on(base_fact.module);
    const pud_global_node_id second_query =
        h.axiom(h.functor("q2", {h.var(0)}), {h.functor("p", {h.var(0)}), h.functor("f", {h.var(0)})}, 1);
    const pud_frontier_change second = h.register_module();
    EXPECT_FALSE(h.refuted(first_query));
    EXPECT_THAT(second.refuted, Contains(second_query));
    EXPECT_THAT(second.offspring, IsEmpty());
}
