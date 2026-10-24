#include <deque>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "functor_fixture.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/pud_module.hpp"
#include "infrastructure/pud_runtime.hpp"
#include "value_objects/pud_frontier_change.hpp"
#include "value_objects/rule.hpp"

using ::testing::Contains;
using ::testing::ElementsAre;
using ::testing::IsEmpty;

// Which axioms a module's goals may resolve against: its own and those of every module it depends
// on, directly or not, never a dependent's or a sibling's. A goal with no visible candidate refutes
// its leaf as the module is registered.
struct PudModuleVisibilityIntegrationTest : public ::testing::Test {
    expr_pool exprs;
    test_functors functors;
    std::deque<pud_module> modules;
    pud_runtime<pud_module, expr_pool> runtime{exprs};

    const expr* var(uint32_t index) { return exprs.make_var(index); }

    const expr* atom(const std::string& name) {
        return exprs.make_functor(functors.id(name), {});
    }

    const expr* functor(const std::string& name, std::vector<const expr*> args) {
        return exprs.make_functor(functors.id(name), args);
    }
};

TEST_F(PudModuleVisibilityIntegrationTest, ADependentModulesGoalsResolveAgainstTheDependencysAxioms) {
    const pud_global_node_id base_fact = pud_global_node_id{.module = 1, .node = 0};
    pud_module& base = modules.emplace_back(
        1,
        std::vector<pud_module_id>{},
        std::vector<rule>{rule(functor("p", {atom("a")}), {}, 0)});
    runtime.register_module(base);
    const pud_global_node_id query = pud_global_node_id{.module = 2, .node = 0};
    pud_module& dependent = modules.emplace_back(
        2,
        std::vector<pud_module_id>{base_fact.module},
        std::vector<rule>{rule(functor("q", {var(0)}), {functor("p", {var(0)})}, 1)});
    EXPECT_THAT(runtime.register_module(dependent).offspring, ElementsAre(query));
    const pud_frontier_change change = runtime.unfold(query, 0);
    ASSERT_EQ(change.offspring.size(), 1u);
    EXPECT_EQ(change.offspring.front().module, query.module);
    const pud_global_node_id child = change.offspring.front();
    const expr* bound = nullptr;
    for (const pud_substitution& substitution : dependent.get_inference(child.node).substitutions) {
        if (substitution.var_idx == 0)
            bound = substitution.value;
    }
    EXPECT_EQ(bound, atom("a"));
}

TEST_F(PudModuleVisibilityIntegrationTest, AModuleWithoutTheDependencyCannotResolveAgainstIt) {
    const pud_global_node_id query = pud_global_node_id{.module = 2, .node = 0};
    pud_module& base = modules.emplace_back(
        1,
        std::vector<pud_module_id>{},
        std::vector<rule>{rule(functor("p", {atom("a")}), {}, 0)});
    runtime.register_module(base);
    pud_module& isolated = modules.emplace_back(
        2,
        std::vector<pud_module_id>{},
        std::vector<rule>{rule(functor("q", {var(0)}), {functor("p", {var(0)})}, 1)});
    const pud_frontier_change change = runtime.register_module(isolated);
    EXPECT_THAT(change.refuted, ElementsAre(query));
    EXPECT_TRUE(isolated.check_refuted(query.node));
}

TEST_F(PudModuleVisibilityIntegrationTest, ADependencyNeverSeesItsDependentsAxioms) {
    const pud_global_node_id query = pud_global_node_id{.module = 1, .node = 0};
    pud_module& first = modules.emplace_back(
        1,
        std::vector<pud_module_id>{},
        std::vector<rule>{rule(atom("q"), {functor("l", {atom("a")})}, 0)});
    EXPECT_THAT(runtime.register_module(first).refuted, ElementsAre(query));
    pud_module& second = modules.emplace_back(
        2,
        std::vector<pud_module_id>{query.module},
        std::vector<rule>{rule(functor("l", {atom("a")}), {}, 0)});
    runtime.register_module(second);
    EXPECT_TRUE(first.check_refuted(query.node));
}

TEST_F(PudModuleVisibilityIntegrationTest, SiblingModulesDoNotSeeEachOthersAxioms) {
    const pud_global_node_id base_fact = pud_global_node_id{.module = 1, .node = 0};
    const pud_global_node_id query = pud_global_node_id{.module = 3, .node = 0};
    pud_module& base = modules.emplace_back(
        1,
        std::vector<pud_module_id>{},
        std::vector<rule>{rule(functor("p", {atom("a")}), {}, 0)});
    runtime.register_module(base);
    pud_module& sibling = modules.emplace_back(
        2,
        std::vector<pud_module_id>{base_fact.module},
        std::vector<rule>{rule(functor("l", {atom("x")}), {}, 0)});
    runtime.register_module(sibling);
    pud_module& dependent = modules.emplace_back(
        3,
        std::vector<pud_module_id>{base_fact.module},
        std::vector<rule>{rule(functor("q", {var(0)}), {functor("l", {var(0)})}, 1)});
    EXPECT_THAT(runtime.register_module(dependent).refuted, ElementsAre(query));
}

TEST_F(PudModuleVisibilityIntegrationTest, IndirectDependenciesAreVisible) {
    const pud_global_node_id base_fact = pud_global_node_id{.module = 1, .node = 0};
    const pud_global_node_id middle_fact = pud_global_node_id{.module = 2, .node = 0};
    const pud_global_node_id query = pud_global_node_id{.module = 3, .node = 0};
    pud_module& base = modules.emplace_back(
        1,
        std::vector<pud_module_id>{},
        std::vector<rule>{rule(functor("p", {atom("a")}), {}, 0)});
    runtime.register_module(base);
    pud_module& middle = modules.emplace_back(
        2,
        std::vector<pud_module_id>{base_fact.module},
        std::vector<rule>{rule(functor("r", {atom("a")}), {}, 0)});
    runtime.register_module(middle);
    pud_module& top = modules.emplace_back(
        3,
        std::vector<pud_module_id>{middle_fact.module},
        std::vector<rule>{
            rule(functor("q", {var(0)}), {functor("p", {var(0)}), functor("r", {var(0)})}, 1)});
    EXPECT_THAT(runtime.register_module(top).offspring, ElementsAre(query));
    const pud_frontier_change change = runtime.unfold(query, 0);
    ASSERT_EQ(change.offspring.size(), 1u);
    EXPECT_EQ(runtime.unfold(change.offspring.front(), 1).offspring.size(), 1u);
}

TEST_F(PudModuleVisibilityIntegrationTest, AxiomsOfTheSameModuleAreCandidatesForEachOther) {
    const pud_global_node_id query = pud_global_node_id{.module = 1, .node = 1};
    pud_module& mod = modules.emplace_back(
        1,
        std::vector<pud_module_id>{},
        std::vector<rule>{
            rule(functor("p", {atom("a")}), {}, 0),
            rule(functor("q", {var(0)}), {functor("p", {var(0)})}, 1),
        });
    runtime.register_module(mod);
    EXPECT_EQ(runtime.unfold(query, 0).offspring.size(), 1u);
}

TEST_F(PudModuleVisibilityIntegrationTest, CandidatesComeFromDependenciesFirstThenTheModulesOwnAxioms) {
    const pud_global_node_id red = pud_global_node_id{.module = 1, .node = 0};
    const pud_global_node_id query = pud_global_node_id{.module = 2, .node = 1};
    pud_module& colors = modules.emplace_back(
        1,
        std::vector<pud_module_id>{},
        std::vector<rule>{
            rule(functor("color", {atom("red")}), {}, 0),
            rule(functor("color", {atom("blue")}), {}, 0),
        });
    runtime.register_module(colors);
    pud_module& extended = modules.emplace_back(
        2,
        std::vector<pud_module_id>{red.module},
        std::vector<rule>{
            rule(functor("color", {atom("green")}), {}, 0),
            rule(functor("q", {var(0)}), {functor("color", {var(0)})}, 1),
        });
    runtime.register_module(extended);
    const std::vector<pud_global_node_id> children = runtime.unfold(query, 0).offspring;
    ASSERT_EQ(children.size(), 3u);
    const expr* bound0 = nullptr;
    for (const pud_substitution& substitution : extended.get_inference(children.at(0).node).substitutions) {
        if (substitution.var_idx == 0)
            bound0 = substitution.value;
    }
    const expr* bound1 = nullptr;
    for (const pud_substitution& substitution : extended.get_inference(children.at(1).node).substitutions) {
        if (substitution.var_idx == 0)
            bound1 = substitution.value;
    }
    const expr* bound2 = nullptr;
    for (const pud_substitution& substitution : extended.get_inference(children.at(2).node).substitutions) {
        if (substitution.var_idx == 0)
            bound2 = substitution.value;
    }
    EXPECT_EQ(bound0, atom("red"));
    EXPECT_EQ(bound1, atom("blue"));
    EXPECT_EQ(bound2, atom("green"));
}

TEST_F(PudModuleVisibilityIntegrationTest, AnUnfoldedChildKeepsItsModulesVisibility) {
    const pud_global_node_id red = pud_global_node_id{.module = 1, .node = 0};
    const pud_global_node_id query = pud_global_node_id{.module = 2, .node = 1};
    pud_module& colors = modules.emplace_back(
        1,
        std::vector<pud_module_id>{},
        std::vector<rule>{
            rule(functor("color", {atom("red")}), {}, 0),
            rule(functor("color", {atom("blue")}), {}, 0),
        });
    runtime.register_module(colors);
    pud_module& extended = modules.emplace_back(
        2,
        std::vector<pud_module_id>{red.module},
        std::vector<rule>{
            rule(functor("warm", {atom("red")}), {}, 0),
            rule(functor("q", {var(0)}), {functor("color", {var(0)}), functor("warm", {var(0)})}, 1),
        });
    runtime.register_module(extended);
    const pud_frontier_change change = runtime.unfold(query, 0);
    ASSERT_EQ(change.offspring.size(), 1u);
    const expr* bound = nullptr;
    for (const pud_substitution& substitution : extended.get_inference(change.offspring.front().node).substitutions) {
        if (substitution.var_idx == 0)
            bound = substitution.value;
    }
    EXPECT_EQ(bound, atom("red"));
    EXPECT_EQ(change.refuted.size(), 1u);
    EXPECT_EQ(runtime.unfold(change.offspring.front(), 1).offspring.size(), 1u);
}

TEST_F(PudModuleVisibilityIntegrationTest, TwoModulesOnOneDependencyStayIsolated) {
    const pud_global_node_id base_fact = pud_global_node_id{.module = 1, .node = 0};
    const pud_global_node_id first_query = pud_global_node_id{.module = 2, .node = 1};
    const pud_global_node_id second_query = pud_global_node_id{.module = 3, .node = 0};
    pud_module& base = modules.emplace_back(
        1,
        std::vector<pud_module_id>{},
        std::vector<rule>{rule(functor("p", {atom("a")}), {}, 0)});
    runtime.register_module(base);
    pud_module& first = modules.emplace_back(
        2,
        std::vector<pud_module_id>{base_fact.module},
        std::vector<rule>{
            rule(functor("f", {atom("a")}), {}, 0),
            rule(functor("q1", {var(0)}), {functor("p", {var(0)}), functor("f", {var(0)})}, 1),
        });
    runtime.register_module(first);
    pud_module& second = modules.emplace_back(
        3,
        std::vector<pud_module_id>{base_fact.module},
        std::vector<rule>{rule(functor("q2", {var(0)}), {functor("p", {var(0)}), functor("f", {var(0)})}, 1)});
    const pud_frontier_change second_change = runtime.register_module(second);
    EXPECT_FALSE(first.check_refuted(first_query.node));
    EXPECT_THAT(second_change.refuted, Contains(second_query));
    EXPECT_THAT(second_change.offspring, IsEmpty());
}
