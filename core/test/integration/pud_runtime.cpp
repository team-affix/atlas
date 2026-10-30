#include <deque>
#include <set>
#include <stdexcept>
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

// The runtime over several modules: one frontier across all of them, with an unfold in one module
// reaching the queries of every module that depends on it. Registering modules that were already
// worked on puts the same frontier back without re-deriving any refutation.
struct PudRuntimeIntegrationTest : public ::testing::Test {
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

TEST_F(PudRuntimeIntegrationTest, RegisteringAModuleBeforeItsDependencyFails) {
    pud_module& mod = modules.emplace_back(
        1,
        std::vector<pud_module_id>{99},
        std::vector<rule>{rule(atom("q"), {}, 0)});
    install_shared_functor_names(mod, this->functors);
    EXPECT_THROW(runtime.register_module(mod), std::logic_error);
}

TEST_F(PudRuntimeIntegrationTest, NodesWithTheSameLocalIdInTwoModulesStayApart) {
    const pud_global_node_id fact = pud_global_node_id{.module = 1, .node = 0};
    pud_module& base = modules.emplace_back(
        1,
        std::vector<pud_module_id>{},
        std::vector<rule>{rule(functor("p", {atom("a")}), {}, 0)});
    install_shared_functor_names(base, this->functors);
    runtime.register_module(base);
    const pud_global_node_id query = pud_global_node_id{.module = 2, .node = 0};
    pud_module& dependent = modules.emplace_back(
        2,
        std::vector<pud_module_id>{fact.module},
        std::vector<rule>{rule(functor("q", {var(0)}), {functor("p", {var(0)})}, 1)});
    install_shared_functor_names(dependent, this->functors);
    runtime.register_module(dependent);
    ASSERT_EQ(fact.node, query.node);
    const pud_frontier_change change = runtime.unfold(query, 0);
    std::vector<pud_global_node_id> query_children;
    for (pud_node_id child : dependent.get_children(query.node))
        query_children.push_back(pud_global_node_id{.module = query.module, .node = child});
    EXPECT_EQ(query_children, change.offspring);
    EXPECT_THAT(base.get_children(fact.node), IsEmpty());
    EXPECT_TRUE(base.check_leaf(fact.node));
    EXPECT_FALSE(base.check_refuted(fact.node));
}

TEST_F(PudRuntimeIntegrationTest, UnfoldingADependencysLeafKeepsADependentsQueryResolvable) {
    const pud_global_node_id step = pud_global_node_id{.module = 1, .node = 1};
    pud_module& nat = modules.emplace_back(
        1,
        std::vector<pud_module_id>{},
        std::vector<rule>{
            rule(functor("nat", {atom("z")}), {}, 0),
            rule(functor("nat", {functor("s", {var(0)})}), {functor("nat", {var(0)})}, 1),
        });
    install_shared_functor_names(nat, this->functors);
    runtime.register_module(nat);
    const pud_global_node_id query = pud_global_node_id{.module = 2, .node = 0};
    pud_module& dependent = modules.emplace_back(
        2,
        std::vector<pud_module_id>{step.module},
        std::vector<rule>{
            rule(atom("q"), {functor("nat", {functor("s", {functor("s", {atom("z")})})})}, 0)});
    install_shared_functor_names(dependent, this->functors);
    runtime.register_module(dependent);
    runtime.unfold(step, 0);
    EXPECT_FALSE(dependent.check_refuted(query.node));
    const pud_frontier_change change = runtime.unfold(query, 0);
    ASSERT_EQ(change.offspring.size(), 1u);
    EXPECT_EQ(change.offspring.front().module, query.module);
    EXPECT_EQ(runtime.unfold(change.offspring.front(), 1).offspring.size(), 1u);
}

TEST_F(PudRuntimeIntegrationTest, ARefutationInADependencyRefutesTheDependentsLeavesThatNeededIt) {
    const pud_global_node_id mid = pud_global_node_id{.module = 1, .node = 2};
    pud_module& base_mod = modules.emplace_back(
        1,
        std::vector<pud_module_id>{},
        std::vector<rule>{
            rule(functor("base", {atom("c")}), {}, 0),
            rule(functor("check", {atom("a")}), {}, 0),
            rule(functor("mid", {var(0)}), {functor("base", {var(0)}), functor("check", {var(0)})}, 1),
        });
    install_shared_functor_names(base_mod, this->functors);
    runtime.register_module(base_mod);
    const pud_global_node_id top = pud_global_node_id{.module = 2, .node = 0};
    const pud_global_node_id other = pud_global_node_id{.module = 2, .node = 1};
    pud_module& dependent = modules.emplace_back(
        2,
        std::vector<pud_module_id>{mid.module},
        std::vector<rule>{
            rule(atom("top"), {functor("mid", {atom("c")})}, 0),
            rule(atom("other"), {functor("check", {atom("a")})}, 0),
        });
    install_shared_functor_names(dependent, this->functors);
    runtime.register_module(dependent);
    const pud_frontier_change change = runtime.unfold(mid, 0);
    EXPECT_THAT(change.refuted, Contains(top));
    EXPECT_TRUE(base_mod.check_refuted(mid.node));
    EXPECT_TRUE(dependent.check_refuted(top.node));
    EXPECT_FALSE(dependent.check_refuted(other.node));
}

TEST_F(PudRuntimeIntegrationTest, ReloadedModulesPutBackTheSameFrontierWithoutRederivingRefutations) {
    const pud_global_node_id step = pud_global_node_id{.module = 1, .node = 1};
    pud_module& nat_colors = modules.emplace_back(
        1,
        std::vector<pud_module_id>{},
        std::vector<rule>{
            rule(functor("nat", {atom("z")}), {}, 0),
            rule(functor("nat", {functor("s", {var(0)})}), {functor("nat", {var(0)})}, 1),
            rule(functor("color", {atom("red")}), {}, 0),
            rule(functor("color", {atom("blue")}), {}, 0),
            rule(functor("warm", {atom("red")}), {}, 0),
        });
    std::set<pud_global_node_id> live;
    install_shared_functor_names(nat_colors, this->functors);
    pud_frontier_change first_register = runtime.register_module(nat_colors);
    live.insert(first_register.offspring.begin(), first_register.offspring.end());
    for (pud_global_node_id refuted : first_register.refuted)
        live.erase(refuted);

    const pud_global_node_id warm_color = pud_global_node_id{.module = 2, .node = 0};
    const pud_global_node_id green = pud_global_node_id{.module = 2, .node = 1};
    const pud_global_node_id number = pud_global_node_id{.module = 2, .node = 2};
    pud_module& mixed = modules.emplace_back(
        2,
        std::vector<pud_module_id>{step.module},
        std::vector<rule>{
            rule(functor("q", {var(0)}), {functor("color", {var(0)}), functor("warm", {var(0)})}, 1),
            rule(atom("g"), {functor("color", {atom("green")})}, 0),
            rule(functor("n", {var(0)}), {functor("nat", {var(0)})}, 1),
        });
    install_shared_functor_names(mixed, this->functors);
    pud_frontier_change second_register = runtime.register_module(mixed);
    live.insert(second_register.offspring.begin(), second_register.offspring.end());
    for (pud_global_node_id refuted : second_register.refuted)
        live.erase(refuted);
    EXPECT_THAT(second_register.refuted, ElementsAre(green));

    live.erase(step);
    pud_frontier_change step_unfold = runtime.unfold(step, 0);
    live.insert(step_unfold.offspring.begin(), step_unfold.offspring.end());
    for (pud_global_node_id refuted : step_unfold.refuted)
        live.erase(refuted);

    live.erase(warm_color);
    const pud_frontier_change colors = runtime.unfold(warm_color, 0);
    live.insert(colors.offspring.begin(), colors.offspring.end());
    for (pud_global_node_id refuted : colors.refuted)
        live.erase(refuted);
    ASSERT_EQ(colors.offspring.size(), 1u);
    ASSERT_EQ(colors.refuted.size(), 1u);

    live.erase(number);
    const pud_frontier_change numbers = runtime.unfold(number, 0);
    live.insert(numbers.offspring.begin(), numbers.offspring.end());
    for (pud_global_node_id refuted : numbers.refuted)
        live.erase(refuted);
    ASSERT_EQ(numbers.offspring.size(), 2u);

    std::deque<pud_module> reloaded;
    pud_runtime<pud_module, expr_pool> reloaded_runtime{exprs};
    std::vector<pud_global_node_id> refuted_on_reload;
    std::set<pud_global_node_id> reloaded_frontier;
    for (const pud_module& data : modules) {
        const pud_frontier_change reload_change = reloaded_runtime.register_module(reloaded.emplace_back(data));
        reloaded_frontier.insert(reload_change.offspring.begin(), reload_change.offspring.end());
        refuted_on_reload.insert(refuted_on_reload.end(), reload_change.refuted.begin(), reload_change.refuted.end());
    }
    EXPECT_EQ(reloaded_frontier, live);
    EXPECT_THAT(refuted_on_reload, IsEmpty());

    const pud_global_node_id red = colors.offspring.front();
    const pud_frontier_change original = runtime.unfold(red, 1);
    EXPECT_EQ(reloaded_runtime.unfold(red, 1), original);
    const pud_module& reloaded_mixed = reloaded.at(1);
    for (pud_node_id child : mixed.get_children(red.node))
        EXPECT_EQ(reloaded_mixed.get_inference(child), mixed.get_inference(child));
}

TEST_F(PudRuntimeIntegrationTest, AReloadedTreeKeepsUnfoldingTheWayTheOriginalDoes) {
    const pud_global_node_id step = pud_global_node_id{.module = 1, .node = 1};
    pud_module& nat = modules.emplace_back(
        1,
        std::vector<pud_module_id>{},
        std::vector<rule>{
            rule(functor("nat", {atom("z")}), {}, 0),
            rule(functor("nat", {functor("s", {var(0)})}), {functor("nat", {var(0)})}, 1),
        });
    std::set<pud_global_node_id> live;
    install_shared_functor_names(nat, this->functors);
    pud_frontier_change first_register = runtime.register_module(nat);
    live.insert(first_register.offspring.begin(), first_register.offspring.end());
    for (pud_global_node_id refuted : first_register.refuted)
        live.erase(refuted);

    const pud_global_node_id number = pud_global_node_id{.module = 2, .node = 0};
    pud_module& dependent = modules.emplace_back(
        2,
        std::vector<pud_module_id>{step.module},
        std::vector<rule>{rule(functor("n", {var(0)}), {functor("nat", {var(0)})}, 1)});
    install_shared_functor_names(dependent, this->functors);
    pud_frontier_change second_register = runtime.register_module(dependent);
    live.insert(second_register.offspring.begin(), second_register.offspring.end());
    for (pud_global_node_id refuted : second_register.refuted)
        live.erase(refuted);

    pud_global_node_id deepest = number;
    pud_body_goal_id goal = 0;
    for (int step_index = 0; step_index < 4; ++step_index) {
        live.erase(deepest);
        const pud_frontier_change change = runtime.unfold(deepest, goal);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
        ASSERT_EQ(change.offspring.size(), 2u) << "unfold " << step_index;
        deepest = change.offspring.back();
        ++goal;
    }
    std::deque<pud_module> reloaded;
    pud_runtime<pud_module, expr_pool> reloaded_runtime{exprs};
    std::vector<pud_global_node_id> refuted_on_reload;
    std::set<pud_global_node_id> reloaded_frontier;
    for (const pud_module& data : modules) {
        const pud_frontier_change reload_change = reloaded_runtime.register_module(reloaded.emplace_back(data));
        reloaded_frontier.insert(reload_change.offspring.begin(), reload_change.offspring.end());
        refuted_on_reload.insert(refuted_on_reload.end(), reload_change.refuted.begin(), reload_change.refuted.end());
    }
    EXPECT_EQ(reloaded_frontier, live);
    const pud_frontier_change original = runtime.unfold(deepest, goal);
    EXPECT_EQ(reloaded_runtime.unfold(deepest, goal), original);
}

TEST_F(PudRuntimeIntegrationTest, AReloadedRefutedNodeIsNeverActivatedAgain) {
    const pud_global_node_id doomed = pud_global_node_id{.module = 1, .node = 1};
    const pud_global_node_id fine = pud_global_node_id{.module = 1, .node = 2};
    pud_module& colors = modules.emplace_back(
        1,
        std::vector<pud_module_id>{},
        std::vector<rule>{
            rule(functor("color", {atom("red")}), {}, 0),
            rule(atom("d"), {functor("color", {atom("green")})}, 0),
            rule(atom("f"), {functor("color", {atom("red")})}, 0),
        });
    install_shared_functor_names(colors, this->functors);
    const pud_frontier_change registered = runtime.register_module(colors);
    EXPECT_THAT(registered.refuted, ElementsAre(doomed));

    std::deque<pud_module> reloaded;
    pud_runtime<pud_module, expr_pool> reloaded_runtime{exprs};
    std::vector<pud_global_node_id> refuted_on_reload;
    std::set<pud_global_node_id> reloaded_frontier;
    for (const pud_module& data : modules) {
        const pud_frontier_change reload_change = reloaded_runtime.register_module(reloaded.emplace_back(data));
        reloaded_frontier.insert(reload_change.offspring.begin(), reload_change.offspring.end());
        refuted_on_reload.insert(refuted_on_reload.end(), reload_change.refuted.begin(), reload_change.refuted.end());
    }
    EXPECT_FALSE(reloaded_frontier.contains(doomed));
    EXPECT_TRUE(reloaded_frontier.contains(fine));
    EXPECT_THAT(refuted_on_reload, IsEmpty());
}
