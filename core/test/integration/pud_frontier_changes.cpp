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
using ::testing::ElementsAreArray;
using ::testing::IsEmpty;
using ::testing::Not;
using ::testing::UnorderedElementsAre;

namespace {

struct PudFrontierChangesIntegrationTest : public ::testing::Test {
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

}

TEST_F(PudFrontierChangesIntegrationTest, AnEmptyModuleOpensNothing) {
    pud_module& mod = modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{});
    install_shared_functor_names(mod, this->functors);
    EXPECT_EQ(runtime.register_module(mod), (pud_frontier_change{.offspring = {}, .refuted = {}}));
}

TEST_F(PudFrontierChangesIntegrationTest, FactsReportAsOffspring) {
    const std::vector<pud_global_node_id> colors = {
        pud_global_node_id{.module = 1, .node = 0},
        pud_global_node_id{.module = 1, .node = 1},
        pud_global_node_id{.module = 1, .node = 2},
    };
    pud_module& mod = modules.emplace_back(
        1,
        std::vector<pud_module_id>{},
        std::vector<rule>{
            rule(functor("color", {atom("red")}), {}, 0),
            rule(functor("color", {atom("blue")}), {}, 0),
            rule(functor("warm", {atom("red")}), {}, 0),
        });
    install_shared_functor_names(mod, this->functors);
    const pud_frontier_change change = runtime.register_module(mod);
    EXPECT_THAT(change.offspring, ElementsAreArray(colors));
    EXPECT_THAT(change.refuted, IsEmpty());
}

TEST_F(PudFrontierChangesIntegrationTest, ARuleOpensWithItsGoals) {
    const std::vector<pud_global_node_id> colors = {
        pud_global_node_id{.module = 1, .node = 0},
        pud_global_node_id{.module = 1, .node = 1},
        pud_global_node_id{.module = 1, .node = 2},
    };
    const pud_global_node_id rule_node = pud_global_node_id{.module = 1, .node = 3};
    pud_module& mod = modules.emplace_back(
        1,
        std::vector<pud_module_id>{},
        std::vector<rule>{
            rule(functor("color", {atom("red")}), {}, 0),
            rule(functor("color", {atom("blue")}), {}, 0),
            rule(functor("warm", {atom("red")}), {}, 0),
            rule(functor("q", {var(0)}), {functor("color", {var(0)})}, 1),
        });
    install_shared_functor_names(mod, this->functors);
    EXPECT_THAT(runtime.register_module(mod).offspring, UnorderedElementsAre(colors.at(0), colors.at(1), colors.at(2), rule_node));
}

TEST_F(PudFrontierChangesIntegrationTest, ARuleWithAnUnprovableGoalOpensRefuted) {
    const pud_global_node_id rule_node = pud_global_node_id{.module = 1, .node = 0};
    pud_module& mod = modules.emplace_back(
        1,
        std::vector<pud_module_id>{},
        std::vector<rule>{rule(atom("q"), {atom("missing")}, 0)});
    install_shared_functor_names(mod, this->functors);
    const pud_frontier_change change = runtime.register_module(mod);
    EXPECT_THAT(change.offspring, IsEmpty());
    EXPECT_THAT(change.refuted, ElementsAre(rule_node));
    EXPECT_TRUE(mod.check_refuted(rule_node.node));
}

TEST_F(PudFrontierChangesIntegrationTest, UnfoldingReportsTheChildrenButNeverTheUnfoldedLeaf) {
    const pud_global_node_id rule_node = pud_global_node_id{.module = 1, .node = 3};
    pud_module& mod = modules.emplace_back(
        1,
        std::vector<pud_module_id>{},
        std::vector<rule>{
            rule(functor("color", {atom("red")}), {}, 0),
            rule(functor("color", {atom("blue")}), {}, 0),
            rule(functor("warm", {atom("red")}), {}, 0),
            rule(functor("q", {var(0)}), {functor("color", {var(0)}), functor("color", {var(0)})}, 1),
        });
    install_shared_functor_names(mod, this->functors);
    runtime.register_module(mod);
    const pud_frontier_change change = runtime.unfold(rule_node, 0);
    std::vector<pud_global_node_id> children;
    for (pud_node_id child : mod.get_children(rule_node.node))
        children.push_back(pud_global_node_id{.module = rule_node.module, .node = child});
    EXPECT_EQ(change.offspring, children);
    EXPECT_THAT(change.offspring, Not(Contains(rule_node)));
}

TEST_F(PudFrontierChangesIntegrationTest, UnfoldingTheLastGoalLeavesGoallessChildrenInOffspring) {
    const pud_global_node_id rule_node = pud_global_node_id{.module = 1, .node = 3};
    pud_module& mod = modules.emplace_back(
        1,
        std::vector<pud_module_id>{},
        std::vector<rule>{
            rule(functor("color", {atom("red")}), {}, 0),
            rule(functor("color", {atom("blue")}), {}, 0),
            rule(functor("warm", {atom("red")}), {}, 0),
            rule(functor("q", {var(0)}), {functor("color", {var(0)})}, 1),
        });
    install_shared_functor_names(mod, this->functors);
    runtime.register_module(mod);
    const pud_frontier_change change = runtime.unfold(rule_node, 0);
    std::vector<pud_global_node_id> children;
    for (pud_node_id child : mod.get_children(rule_node.node))
        children.push_back(pud_global_node_id{.module = rule_node.module, .node = child});
    EXPECT_EQ(change.offspring, children);
}

TEST_F(PudFrontierChangesIntegrationTest, ChildrenAreStoredInTheUnfoldedLeafsModule) {
    const pud_global_node_id rule_node = pud_global_node_id{.module = 1, .node = 3};
    pud_module& mod = modules.emplace_back(
        1,
        std::vector<pud_module_id>{},
        std::vector<rule>{
            rule(functor("color", {atom("red")}), {}, 0),
            rule(functor("color", {atom("blue")}), {}, 0),
            rule(functor("warm", {atom("red")}), {}, 0),
            rule(functor("q", {var(0)}), {functor("color", {var(0)})}, 1),
        });
    install_shared_functor_names(mod, this->functors);
    runtime.register_module(mod);
    for (pud_global_node_id child : runtime.unfold(rule_node, 0).offspring)
        EXPECT_EQ(child.module, rule_node.module);
}

TEST_F(PudFrontierChangesIntegrationTest, AChildRefutedOnArrivalIsReportedRefutedOnly) {
    const pud_global_node_id rule_node = pud_global_node_id{.module = 1, .node = 3};
    pud_module& mod = modules.emplace_back(
        1,
        std::vector<pud_module_id>{},
        std::vector<rule>{
            rule(functor("color", {atom("red")}), {}, 0),
            rule(functor("color", {atom("blue")}), {}, 0),
            rule(functor("warm", {atom("red")}), {}, 0),
            rule(functor("q", {var(0)}), {functor("color", {var(0)}), functor("warm", {var(0)})}, 1),
        });
    install_shared_functor_names(mod, this->functors);
    runtime.register_module(mod);
    const pud_frontier_change change = runtime.unfold(rule_node, 0);
    std::vector<pud_global_node_id> children;
    for (pud_node_id child : mod.get_children(rule_node.node))
        children.push_back(pud_global_node_id{.module = rule_node.module, .node = child});
    ASSERT_EQ(children.size(), 2u);
    const pud_global_node_id red = children.at(0);
    const pud_global_node_id blue = children.at(1);
    EXPECT_THAT(change.offspring, ElementsAre(red));
    EXPECT_THAT(change.refuted, ElementsAre(blue));
    EXPECT_TRUE(mod.check_refuted(blue.node));
    EXPECT_FALSE(mod.check_refuted(red.node));
}

TEST_F(PudFrontierChangesIntegrationTest, SiblingsRefutedOnArrivalThatShareAnExhaustedQueryAreRefutedOnce) {
    const pud_global_node_id rule_node = pud_global_node_id{.module = 1, .node = 4};
    pud_module& mod = modules.emplace_back(
        1,
        std::vector<pud_module_id>{},
        std::vector<rule>{
            rule(functor("q", {var(0)}), {functor("r", {var(0)})}, 1),
            rule(functor("q", {var(0)}), {functor("s", {var(0)})}, 1),
            rule(functor("r", {atom("c")}), {}, 0),
            rule(functor("s", {atom("c")}), {}, 0),
            rule(atom("p"), {atom("p"), functor("q", {atom("d")})}, 0),
        });
    install_shared_functor_names(mod, this->functors);
    runtime.register_module(mod);
    const pud_frontier_change change = runtime.unfold(rule_node, 1);
    std::vector<pud_global_node_id> children;
    for (pud_node_id child : mod.get_children(rule_node.node))
        children.push_back(pud_global_node_id{.module = rule_node.module, .node = child});
    ASSERT_EQ(children.size(), 2u);
    EXPECT_THAT(change.refuted, UnorderedElementsAre(children.at(0), children.at(1)));
    EXPECT_THAT(change.offspring, IsEmpty());
}

TEST_F(PudFrontierChangesIntegrationTest, ARefutationIsReportedInEveryTreeItReaches) {
    const pud_global_node_id mid = pud_global_node_id{.module = 1, .node = 2};
    const pud_global_node_id top = pud_global_node_id{.module = 1, .node = 3};
    pud_module& mod = modules.emplace_back(
        1,
        std::vector<pud_module_id>{},
        std::vector<rule>{
            rule(functor("base", {atom("c")}), {}, 0),
            rule(functor("check", {atom("a")}), {}, 0),
            rule(functor("mid", {var(0)}), {functor("base", {var(0)}), functor("check", {var(0)})}, 1),
            rule(atom("top"), {functor("mid", {atom("c")})}, 0),
        });
    install_shared_functor_names(mod, this->functors);
    runtime.register_module(mod);
    const pud_frontier_change change = runtime.unfold(mid, 0);
    EXPECT_THAT(change.refuted, Contains(top));
    EXPECT_TRUE(mod.check_refuted(mid.node));
    EXPECT_TRUE(mod.check_refuted(top.node));
}

TEST_F(PudFrontierChangesIntegrationTest, AnAxiomWhoseOnlyCandidateIsRefutedDuringRegistrationIsRefutedToo) {
    const pud_global_node_id rule_node = pud_global_node_id{.module = 1, .node = 4};
    pud_module& mod = modules.emplace_back(
        1,
        std::vector<pud_module_id>{},
        std::vector<rule>{
            rule(functor("color", {atom("red")}), {}, 0),
            rule(functor("color", {atom("blue")}), {}, 0),
            rule(functor("warm", {atom("red")}), {}, 0),
            rule(functor("q", {var(0)}), {functor("color", {var(0)}), atom("never")}, 1),
            rule(atom("never"), {atom("never_holds")}, 0),
        });
    install_shared_functor_names(mod, this->functors);
    const pud_frontier_change registered = runtime.register_module(mod);
    EXPECT_THAT(registered.refuted, Contains(rule_node));
    EXPECT_TRUE(mod.check_refuted(rule_node.node));
}
