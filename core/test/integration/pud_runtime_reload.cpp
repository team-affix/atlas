#include <algorithm>
#include <cstdint>
#include <deque>
#include <limits>
#include <memory>
#include <optional>
#include <random>
#include <set>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "functor_fixture.hpp"
#include "reference_pud_frontier.hpp"
#include "reference_pud_module_differences.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/pud_module.hpp"
#include "infrastructure/pud_runtime.hpp"

using ::testing::IsEmpty;
using ::testing::Not;
using ::testing::UnorderedElementsAreArray;

// Reloading onto copies: a fresh runtime registers copies of the caller's modules while the original
// runtime goes on, so the two can be compared at every step. Each test builds its program, drives
// the runtime, and checks the result inline; the only shared code is the reference walks.
template<typename IRuntime>
struct PudRuntimeReloadIntegrationTest : public ::testing::Test {
    test_functors functors;
    expr_pool exprs;

    const expr* var(uint32_t index) { return exprs.make_var(index); }
    const expr* atom(const char* name) { return exprs.make_functor(functors.id(name), {}); }
    const expr* functor(const char* name, std::vector<const expr*> args) {
        return exprs.make_functor(functors.id(name), std::move(args));
    }
};

using reloaded_runtimes_t = ::testing::Types<pud_runtime<pud_module, expr_pool>>;
TYPED_TEST_SUITE(PudRuntimeReloadIntegrationTest, reloaded_runtimes_t);

// Reloading: the frontier, the refutations and the modules come back exactly as they were left.

TYPED_TEST(PudRuntimeReloadIntegrationTest, AModuleWithoutAxiomsReloadsToAnEmptyFrontier) {
    std::deque<pud_module> modules;
    modules.emplace_back(9, std::vector<pud_module_id>{}, std::vector<rule>{});
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());
    EXPECT_THAT(reloaded_live, IsEmpty());

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, FactsAloneReloadAsTheirOwnFrontier) {
    std::deque<pud_module> modules;
    modules.emplace_back(10, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("p", {this->atom("a")}), {}, 0),
        rule(this->functor("p", {this->atom("b")}), {}, 0),
        rule(this->functor("q", {this->atom("a")}), {}, 0),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());
    EXPECT_EQ(reloaded_live.size(), 3u);

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, AxiomsWithNothingDerivedReload) {
    std::deque<pud_module> modules;
    modules.emplace_back(1001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("color", {this->atom("red")}), {}, 0),
        rule(this->functor("color", {this->atom("blue")}), {}, 0),
        rule(this->functor("color", {this->atom("green")}), {}, 0),
        rule(this->functor("warm", {this->atom("red")}), {}, 0),
        rule(this->functor("warm", {this->atom("orange")}), {}, 0),
        rule(
            this->functor("pair", {this->var(0), this->var(1)}),
            {this->functor("color", {this->var(0)}), this->functor("color", {this->var(1)})},
            2),
        rule(
            this->functor("warm_color", {this->var(0)}),
            {this->functor("color", {this->var(0)}), this->functor("warm", {this->var(0)})},
            1),
        rule(this->atom("impossible"), {this->functor("color", {this->atom("purple")})}, 0),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, ARefutationFoundOnRegistrationIsNotFoundAgainOnReload) {
    std::deque<pud_module> modules;
    modules.emplace_back(7001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("a", {this->atom("one")}), {}, 0),
        rule(this->functor("a", {this->atom("two")}), {}, 0),
        rule(this->functor("b", {this->atom("two")}), {}, 0),
        rule(this->functor("d", {this->atom("one")}), {}, 0),
        rule(this->functor("c", {this->var(0)}), {this->functor("a", {this->var(0)}), this->functor("b", {this->var(0)}), this->functor("d", {this->var(0)})}, 1),
        rule(this->functor("e", {this->var(0)}), {this->functor("c", {this->var(0)})}, 1),
        rule(this->atom("f"), {this->atom("missing")}, 0),
        rule(this->functor("g", {this->var(0)}), {this->functor("a", {this->var(0)}), this->functor("missing", {this->var(0)})}, 1),
        rule(this->functor("h", {this->var(0)}), {this->functor("a", {this->var(0)}), this->functor("d", {this->var(0)})}, 1),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    std::vector<pud_global_node_id> refuted_on_registration;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        refuted_on_registration.insert(refuted_on_registration.end(), change.refuted.begin(), change.refuted.end());
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }

    ASSERT_THAT(refuted_on_registration, Not(IsEmpty()));
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, AFewUnfoldsReload) {
    std::mt19937 rng(0);
    std::deque<pud_module> modules;
    modules.emplace_back(1001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("color", {this->atom("red")}), {}, 0),
        rule(this->functor("color", {this->atom("blue")}), {}, 0),
        rule(this->functor("color", {this->atom("green")}), {}, 0),
        rule(this->functor("warm", {this->atom("red")}), {}, 0),
        rule(this->functor("warm", {this->atom("orange")}), {}, 0),
        rule(
            this->functor("pair", {this->var(0), this->var(1)}),
            {this->functor("color", {this->var(0)}), this->functor("color", {this->var(1)})},
            2),
        rule(
            this->functor("warm_color", {this->var(0)}),
            {this->functor("color", {this->var(0)}), this->functor("warm", {this->var(0)})},
            1),
        rule(this->atom("impossible"), {this->functor("color", {this->atom("purple")})}, 0),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }

    size_t taken = 0;
    for (; taken < 3; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    ASSERT_EQ(taken, 3u);
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());

    std::deque<pud_module> copies(modules.begin(), modules.end());
    TypeParam reloaded(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : copies) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(copies));
    EXPECT_THAT(reference_pud_stranded_nodes(copies), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, copies), IsEmpty());
}

TYPED_TEST(PudRuntimeReloadIntegrationTest, AProgramUnfoldedToExhaustionReloads) {
    std::mt19937 rng(0);
    std::deque<pud_module> modules;
    modules.emplace_back(1001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("color", {this->atom("red")}), {}, 0),
        rule(this->functor("color", {this->atom("blue")}), {}, 0),
        rule(this->functor("color", {this->atom("green")}), {}, 0),
        rule(this->functor("warm", {this->atom("red")}), {}, 0),
        rule(this->functor("warm", {this->atom("orange")}), {}, 0),
        rule(
            this->functor("pair", {this->var(0), this->var(1)}),
            {this->functor("color", {this->var(0)}), this->functor("color", {this->var(1)})},
            2),
        rule(
            this->functor("warm_color", {this->var(0)}),
            {this->functor("color", {this->var(0)}), this->functor("warm", {this->var(0)})},
            1),
        rule(this->atom("impossible"), {this->functor("color", {this->atom("purple")})}, 0),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < 2000u; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(0);
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    EXPECT_LT(taken, 2000u) << "the program did not run out of unfolds";

    EXPECT_GT(taken, 5u);
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, AFrontierOfOnlySolvedLeavesReloads) {
    std::mt19937 rng(0);
    std::deque<pud_module> modules;
    modules.emplace_back(1001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("color", {this->atom("red")}), {}, 0),
        rule(this->functor("color", {this->atom("blue")}), {}, 0),
        rule(this->functor("color", {this->atom("green")}), {}, 0),
        rule(this->functor("warm", {this->atom("red")}), {}, 0),
        rule(this->functor("warm", {this->atom("orange")}), {}, 0),
        rule(
            this->functor("pair", {this->var(0), this->var(1)}),
            {this->functor("color", {this->var(0)}), this->functor("color", {this->var(1)})},
            2),
        rule(
            this->functor("warm_color", {this->var(0)}),
            {this->functor("color", {this->var(0)}), this->functor("warm", {this->var(0)})},
            1),
        rule(this->atom("impossible"), {this->functor("color", {this->atom("purple")})}, 0),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < 2000u; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(0);
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    EXPECT_LT(taken, 2000u) << "the program did not run out of unfolds";

    for (pud_global_node_id leaf : live) {
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        EXPECT_THAT(goal_ids, IsEmpty());
    }
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, ADeeplyUnfoldedDatabaseReloads) {
    std::deque<pud_module> modules;
    modules.emplace_back(2001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {this->atom("z")}), {}, 0),
        rule(this->functor("nat", {this->functor("s", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("add", {this->atom("z"), this->var(0), this->var(0)}), {}, 1),
        rule(
            this->functor("add", {this->functor("s", {this->var(0)}), this->var(1), this->functor("s", {this->var(2)})}),
            {this->functor("add", {this->var(0), this->var(1), this->var(2)})},
            3),
    });
    modules.emplace_back(2002, std::vector<pud_module_id>{2001}, std::vector<rule>{
        rule(
            this->functor("half", {this->var(0)}),
            {this->functor("add", {this->var(0), this->var(0),
                this->functor("s", {this->functor("s", {this->functor("s", {this->functor("s", {this->atom("z")})})})})})},
            1),
        rule(this->functor("number", {this->var(0)}), {this->functor("nat", {this->var(0)})}, 1),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }

    pud_global_node_id deepest{.module = 2001, .node = 1};
    pud_body_goal_id goal = 0;
    for (int level = 0; level < 12; ++level) {
        const pud_frontier_change change = runtime.unfold(deepest, goal);
        live.erase(deepest);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
        ASSERT_FALSE(change.offspring.empty()) << "level " << level;
        deepest = change.offspring.back();
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal_expr] : runtime.get_rule_information(deepest).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        ASSERT_THAT(goal_ids, Not(IsEmpty())) << "level " << level;
        goal = goal_ids.front();
    }
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, ManyRandomUnfoldsOfArithmeticReload) {
    std::mt19937 rng(1);
    std::deque<pud_module> modules;
    modules.emplace_back(2001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {this->atom("z")}), {}, 0),
        rule(this->functor("nat", {this->functor("s", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("add", {this->atom("z"), this->var(0), this->var(0)}), {}, 1),
        rule(
            this->functor("add", {this->functor("s", {this->var(0)}), this->var(1), this->functor("s", {this->var(2)})}),
            {this->functor("add", {this->var(0), this->var(1), this->var(2)})},
            3),
    });
    modules.emplace_back(2002, std::vector<pud_module_id>{2001}, std::vector<rule>{
        rule(
            this->functor("half", {this->var(0)}),
            {this->functor("add", {this->var(0), this->var(0),
                this->functor("s", {this->functor("s", {this->functor("s", {this->functor("s", {this->atom("z")})})})})})},
            1),
        rule(this->functor("number", {this->var(0)}), {this->functor("nat", {this->var(0)})}, 1),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < 40; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }

    ASSERT_EQ(taken, 40u);
    size_t node_count = 0;
    for (const pud_module& module : modules)
        node_count += reference_pud_count_nodes(module);

    EXPECT_GT(node_count, 60u);
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, UnfoldingOnlyTheQueriesOfArithmeticReloads) {
    std::mt19937 rng(2);
    std::deque<pud_module> modules;
    modules.emplace_back(2001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {this->atom("z")}), {}, 0),
        rule(this->functor("nat", {this->functor("s", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("add", {this->atom("z"), this->var(0), this->var(0)}), {}, 1),
        rule(
            this->functor("add", {this->functor("s", {this->var(0)}), this->var(1), this->functor("s", {this->var(2)})}),
            {this->functor("add", {this->var(0), this->var(1), this->var(2)})},
            3),
    });
    modules.emplace_back(2002, std::vector<pud_module_id>{2001}, std::vector<rule>{
        rule(
            this->functor("half", {this->var(0)}),
            {this->functor("add", {this->var(0), this->var(0),
                this->functor("s", {this->functor("s", {this->functor("s", {this->functor("s", {this->atom("z")})})})})})},
            1),
        rule(this->functor("number", {this->var(0)}), {this->functor("nat", {this->var(0)})}, 1),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < 30; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = 2002;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }

    ASSERT_EQ(taken, 30u);
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, RandomUnfoldsOfListsReload) {
    std::mt19937 rng(3);
    std::deque<pud_module> modules;
    modules.emplace_back(3001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("app", {this->atom("nil"), this->var(0), this->var(0)}), {}, 1),
        rule(
            this->functor("app", {this->functor("cons", {this->var(0), this->var(1)}), this->var(2), this->functor("cons", {this->var(0), this->var(3)})}),
            {this->functor("app", {this->var(1), this->var(2), this->var(3)})},
            4),
    });
    const expr* list_abc = this->atom("nil");
    list_abc = this->functor("cons", {this->atom("c"), list_abc});
    list_abc = this->functor("cons", {this->atom("b"), list_abc});
    list_abc = this->functor("cons", {this->atom("a"), list_abc});
    const expr* list_a = this->functor("cons", {this->atom("a"), this->atom("nil")});
    const expr* list_ab = this->functor("cons", {this->atom("a"), this->functor("cons", {this->atom("b"), this->atom("nil")})});
    modules.emplace_back(3002, std::vector<pud_module_id>{3001}, std::vector<rule>{
        rule(
            this->functor("split", {this->var(0), this->var(1)}),
            {this->functor("app", {this->var(0), this->var(1), list_abc})},
            2),
        rule(
            this->functor("suffix", {this->var(0)}),
            {this->functor("app", {list_a, this->var(0), list_ab})},
            1),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < 30; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }

    ASSERT_EQ(taken, 30u);
    size_t node_count = 0;
    for (const pud_module& module : modules)
        node_count += reference_pud_count_nodes(module);

    EXPECT_GT(node_count, 40u);
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, ListQueriesUnfoldedToExhaustionReload) {
    std::mt19937 rng(4);
    std::deque<pud_module> modules;
    modules.emplace_back(3001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("app", {this->atom("nil"), this->var(0), this->var(0)}), {}, 1),
        rule(
            this->functor("app", {this->functor("cons", {this->var(0), this->var(1)}), this->var(2), this->functor("cons", {this->var(0), this->var(3)})}),
            {this->functor("app", {this->var(1), this->var(2), this->var(3)})},
            4),
    });
    const expr* list_abc = this->atom("nil");
    list_abc = this->functor("cons", {this->atom("c"), list_abc});
    list_abc = this->functor("cons", {this->atom("b"), list_abc});
    list_abc = this->functor("cons", {this->atom("a"), list_abc});
    const expr* list_a = this->functor("cons", {this->atom("a"), this->atom("nil")});
    const expr* list_ab = this->functor("cons", {this->atom("a"), this->functor("cons", {this->atom("b"), this->atom("nil")})});
    modules.emplace_back(3002, std::vector<pud_module_id>{3001}, std::vector<rule>{
        rule(
            this->functor("split", {this->var(0), this->var(1)}),
            {this->functor("app", {this->var(0), this->var(1), list_abc})},
            2),
        rule(
            this->functor("suffix", {this->var(0)}),
            {this->functor("app", {list_a, this->var(0), list_ab})},
            1),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < 2000u; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = 3002;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(0);
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    EXPECT_LT(taken, 2000u) << "the program did not run out of unfolds";

    EXPECT_GT(taken, 3u);
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, GraphQueriesUnfoldedToExhaustionReload) {
    std::mt19937 rng(5);
    std::deque<pud_module> modules;
    modules.emplace_back(4001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("edge", {this->atom("a"), this->atom("b")}), {}, 0),
        rule(this->functor("edge", {this->atom("b"), this->atom("c")}), {}, 0),
        rule(this->functor("edge", {this->atom("c"), this->atom("d")}), {}, 0),
        rule(this->functor("edge", {this->atom("b"), this->atom("e")}), {}, 0),
        rule(this->functor("path", {this->var(0), this->var(1)}), {this->functor("edge", {this->var(0), this->var(1)})}, 2),
        rule(
            this->functor("path", {this->var(0), this->var(2)}),
            {this->functor("edge", {this->var(0), this->var(1)}), this->functor("path", {this->var(1), this->var(2)})},
            3),
    });
    modules.emplace_back(4002, std::vector<pud_module_id>{4001}, std::vector<rule>{
        rule(this->functor("reach", {this->var(0)}), {this->functor("path", {this->atom("a"), this->var(0)})}, 1),
        rule(this->atom("unreachable"), {this->functor("path", {this->atom("d"), this->atom("a")})}, 0),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < 2000u; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = 4002;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(0);
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    EXPECT_LT(taken, 2000u) << "the program did not run out of unfolds";

    EXPECT_GT(taken, 5u);
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, AGraphUnfoldedEverywhereReloads) {
    std::mt19937 rng(6);
    std::deque<pud_module> modules;
    modules.emplace_back(4001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("edge", {this->atom("a"), this->atom("b")}), {}, 0),
        rule(this->functor("edge", {this->atom("b"), this->atom("c")}), {}, 0),
        rule(this->functor("edge", {this->atom("c"), this->atom("d")}), {}, 0),
        rule(this->functor("edge", {this->atom("b"), this->atom("e")}), {}, 0),
        rule(this->functor("path", {this->var(0), this->var(1)}), {this->functor("edge", {this->var(0), this->var(1)})}, 2),
        rule(
            this->functor("path", {this->var(0), this->var(2)}),
            {this->functor("edge", {this->var(0), this->var(1)}), this->functor("path", {this->var(1), this->var(2)})},
            3),
    });
    modules.emplace_back(4002, std::vector<pud_module_id>{4001}, std::vector<rule>{
        rule(this->functor("reach", {this->var(0)}), {this->functor("path", {this->atom("a"), this->var(0)})}, 1),
        rule(this->atom("unreachable"), {this->functor("path", {this->atom("d"), this->atom("a")})}, 0),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < 40; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }

    EXPECT_GT(taken, 20u);
    size_t node_count = 0;
    for (const pud_module& module : modules)
        node_count += reference_pud_count_nodes(module);

    EXPECT_GT(node_count, 50u);
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, TiersUnfoldedOnlyInTheBottomModuleReload) {
    std::mt19937 rng(7);
    std::deque<pud_module> modules;
    modules.emplace_back(5001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("p", {this->atom("a")}), {}, 0),
        rule(this->functor("p", {this->atom("b")}), {}, 0),
    });
    modules.emplace_back(5002, std::vector<pud_module_id>{5001}, std::vector<rule>{
        rule(this->functor("r", {this->var(0)}), {this->functor("p", {this->var(0)})}, 1),
        rule(this->functor("r", {this->atom("c")}), {}, 0),
    });
    modules.emplace_back(5003, std::vector<pud_module_id>{5002}, std::vector<rule>{
        rule(this->functor("q", {this->var(0)}), {this->functor("r", {this->var(0)}), this->functor("p", {this->var(0)})}, 1),
        rule(this->functor("only_c", {this->var(0)}), {this->functor("r", {this->var(0)}), this->functor("s", {this->var(0)})}, 1),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < 10; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = 5002;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }

    EXPECT_GT(taken, 0u);
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, TiersUnfoldedOnlyInTheTopModuleReload) {
    std::mt19937 rng(8);
    std::deque<pud_module> modules;
    modules.emplace_back(5001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("p", {this->atom("a")}), {}, 0),
        rule(this->functor("p", {this->atom("b")}), {}, 0),
    });
    modules.emplace_back(5002, std::vector<pud_module_id>{5001}, std::vector<rule>{
        rule(this->functor("r", {this->var(0)}), {this->functor("p", {this->var(0)})}, 1),
        rule(this->functor("r", {this->atom("c")}), {}, 0),
    });
    modules.emplace_back(5003, std::vector<pud_module_id>{5002}, std::vector<rule>{
        rule(this->functor("q", {this->var(0)}), {this->functor("r", {this->var(0)}), this->functor("p", {this->var(0)})}, 1),
        rule(this->functor("only_c", {this->var(0)}), {this->functor("r", {this->var(0)}), this->functor("s", {this->var(0)})}, 1),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < 2000u; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = 5003;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(0);
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    EXPECT_LT(taken, 2000u) << "the program did not run out of unfolds";

    EXPECT_GT(taken, 1u);
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, TiersUnfoldedEverywhereReload) {
    std::mt19937 rng(9);
    std::deque<pud_module> modules;
    modules.emplace_back(5001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("p", {this->atom("a")}), {}, 0),
        rule(this->functor("p", {this->atom("b")}), {}, 0),
    });
    modules.emplace_back(5002, std::vector<pud_module_id>{5001}, std::vector<rule>{
        rule(this->functor("r", {this->var(0)}), {this->functor("p", {this->var(0)})}, 1),
        rule(this->functor("r", {this->atom("c")}), {}, 0),
    });
    modules.emplace_back(5003, std::vector<pud_module_id>{5002}, std::vector<rule>{
        rule(this->functor("q", {this->var(0)}), {this->functor("r", {this->var(0)}), this->functor("p", {this->var(0)})}, 1),
        rule(this->functor("only_c", {this->var(0)}), {this->functor("r", {this->var(0)}), this->functor("s", {this->var(0)})}, 1),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < 2000u; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(0);
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    EXPECT_LT(taken, 2000u) << "the program did not run out of unfolds";

    EXPECT_GT(taken, 2u);
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, SiblingModulesOverOneBaseReload) {
    std::mt19937 rng(10);
    std::deque<pud_module> modules;
    modules.emplace_back(6001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {this->atom("z")}), {}, 0),
        rule(this->functor("nat", {this->functor("s", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
    });
    modules.emplace_back(6002, std::vector<pud_module_id>{6001}, std::vector<rule>{
        rule(this->functor("even", {this->atom("z")}), {}, 0),
        rule(this->functor("even", {this->functor("s", {this->functor("s", {this->var(0)})})}), {this->functor("even", {this->var(0)})}, 1),
    });
    const expr* peano_1 = this->functor("s", {this->atom("z")});
    modules.emplace_back(6003, std::vector<pud_module_id>{6001}, std::vector<rule>{
        rule(this->functor("odd", {peano_1}), {}, 0),
        rule(this->functor("odd", {this->functor("s", {this->functor("s", {this->var(0)})})}), {this->functor("odd", {this->var(0)})}, 1),
    });
    modules.emplace_back(6004, std::vector<pud_module_id>{6002, 6003}, std::vector<rule>{
        rule(this->functor("even_nat", {this->var(0)}), {this->functor("even", {this->var(0)}), this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("odd_nat", {this->var(0)}), {this->functor("odd", {this->var(0)}), this->functor("nat", {this->var(0)})}, 1),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < 40; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }

    ASSERT_EQ(taken, 40u);
    size_t node_count = 0;
    for (const pud_module& module : modules)
        node_count += reference_pud_count_nodes(module);

    EXPECT_GT(node_count, 50u);
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, CascadingRefutationsReload) {
    std::mt19937 rng(11);
    std::deque<pud_module> modules;
    modules.emplace_back(7001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("a", {this->atom("one")}), {}, 0),
        rule(this->functor("a", {this->atom("two")}), {}, 0),
        rule(this->functor("b", {this->atom("two")}), {}, 0),
        rule(this->functor("d", {this->atom("one")}), {}, 0),
        rule(this->functor("c", {this->var(0)}), {this->functor("a", {this->var(0)}), this->functor("b", {this->var(0)}), this->functor("d", {this->var(0)})}, 1),
        rule(this->functor("e", {this->var(0)}), {this->functor("c", {this->var(0)})}, 1),
        rule(this->atom("f"), {this->atom("missing")}, 0),
        rule(this->functor("g", {this->var(0)}), {this->functor("a", {this->var(0)}), this->functor("missing", {this->var(0)})}, 1),
        rule(this->functor("h", {this->var(0)}), {this->functor("a", {this->var(0)}), this->functor("d", {this->var(0)})}, 1),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < 2000u; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(0);
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    EXPECT_LT(taken, 2000u) << "the program did not run out of unfolds";

    EXPECT_GT(taken, 2u);
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, NothingBelowARefutedInteriorNodeComesBackOnReload) {
    std::deque<pud_module> modules;
    modules.emplace_back(8001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("base", {this->atom("c")}), {}, 0),
        rule(this->functor("check", {this->atom("a")}), {}, 0),
        rule(this->functor("mid", {this->var(0)}), {this->functor("base", {this->var(0)}), this->functor("check", {this->var(0)})}, 1),
    });
    modules.emplace_back(8002, std::vector<pud_module_id>{8001}, std::vector<rule>{
        rule(this->atom("top"), {this->functor("mid", {this->atom("c")})}, 0),
        rule(this->atom("other"), {this->functor("check", {this->atom("a")})}, 0),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }

    const pud_frontier_change unfold_change = runtime.unfold(pud_global_node_id{.module = 8001, .node = 2}, 0);
    live.erase(pud_global_node_id{.module = 8001, .node = 2});
    live.insert(unfold_change.offspring.begin(), unfold_change.offspring.end());
    for (pud_global_node_id refuted : unfold_change.refuted)
        live.erase(refuted);
    const pud_module* base = nullptr;
    for (const pud_module& module : modules) {
        if (module.get_module_id() == 8001)
            base = &module;
    }
    ASSERT_TRUE(base->check_refuted(2));
    ASSERT_THAT(base->get_children(2), Not(IsEmpty()));
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());

    for (pud_node_id child : base->get_children(2))
        EXPECT_FALSE(reloaded_live.contains(pud_global_node_id{.module = 8001, .node = child}));

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, ARefutationThatCrossedModulesStaysOnReload) {
    std::deque<pud_module> modules;
    modules.emplace_back(8001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("base", {this->atom("c")}), {}, 0),
        rule(this->functor("check", {this->atom("a")}), {}, 0),
        rule(this->functor("mid", {this->var(0)}), {this->functor("base", {this->var(0)}), this->functor("check", {this->var(0)})}, 1),
    });
    modules.emplace_back(8002, std::vector<pud_module_id>{8001}, std::vector<rule>{
        rule(this->atom("top"), {this->functor("mid", {this->atom("c")})}, 0),
        rule(this->atom("other"), {this->functor("check", {this->atom("a")})}, 0),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }

    const pud_frontier_change unfold_change = runtime.unfold(pud_global_node_id{.module = 8001, .node = 2}, 0);
    live.erase(pud_global_node_id{.module = 8001, .node = 2});
    live.insert(unfold_change.offspring.begin(), unfold_change.offspring.end());
    for (pud_global_node_id refuted : unfold_change.refuted)
        live.erase(refuted);
    const pud_module* dep_module = nullptr;
    for (const pud_module& module : modules) {
        if (module.get_module_id() == 8002)
            dep_module = &module;
    }
    ASSERT_TRUE(dep_module->check_refuted(0));
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());

    EXPECT_FALSE(reloaded_live.contains(pud_global_node_id{.module = 8002, .node = 0}));
    EXPECT_TRUE(reloaded_live.contains(pud_global_node_id{.module = 8002, .node = 1}));

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, AReloadOfAReloadIsFaithfulToo) {
    std::mt19937 rng(12);
    std::deque<pud_module> modules;
    modules.emplace_back(3001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("app", {this->atom("nil"), this->var(0), this->var(0)}), {}, 1),
        rule(
            this->functor("app", {this->functor("cons", {this->var(0), this->var(1)}), this->var(2), this->functor("cons", {this->var(0), this->var(3)})}),
            {this->functor("app", {this->var(1), this->var(2), this->var(3)})},
            4),
    });
    const expr* list_abc = this->atom("nil");
    list_abc = this->functor("cons", {this->atom("c"), list_abc});
    list_abc = this->functor("cons", {this->atom("b"), list_abc});
    list_abc = this->functor("cons", {this->atom("a"), list_abc});
    const expr* list_a = this->functor("cons", {this->atom("a"), this->atom("nil")});
    const expr* list_ab = this->functor("cons", {this->atom("a"), this->functor("cons", {this->atom("b"), this->atom("nil")})});
    modules.emplace_back(3002, std::vector<pud_module_id>{3001}, std::vector<rule>{
        rule(
            this->functor("split", {this->var(0), this->var(1)}),
            {this->functor("app", {this->var(0), this->var(1), list_abc})},
            2),
        rule(
            this->functor("suffix", {this->var(0)}),
            {this->functor("app", {list_a, this->var(0), list_ab})},
            1),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < 25; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());

    std::deque<pud_module> second_modules(reloaded_modules.begin(), reloaded_modules.end());
    TypeParam second_runtime(this->exprs);
    std::set<pud_global_node_id> second_live;
    std::vector<pud_global_node_id> refuted_on_second_reload;
    for (pud_module& copy : second_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = second_runtime.register_module(copy);
        refuted_on_second_reload.insert(refuted_on_second_reload.end(), change.refuted.begin(), change.refuted.end());
        second_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            second_live.erase(refuted);
    }
    EXPECT_EQ(second_live, live);
    EXPECT_EQ(second_live, reference_pud_frontier(second_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(second_modules), IsEmpty());
    EXPECT_THAT(refuted_on_second_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(reloaded_modules, second_modules), IsEmpty());
    EXPECT_EQ(second_live, live);
    EXPECT_THAT(reference_pud_module_differences(modules, second_modules), IsEmpty());

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, ReloadingTheSameModulesManyTimesNeverChangesThem) {
    std::mt19937 rng(13);
    std::deque<pud_module> modules;
    modules.emplace_back(4001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("edge", {this->atom("a"), this->atom("b")}), {}, 0),
        rule(this->functor("edge", {this->atom("b"), this->atom("c")}), {}, 0),
        rule(this->functor("edge", {this->atom("c"), this->atom("d")}), {}, 0),
        rule(this->functor("edge", {this->atom("b"), this->atom("e")}), {}, 0),
        rule(this->functor("path", {this->var(0), this->var(1)}), {this->functor("edge", {this->var(0), this->var(1)})}, 2),
        rule(
            this->functor("path", {this->var(0), this->var(2)}),
            {this->functor("edge", {this->var(0), this->var(1)}), this->functor("path", {this->var(1), this->var(2)})},
            3),
    });
    modules.emplace_back(4002, std::vector<pud_module_id>{4001}, std::vector<rule>{
        rule(this->functor("reach", {this->var(0)}), {this->functor("path", {this->atom("a"), this->var(0)})}, 1),
        rule(this->atom("unreachable"), {this->functor("path", {this->atom("d"), this->atom("a")})}, 0),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < 30; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }

    for (int reload_index = 0; reload_index < 6; ++reload_index) {
        std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
        TypeParam reloaded_runtime(this->exprs);
        std::set<pud_global_node_id> reloaded_live;
        std::vector<pud_global_node_id> refuted_on_reload;
        for (pud_module& copy : reloaded_modules) {
                install_shared_functor_names(copy, this->functors);
                const pud_frontier_change change = reloaded_runtime.register_module(copy);
                refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
                reloaded_live.insert(change.offspring.begin(), change.offspring.end());
                for (pud_global_node_id refuted : change.refuted)
                        reloaded_live.erase(refuted);
        }
        EXPECT_EQ(reloaded_live, live);
        EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
        EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
        EXPECT_THAT(refuted_on_reload, IsEmpty());
        EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());

    }

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, ModuleIdsAreOnlyNames) {
    std::mt19937 rng(14);
    const pud_module_id large = std::numeric_limits<pud_module_id>::max() - 3;
    std::deque<pud_module> modules;
    modules.emplace_back(large, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {this->atom("z")}), {}, 0),
        rule(this->functor("nat", {this->functor("s", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
    });
    modules.emplace_back(0, std::vector<pud_module_id>{large}, std::vector<rule>{
        rule(this->functor("number", {this->var(0)}), {this->functor("nat", {this->var(0)})}, 1),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < 20; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, PeanoMulWithManyRandomUnfoldsReloads) {
    std::mt19937 rng(60);
    std::deque<pud_module> modules;
    modules.emplace_back(9001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {this->atom("z")}), {}, 0),
        rule(this->functor("nat", {this->functor("s", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("add", {this->atom("z"), this->var(0), this->var(0)}), {}, 1),
        rule(
            this->functor("add", {this->functor("s", {this->var(0)}), this->var(1), this->functor("s", {this->var(2)})}),
            {this->functor("add", {this->var(0), this->var(1), this->var(2)})},
            3),
        rule(this->functor("mul", {this->atom("z"), this->var(0), this->atom("z")}), {}, 1),
        rule(
            this->functor("mul", {this->functor("s", {this->var(0)}), this->var(1), this->var(2)}),
            {this->functor("mul", {this->var(0), this->var(1), this->var(3)}), this->functor("add", {this->var(3), this->var(1), this->var(2)})},
            4),
    });
    const expr* peano_6 = this->atom("z");
    for (uint32_t step = 0; step < 6; ++step)
        peano_6 = this->functor("s", {peano_6});
    const expr* peano_3 = this->functor("s", {this->functor("s", {this->functor("s", {this->atom("z")})})});
    const expr* peano_9 = this->atom("z");
    for (uint32_t step = 0; step < 9; ++step)
        peano_9 = this->functor("s", {peano_9});
    const expr* peano_2 = this->functor("s", {this->functor("s", {this->atom("z")})});
    modules.emplace_back(9002, std::vector<pud_module_id>{9001}, std::vector<rule>{
        rule(this->functor("factor", {this->var(0), this->var(1)}), {this->functor("mul", {this->var(0), this->var(1), peano_6})}, 2),
        rule(this->functor("triple", {this->var(0)}), {this->functor("mul", {peano_3, this->var(0), peano_9})}, 1),
        rule(
            this->functor("add_mul", {this->var(0), this->var(1), this->var(2)}),
            {this->functor("add", {this->var(0), this->var(1), this->var(2)}), this->functor("mul", {this->var(0), peano_2, this->var(3)})},
            4),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < 70; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }

    ASSERT_EQ(taken, 70u);
    size_t node_count = 0;
    for (const pud_module& module : modules)
        node_count += reference_pud_count_nodes(module);

    EXPECT_GT(node_count, 120u);
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, ListMembersUnfoldedToExhaustionReload) {
    std::mt19937 rng(61);
    std::deque<pud_module> modules;
    modules.emplace_back(9101, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("mem", {this->var(0), this->functor("cons", {this->var(0), this->var(1)})}), {}, 2),
        rule(
            this->functor("mem", {this->var(0), this->functor("cons", {this->var(1), this->var(2)})}),
            {this->functor("mem", {this->var(0), this->var(2)})},
            3),
    });
    const expr* list_xyz = this->atom("nil");
    list_xyz = this->functor("cons", {this->atom("z"), list_xyz});
    list_xyz = this->functor("cons", {this->atom("y"), list_xyz});
    list_xyz = this->functor("cons", {this->atom("x"), list_xyz});
    const expr* list_abcd = this->atom("nil");
    list_abcd = this->functor("cons", {this->atom("d"), list_abcd});
    list_abcd = this->functor("cons", {this->atom("c"), list_abcd});
    list_abcd = this->functor("cons", {this->atom("b"), list_abcd});
    list_abcd = this->functor("cons", {this->atom("a"), list_abcd});
    const expr* list_bcd = this->functor("cons", {this->atom("b"), this->functor("cons", {this->atom("c"), this->functor("cons", {this->atom("d"), this->atom("nil")})})});
    modules.emplace_back(9102, std::vector<pud_module_id>{9101}, std::vector<rule>{
        rule(
            this->functor("every", {this->var(0)}),
            {this->functor("mem", {this->var(0), list_xyz})},
            1),
        rule(
            this->functor("pair_mem", {this->var(0), this->var(1)}),
            {
                this->functor("mem", {this->var(0), list_abcd}),
                this->functor("mem", {this->var(1), list_bcd}),
            },
            2),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < 2000u; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = 9102;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(0);
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    EXPECT_LT(taken, 2000u) << "the program did not run out of unfolds";

    EXPECT_GT(taken, 5u);
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, MirrorTreesWithDeepStructureReload) {
    std::mt19937 rng(62);
    std::deque<pud_module> modules;
    const expr* sample =
        this->functor("branch", {this->functor("leaf", {this->atom("l")}), this->functor("branch", {this->functor("leaf", {this->atom("m")}), this->functor("leaf", {this->atom("r")})})});
    modules.emplace_back(9201, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("mirror", {this->functor("leaf", {this->var(0)}), this->functor("leaf", {this->var(0)})}), {}, 1),
        rule(
            this->functor("mirror", {this->functor("branch", {this->var(0), this->var(1)}), this->functor("branch", {this->var(2), this->var(3)})}),
            {this->functor("mirror", {this->var(0), this->var(3)}), this->functor("mirror", {this->var(1), this->var(2)})},
            4),
    });
    modules.emplace_back(9202, std::vector<pud_module_id>{9201}, std::vector<rule>{
        rule(this->functor("flip", {this->var(0)}), {this->functor("mirror", {sample, this->var(0)})}, 1),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < 2000u; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = 9202;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(0);
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    EXPECT_LT(taken, 2000u) << "the program did not run out of unfolds";

    EXPECT_GT(taken, 2u);
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, NestedUnaryChainUnfoldsDeeplyThenReloads) {
    std::deque<pud_module> modules;
    const expr* nested_s8 = this->atom("z");
    for (uint32_t level = 0; level < 8; ++level)
        nested_s8 = this->functor("s", {nested_s8});
    const expr* nested_s3 = this->atom("z");
    for (uint32_t level = 0; level < 3; ++level)
        nested_s3 = this->functor("s", {nested_s3});
    const expr* nested_s2 = this->functor("s", {this->functor("s", {nested_s3})});
    const expr* peano_2 = this->functor("s", {this->functor("s", {this->atom("z")})});
    modules.emplace_back(9301, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {this->atom("z")}), {}, 0),
        rule(this->functor("nat", {this->functor("s", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(
            this->functor("deep_nat", {nested_s8}),
            {this->functor("nat", {nested_s8})},
            0),
        rule(
            this->functor("shallow", {this->var(0)}),
            {this->functor("nat", {this->var(0)}), this->functor("add", {this->var(0), peano_2, nested_s2})},
            1),
        rule(this->functor("add", {this->atom("z"), this->var(0), this->var(0)}), {}, 1),
        rule(
            this->functor("add", {this->functor("s", {this->var(0)}), this->var(1), this->functor("s", {this->var(2)})}),
            {this->functor("add", {this->var(0), this->var(1), this->var(2)})},
            3),
    });
    const expr* nested_s5 = this->atom("z");
    for (uint32_t level = 0; level < 5; ++level)
        nested_s5 = this->functor("s", {nested_s5});
    modules.emplace_back(9302, std::vector<pud_module_id>{9301}, std::vector<rule>{
        rule(
            this->functor("reach", {this->var(0)}),
            {this->functor("nat", {nested_s5}), this->functor("add", {this->var(0), this->functor("s", {this->atom("z")}), nested_s5})},
            1),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }

    const pud_global_node_id deep_axiom{.module = 9301, .node = 1};
    const pud_frontier_change deep_change = runtime.unfold(deep_axiom, 0);
    live.erase(deep_axiom);
    live.insert(deep_change.offspring.begin(), deep_change.offspring.end());
    for (pud_global_node_id refuted : deep_change.refuted)
        live.erase(refuted);
    ASSERT_FALSE(deep_change.offspring.empty());
    std::mt19937 rng(0);
    for (int level = 0; level < 10; ++level) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 9301)
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(0);
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t node_count = 0;
    for (const pud_module& module : modules)
        node_count += reference_pud_count_nodes(module);

    EXPECT_GT(node_count, 15u);
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, TripleAppendStressReloads) {
    std::mt19937 rng(63);
    std::deque<pud_module> modules;
    modules.emplace_back(9401, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("app", {this->atom("nil"), this->var(0), this->var(0)}), {}, 1),
        rule(
            this->functor("app", {this->functor("cons", {this->var(0), this->var(1)}), this->var(2), this->functor("cons", {this->var(0), this->var(3)})}),
            {this->functor("app", {this->var(1), this->var(2), this->var(3)})},
            4),
    });
    const expr* list_abc = this->atom("nil");
    list_abc = this->functor("cons", {this->atom("c"), list_abc});
    list_abc = this->functor("cons", {this->atom("b"), list_abc});
    list_abc = this->functor("cons", {this->atom("a"), list_abc});
    const expr* list_de = this->functor("cons", {this->atom("d"), this->functor("cons", {this->atom("e"), this->atom("nil")})});
    const expr* list_pq = this->functor("cons", {this->atom("p"), this->functor("cons", {this->atom("q"), this->atom("nil")})});
    const expr* list_r = this->functor("cons", {this->atom("r"), this->atom("nil")});
    const expr* list_pqr = this->functor("cons", {this->atom("p"), this->functor("cons", {this->atom("q"), this->functor("cons", {this->atom("r"), this->atom("nil")})})});
    modules.emplace_back(9402, std::vector<pud_module_id>{9401}, std::vector<rule>{
        rule(
            this->atom("triple_split"),
            {
                this->functor("app", {this->var(0), this->var(1), list_abc}),
                this->functor("app", {this->var(1), this->var(2), list_de}),
                this->functor("app", {this->var(0), this->var(2), this->var(3)}),
            },
            4),
        rule(
            this->functor("nested_app", {this->var(0)}),
            {
                this->functor("app", {this->var(0), this->var(1), list_pq}),
                this->functor("app", {this->var(1), list_r, list_pqr}),
            },
            2),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < 55; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }

    ASSERT_EQ(taken, 55u);
    size_t node_count = 0;
    for (const pud_module& module : modules)
        node_count += reference_pud_count_nodes(module);

    EXPECT_GT(node_count, 80u);
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, SharedBinaryNestedTermsReload) {
    std::mt19937 rng(64);
    std::deque<pud_module> modules;
    const expr* shared = this->atom("v");
    for (int level = 0; level < 4; ++level)
        shared = this->functor("pair", {shared, shared});
    modules.emplace_back(9501, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {this->atom("z")}), {}, 0),
        rule(this->functor("nat", {this->functor("s", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("unpack", {shared, this->var(0)}), {this->functor("same", {this->var(0), shared})}, 1),
        rule(this->functor("same", {this->var(0), this->var(0)}), {}, 1),
        rule(
            this->functor("both", {this->var(0), this->var(1)}),
            {this->functor("same", {this->var(0), this->var(1)}), this->functor("nat", {this->var(0)})},
            2),
    });
    const expr* peano_2 = this->functor("s", {this->functor("s", {this->atom("z")})});
    modules.emplace_back(9502, std::vector<pud_module_id>{9501}, std::vector<rule>{
        rule(this->functor("grounded", {this->var(0)}), {this->functor("unpack", {shared, this->var(0)})}, 1),
        rule(this->functor("nat_query", {this->var(0)}), {this->functor("both", {peano_2, this->var(0)})}, 1),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < 40; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }

    ASSERT_EQ(taken, 40u);
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, PeanoMulReloadsAfterHeavyRandomUnfolds) {
    for (uint32_t seed = 0; seed < 4 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(300 + seed);
    std::deque<pud_module> modules;
    modules.emplace_back(9001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {this->atom("z")}), {}, 0),
        rule(this->functor("nat", {this->functor("s", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("add", {this->atom("z"), this->var(0), this->var(0)}), {}, 1),
        rule(
            this->functor("add", {this->functor("s", {this->var(0)}), this->var(1), this->functor("s", {this->var(2)})}),
            {this->functor("add", {this->var(0), this->var(1), this->var(2)})},
            3),
        rule(this->functor("mul", {this->atom("z"), this->var(0), this->atom("z")}), {}, 1),
        rule(
            this->functor("mul", {this->functor("s", {this->var(0)}), this->var(1), this->var(2)}),
            {this->functor("mul", {this->var(0), this->var(1), this->var(3)}), this->functor("add", {this->var(3), this->var(1), this->var(2)})},
            4),
    });
    const expr* peano_6 = this->atom("z");
    for (uint32_t step = 0; step < 6; ++step)
        peano_6 = this->functor("s", {peano_6});
    const expr* peano_3 = this->functor("s", {this->functor("s", {this->functor("s", {this->atom("z")})})});
    const expr* peano_9 = this->atom("z");
    for (uint32_t step = 0; step < 9; ++step)
        peano_9 = this->functor("s", {peano_9});
    const expr* peano_2 = this->functor("s", {this->functor("s", {this->atom("z")})});
    modules.emplace_back(9002, std::vector<pud_module_id>{9001}, std::vector<rule>{
        rule(this->functor("factor", {this->var(0), this->var(1)}), {this->functor("mul", {this->var(0), this->var(1), peano_6})}, 2),
        rule(this->functor("triple", {this->var(0)}), {this->functor("mul", {peano_3, this->var(0), peano_9})}, 1),
        rule(
            this->functor("add_mul", {this->var(0), this->var(1), this->var(2)}),
            {this->functor("add", {this->var(0), this->var(1), this->var(2)}), this->functor("mul", {this->var(0), peano_2, this->var(3)})},
            4),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }

        const size_t budget = 40 + rng() % 40;
    size_t taken = 0;
    for (; taken < budget; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());

        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 9001;
            return;
        }
    }

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, ListMembersReloadsAfterHeavyRandomUnfolds) {
    for (uint32_t seed = 0; seed < 4 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(300 + seed);
    std::deque<pud_module> modules;
    modules.emplace_back(9101, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("mem", {this->var(0), this->functor("cons", {this->var(0), this->var(1)})}), {}, 2),
        rule(
            this->functor("mem", {this->var(0), this->functor("cons", {this->var(1), this->var(2)})}),
            {this->functor("mem", {this->var(0), this->var(2)})},
            3),
    });
    const expr* list_xyz = this->atom("nil");
    list_xyz = this->functor("cons", {this->atom("z"), list_xyz});
    list_xyz = this->functor("cons", {this->atom("y"), list_xyz});
    list_xyz = this->functor("cons", {this->atom("x"), list_xyz});
    const expr* list_abcd = this->atom("nil");
    list_abcd = this->functor("cons", {this->atom("d"), list_abcd});
    list_abcd = this->functor("cons", {this->atom("c"), list_abcd});
    list_abcd = this->functor("cons", {this->atom("b"), list_abcd});
    list_abcd = this->functor("cons", {this->atom("a"), list_abcd});
    const expr* list_bcd = this->functor("cons", {this->atom("b"), this->functor("cons", {this->atom("c"), this->functor("cons", {this->atom("d"), this->atom("nil")})})});
    modules.emplace_back(9102, std::vector<pud_module_id>{9101}, std::vector<rule>{
        rule(
            this->functor("every", {this->var(0)}),
            {this->functor("mem", {this->var(0), list_xyz})},
            1),
        rule(
            this->functor("pair_mem", {this->var(0), this->var(1)}),
            {
                this->functor("mem", {this->var(0), list_abcd}),
                this->functor("mem", {this->var(1), list_bcd}),
            },
            2),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }

        const size_t budget = 40 + rng() % 40;
    size_t taken = 0;
    for (; taken < budget; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());

        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 9101;
            return;
        }
    }

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, MirrorTreesReloadsAfterHeavyRandomUnfolds) {
    for (uint32_t seed = 0; seed < 4 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(300 + seed);
    std::deque<pud_module> modules;
    const expr* sample =
        this->functor("branch", {this->functor("leaf", {this->atom("l")}), this->functor("branch", {this->functor("leaf", {this->atom("m")}), this->functor("leaf", {this->atom("r")})})});
    modules.emplace_back(9201, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("mirror", {this->functor("leaf", {this->var(0)}), this->functor("leaf", {this->var(0)})}), {}, 1),
        rule(
            this->functor("mirror", {this->functor("branch", {this->var(0), this->var(1)}), this->functor("branch", {this->var(2), this->var(3)})}),
            {this->functor("mirror", {this->var(0), this->var(3)}), this->functor("mirror", {this->var(1), this->var(2)})},
            4),
    });
    modules.emplace_back(9202, std::vector<pud_module_id>{9201}, std::vector<rule>{
        rule(this->functor("flip", {this->var(0)}), {this->functor("mirror", {sample, this->var(0)})}, 1),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }

        const size_t budget = 40 + rng() % 40;
    size_t taken = 0;
    for (; taken < budget; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());

        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 9201;
            return;
        }
    }

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, NestedUnaryChainReloadsAfterHeavyRandomUnfolds) {
    for (uint32_t seed = 0; seed < 4 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(300 + seed);
    std::deque<pud_module> modules;
    const expr* nested_s8 = this->atom("z");
    for (uint32_t level = 0; level < 8; ++level)
        nested_s8 = this->functor("s", {nested_s8});
    const expr* nested_s3 = this->atom("z");
    for (uint32_t level = 0; level < 3; ++level)
        nested_s3 = this->functor("s", {nested_s3});
    const expr* nested_s2 = this->functor("s", {this->functor("s", {nested_s3})});
    const expr* peano_2 = this->functor("s", {this->functor("s", {this->atom("z")})});
    modules.emplace_back(9301, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {this->atom("z")}), {}, 0),
        rule(this->functor("nat", {this->functor("s", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(
            this->functor("deep_nat", {nested_s8}),
            {this->functor("nat", {nested_s8})},
            0),
        rule(
            this->functor("shallow", {this->var(0)}),
            {this->functor("nat", {this->var(0)}), this->functor("add", {this->var(0), peano_2, nested_s2})},
            1),
        rule(this->functor("add", {this->atom("z"), this->var(0), this->var(0)}), {}, 1),
        rule(
            this->functor("add", {this->functor("s", {this->var(0)}), this->var(1), this->functor("s", {this->var(2)})}),
            {this->functor("add", {this->var(0), this->var(1), this->var(2)})},
            3),
    });
    const expr* nested_s5 = this->atom("z");
    for (uint32_t level = 0; level < 5; ++level)
        nested_s5 = this->functor("s", {nested_s5});
    modules.emplace_back(9302, std::vector<pud_module_id>{9301}, std::vector<rule>{
        rule(
            this->functor("reach", {this->var(0)}),
            {this->functor("nat", {nested_s5}), this->functor("add", {this->var(0), this->functor("s", {this->atom("z")}), nested_s5})},
            1),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }

        const size_t budget = 40 + rng() % 40;
    size_t taken = 0;
    for (; taken < budget; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());

        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 9301;
            return;
        }
    }

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, TripleAppendReloadsAfterHeavyRandomUnfolds) {
    for (uint32_t seed = 0; seed < 4 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(300 + seed);
    std::deque<pud_module> modules;
    modules.emplace_back(9401, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("app", {this->atom("nil"), this->var(0), this->var(0)}), {}, 1),
        rule(
            this->functor("app", {this->functor("cons", {this->var(0), this->var(1)}), this->var(2), this->functor("cons", {this->var(0), this->var(3)})}),
            {this->functor("app", {this->var(1), this->var(2), this->var(3)})},
            4),
    });
    const expr* list_abc = this->atom("nil");
    list_abc = this->functor("cons", {this->atom("c"), list_abc});
    list_abc = this->functor("cons", {this->atom("b"), list_abc});
    list_abc = this->functor("cons", {this->atom("a"), list_abc});
    const expr* list_de = this->functor("cons", {this->atom("d"), this->functor("cons", {this->atom("e"), this->atom("nil")})});
    const expr* list_pq = this->functor("cons", {this->atom("p"), this->functor("cons", {this->atom("q"), this->atom("nil")})});
    const expr* list_r = this->functor("cons", {this->atom("r"), this->atom("nil")});
    const expr* list_pqr = this->functor("cons", {this->atom("p"), this->functor("cons", {this->atom("q"), this->functor("cons", {this->atom("r"), this->atom("nil")})})});
    modules.emplace_back(9402, std::vector<pud_module_id>{9401}, std::vector<rule>{
        rule(
            this->atom("triple_split"),
            {
                this->functor("app", {this->var(0), this->var(1), list_abc}),
                this->functor("app", {this->var(1), this->var(2), list_de}),
                this->functor("app", {this->var(0), this->var(2), this->var(3)}),
            },
            4),
        rule(
            this->functor("nested_app", {this->var(0)}),
            {
                this->functor("app", {this->var(0), this->var(1), list_pq}),
                this->functor("app", {this->var(1), list_r, list_pqr}),
            },
            2),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }

        const size_t budget = 40 + rng() % 40;
    size_t taken = 0;
    for (; taken < budget; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());

        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 9401;
            return;
        }
    }

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, SharedBinaryReloadsAfterHeavyRandomUnfolds) {
    for (uint32_t seed = 0; seed < 4 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(300 + seed);
    std::deque<pud_module> modules;
    const expr* shared = this->atom("v");
    for (int level = 0; level < 4; ++level)
        shared = this->functor("pair", {shared, shared});
    modules.emplace_back(9501, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {this->atom("z")}), {}, 0),
        rule(this->functor("nat", {this->functor("s", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("unpack", {shared, this->var(0)}), {this->functor("same", {this->var(0), shared})}, 1),
        rule(this->functor("same", {this->var(0), this->var(0)}), {}, 1),
        rule(
            this->functor("both", {this->var(0), this->var(1)}),
            {this->functor("same", {this->var(0), this->var(1)}), this->functor("nat", {this->var(0)})},
            2),
    });
    const expr* peano_2 = this->functor("s", {this->functor("s", {this->atom("z")})});
    modules.emplace_back(9502, std::vector<pud_module_id>{9501}, std::vector<rule>{
        rule(this->functor("grounded", {this->var(0)}), {this->functor("unpack", {shared, this->var(0)})}, 1),
        rule(this->functor("nat_query", {this->var(0)}), {this->functor("both", {peano_2, this->var(0)})}, 1),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }

        const size_t budget = 40 + rng() % 40;
    size_t taken = 0;
    for (; taken < budget; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());

        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 9501;
            return;
        }
    }

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, ColorsReloadsFaithfullyAfterAnyNumberOfRandomUnfolds) {
    for (uint32_t seed = 0; seed < 6 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(seed);
    std::deque<pud_module> modules;
    modules.emplace_back(1001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("color", {this->atom("red")}), {}, 0),
        rule(this->functor("color", {this->atom("blue")}), {}, 0),
        rule(this->functor("color", {this->atom("green")}), {}, 0),
        rule(this->functor("warm", {this->atom("red")}), {}, 0),
        rule(this->functor("warm", {this->atom("orange")}), {}, 0),
        rule(
            this->functor("pair", {this->var(0), this->var(1)}),
            {this->functor("color", {this->var(0)}), this->functor("color", {this->var(1)})},
            2),
        rule(
            this->functor("warm_color", {this->var(0)}),
            {this->functor("color", {this->var(0)}), this->functor("warm", {this->var(0)})},
            1),
        rule(this->atom("impossible"), {this->functor("color", {this->atom("purple")})}, 0),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < rng() % 30; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());

        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 1001;
            return;
        }
    }

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, ArithmeticReloadsFaithfullyAfterAnyNumberOfRandomUnfolds) {
    for (uint32_t seed = 0; seed < 6 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(seed);
    std::deque<pud_module> modules;
    modules.emplace_back(2001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {this->atom("z")}), {}, 0),
        rule(this->functor("nat", {this->functor("s", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("add", {this->atom("z"), this->var(0), this->var(0)}), {}, 1),
        rule(
            this->functor("add", {this->functor("s", {this->var(0)}), this->var(1), this->functor("s", {this->var(2)})}),
            {this->functor("add", {this->var(0), this->var(1), this->var(2)})},
            3),
    });
    modules.emplace_back(2002, std::vector<pud_module_id>{2001}, std::vector<rule>{
        rule(
            this->functor("half", {this->var(0)}),
            {this->functor("add", {this->var(0), this->var(0),
                this->functor("s", {this->functor("s", {this->functor("s", {this->functor("s", {this->atom("z")})})})})})},
            1),
        rule(this->functor("number", {this->var(0)}), {this->functor("nat", {this->var(0)})}, 1),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < rng() % 30; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());

        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 2001;
            return;
        }
    }

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, ListsReloadsFaithfullyAfterAnyNumberOfRandomUnfolds) {
    for (uint32_t seed = 0; seed < 6 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(seed);
    std::deque<pud_module> modules;
    modules.emplace_back(3001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("app", {this->atom("nil"), this->var(0), this->var(0)}), {}, 1),
        rule(
            this->functor("app", {this->functor("cons", {this->var(0), this->var(1)}), this->var(2), this->functor("cons", {this->var(0), this->var(3)})}),
            {this->functor("app", {this->var(1), this->var(2), this->var(3)})},
            4),
    });
    const expr* list_abc = this->atom("nil");
    list_abc = this->functor("cons", {this->atom("c"), list_abc});
    list_abc = this->functor("cons", {this->atom("b"), list_abc});
    list_abc = this->functor("cons", {this->atom("a"), list_abc});
    const expr* list_a = this->functor("cons", {this->atom("a"), this->atom("nil")});
    const expr* list_ab = this->functor("cons", {this->atom("a"), this->functor("cons", {this->atom("b"), this->atom("nil")})});
    modules.emplace_back(3002, std::vector<pud_module_id>{3001}, std::vector<rule>{
        rule(
            this->functor("split", {this->var(0), this->var(1)}),
            {this->functor("app", {this->var(0), this->var(1), list_abc})},
            2),
        rule(
            this->functor("suffix", {this->var(0)}),
            {this->functor("app", {list_a, this->var(0), list_ab})},
            1),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < rng() % 30; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());

        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 3001;
            return;
        }
    }

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, GraphReloadsFaithfullyAfterAnyNumberOfRandomUnfolds) {
    for (uint32_t seed = 0; seed < 6 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(seed);
    std::deque<pud_module> modules;
    modules.emplace_back(4001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("edge", {this->atom("a"), this->atom("b")}), {}, 0),
        rule(this->functor("edge", {this->atom("b"), this->atom("c")}), {}, 0),
        rule(this->functor("edge", {this->atom("c"), this->atom("d")}), {}, 0),
        rule(this->functor("edge", {this->atom("b"), this->atom("e")}), {}, 0),
        rule(this->functor("path", {this->var(0), this->var(1)}), {this->functor("edge", {this->var(0), this->var(1)})}, 2),
        rule(
            this->functor("path", {this->var(0), this->var(2)}),
            {this->functor("edge", {this->var(0), this->var(1)}), this->functor("path", {this->var(1), this->var(2)})},
            3),
    });
    modules.emplace_back(4002, std::vector<pud_module_id>{4001}, std::vector<rule>{
        rule(this->functor("reach", {this->var(0)}), {this->functor("path", {this->atom("a"), this->var(0)})}, 1),
        rule(this->atom("unreachable"), {this->functor("path", {this->atom("d"), this->atom("a")})}, 0),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < rng() % 30; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());

        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 4001;
            return;
        }
    }

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, TiersReloadsFaithfullyAfterAnyNumberOfRandomUnfolds) {
    for (uint32_t seed = 0; seed < 6 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(seed);
    std::deque<pud_module> modules;
    modules.emplace_back(5001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("p", {this->atom("a")}), {}, 0),
        rule(this->functor("p", {this->atom("b")}), {}, 0),
    });
    modules.emplace_back(5002, std::vector<pud_module_id>{5001}, std::vector<rule>{
        rule(this->functor("r", {this->var(0)}), {this->functor("p", {this->var(0)})}, 1),
        rule(this->functor("r", {this->atom("c")}), {}, 0),
    });
    modules.emplace_back(5003, std::vector<pud_module_id>{5002}, std::vector<rule>{
        rule(this->functor("q", {this->var(0)}), {this->functor("r", {this->var(0)}), this->functor("p", {this->var(0)})}, 1),
        rule(this->functor("only_c", {this->var(0)}), {this->functor("r", {this->var(0)}), this->functor("s", {this->var(0)})}, 1),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < rng() % 30; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());

        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 5001;
            return;
        }
    }

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, SiblingsReloadsFaithfullyAfterAnyNumberOfRandomUnfolds) {
    for (uint32_t seed = 0; seed < 6 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(seed);
    std::deque<pud_module> modules;
    modules.emplace_back(6001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {this->atom("z")}), {}, 0),
        rule(this->functor("nat", {this->functor("s", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
    });
    modules.emplace_back(6002, std::vector<pud_module_id>{6001}, std::vector<rule>{
        rule(this->functor("even", {this->atom("z")}), {}, 0),
        rule(this->functor("even", {this->functor("s", {this->functor("s", {this->var(0)})})}), {this->functor("even", {this->var(0)})}, 1),
    });
    const expr* peano_1 = this->functor("s", {this->atom("z")});
    modules.emplace_back(6003, std::vector<pud_module_id>{6001}, std::vector<rule>{
        rule(this->functor("odd", {peano_1}), {}, 0),
        rule(this->functor("odd", {this->functor("s", {this->functor("s", {this->var(0)})})}), {this->functor("odd", {this->var(0)})}, 1),
    });
    modules.emplace_back(6004, std::vector<pud_module_id>{6002, 6003}, std::vector<rule>{
        rule(this->functor("even_nat", {this->var(0)}), {this->functor("even", {this->var(0)}), this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("odd_nat", {this->var(0)}), {this->functor("odd", {this->var(0)}), this->functor("nat", {this->var(0)})}, 1),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < rng() % 30; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());

        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 6001;
            return;
        }
    }

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, RefutationsReloadsFaithfullyAfterAnyNumberOfRandomUnfolds) {
    for (uint32_t seed = 0; seed < 6 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(seed);
    std::deque<pud_module> modules;
    modules.emplace_back(7001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("a", {this->atom("one")}), {}, 0),
        rule(this->functor("a", {this->atom("two")}), {}, 0),
        rule(this->functor("b", {this->atom("two")}), {}, 0),
        rule(this->functor("d", {this->atom("one")}), {}, 0),
        rule(this->functor("c", {this->var(0)}), {this->functor("a", {this->var(0)}), this->functor("b", {this->var(0)}), this->functor("d", {this->var(0)})}, 1),
        rule(this->functor("e", {this->var(0)}), {this->functor("c", {this->var(0)})}, 1),
        rule(this->atom("f"), {this->atom("missing")}, 0),
        rule(this->functor("g", {this->var(0)}), {this->functor("a", {this->var(0)}), this->functor("missing", {this->var(0)})}, 1),
        rule(this->functor("h", {this->var(0)}), {this->functor("a", {this->var(0)}), this->functor("d", {this->var(0)})}, 1),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < rng() % 30; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());

        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 7001;
            return;
        }
    }

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, CascadeReloadsFaithfullyAfterAnyNumberOfRandomUnfolds) {
    for (uint32_t seed = 0; seed < 6 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(seed);
    std::deque<pud_module> modules;
    modules.emplace_back(8001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("base", {this->atom("c")}), {}, 0),
        rule(this->functor("check", {this->atom("a")}), {}, 0),
        rule(this->functor("mid", {this->var(0)}), {this->functor("base", {this->var(0)}), this->functor("check", {this->var(0)})}, 1),
    });
    modules.emplace_back(8002, std::vector<pud_module_id>{8001}, std::vector<rule>{
        rule(this->atom("top"), {this->functor("mid", {this->atom("c")})}, 0),
        rule(this->atom("other"), {this->functor("check", {this->atom("a")})}, 0),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < rng() % 30; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());

        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 8001;
            return;
        }
    }

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, PeanoMulReloadsFaithfullyAfterAnyNumberOfRandomUnfolds) {
    for (uint32_t seed = 0; seed < 6 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(seed);
    std::deque<pud_module> modules;
    modules.emplace_back(9001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {this->atom("z")}), {}, 0),
        rule(this->functor("nat", {this->functor("s", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("add", {this->atom("z"), this->var(0), this->var(0)}), {}, 1),
        rule(
            this->functor("add", {this->functor("s", {this->var(0)}), this->var(1), this->functor("s", {this->var(2)})}),
            {this->functor("add", {this->var(0), this->var(1), this->var(2)})},
            3),
        rule(this->functor("mul", {this->atom("z"), this->var(0), this->atom("z")}), {}, 1),
        rule(
            this->functor("mul", {this->functor("s", {this->var(0)}), this->var(1), this->var(2)}),
            {this->functor("mul", {this->var(0), this->var(1), this->var(3)}), this->functor("add", {this->var(3), this->var(1), this->var(2)})},
            4),
    });
    const expr* peano_6 = this->atom("z");
    for (uint32_t step = 0; step < 6; ++step)
        peano_6 = this->functor("s", {peano_6});
    const expr* peano_3 = this->functor("s", {this->functor("s", {this->functor("s", {this->atom("z")})})});
    const expr* peano_9 = this->atom("z");
    for (uint32_t step = 0; step < 9; ++step)
        peano_9 = this->functor("s", {peano_9});
    const expr* peano_2 = this->functor("s", {this->functor("s", {this->atom("z")})});
    modules.emplace_back(9002, std::vector<pud_module_id>{9001}, std::vector<rule>{
        rule(this->functor("factor", {this->var(0), this->var(1)}), {this->functor("mul", {this->var(0), this->var(1), peano_6})}, 2),
        rule(this->functor("triple", {this->var(0)}), {this->functor("mul", {peano_3, this->var(0), peano_9})}, 1),
        rule(
            this->functor("add_mul", {this->var(0), this->var(1), this->var(2)}),
            {this->functor("add", {this->var(0), this->var(1), this->var(2)}), this->functor("mul", {this->var(0), peano_2, this->var(3)})},
            4),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < rng() % 30; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());

        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 9001;
            return;
        }
    }

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, ListMembersReloadsFaithfullyAfterAnyNumberOfRandomUnfolds) {
    for (uint32_t seed = 0; seed < 6 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(seed);
    std::deque<pud_module> modules;
    modules.emplace_back(9101, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("mem", {this->var(0), this->functor("cons", {this->var(0), this->var(1)})}), {}, 2),
        rule(
            this->functor("mem", {this->var(0), this->functor("cons", {this->var(1), this->var(2)})}),
            {this->functor("mem", {this->var(0), this->var(2)})},
            3),
    });
    const expr* list_xyz = this->atom("nil");
    list_xyz = this->functor("cons", {this->atom("z"), list_xyz});
    list_xyz = this->functor("cons", {this->atom("y"), list_xyz});
    list_xyz = this->functor("cons", {this->atom("x"), list_xyz});
    const expr* list_abcd = this->atom("nil");
    list_abcd = this->functor("cons", {this->atom("d"), list_abcd});
    list_abcd = this->functor("cons", {this->atom("c"), list_abcd});
    list_abcd = this->functor("cons", {this->atom("b"), list_abcd});
    list_abcd = this->functor("cons", {this->atom("a"), list_abcd});
    const expr* list_bcd = this->functor("cons", {this->atom("b"), this->functor("cons", {this->atom("c"), this->functor("cons", {this->atom("d"), this->atom("nil")})})});
    modules.emplace_back(9102, std::vector<pud_module_id>{9101}, std::vector<rule>{
        rule(
            this->functor("every", {this->var(0)}),
            {this->functor("mem", {this->var(0), list_xyz})},
            1),
        rule(
            this->functor("pair_mem", {this->var(0), this->var(1)}),
            {
                this->functor("mem", {this->var(0), list_abcd}),
                this->functor("mem", {this->var(1), list_bcd}),
            },
            2),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < rng() % 30; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());

        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 9101;
            return;
        }
    }

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, MirrorTreesReloadsFaithfullyAfterAnyNumberOfRandomUnfolds) {
    for (uint32_t seed = 0; seed < 6 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(seed);
    std::deque<pud_module> modules;
    const expr* sample =
        this->functor("branch", {this->functor("leaf", {this->atom("l")}), this->functor("branch", {this->functor("leaf", {this->atom("m")}), this->functor("leaf", {this->atom("r")})})});
    modules.emplace_back(9201, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("mirror", {this->functor("leaf", {this->var(0)}), this->functor("leaf", {this->var(0)})}), {}, 1),
        rule(
            this->functor("mirror", {this->functor("branch", {this->var(0), this->var(1)}), this->functor("branch", {this->var(2), this->var(3)})}),
            {this->functor("mirror", {this->var(0), this->var(3)}), this->functor("mirror", {this->var(1), this->var(2)})},
            4),
    });
    modules.emplace_back(9202, std::vector<pud_module_id>{9201}, std::vector<rule>{
        rule(this->functor("flip", {this->var(0)}), {this->functor("mirror", {sample, this->var(0)})}, 1),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < rng() % 30; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());

        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 9201;
            return;
        }
    }

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, NestedUnaryChainReloadsFaithfullyAfterAnyNumberOfRandomUnfolds) {
    for (uint32_t seed = 0; seed < 6 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(seed);
    std::deque<pud_module> modules;
    const expr* nested_s8 = this->atom("z");
    for (uint32_t level = 0; level < 8; ++level)
        nested_s8 = this->functor("s", {nested_s8});
    const expr* nested_s3 = this->atom("z");
    for (uint32_t level = 0; level < 3; ++level)
        nested_s3 = this->functor("s", {nested_s3});
    const expr* nested_s2 = this->functor("s", {this->functor("s", {nested_s3})});
    const expr* peano_2 = this->functor("s", {this->functor("s", {this->atom("z")})});
    modules.emplace_back(9301, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {this->atom("z")}), {}, 0),
        rule(this->functor("nat", {this->functor("s", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(
            this->functor("deep_nat", {nested_s8}),
            {this->functor("nat", {nested_s8})},
            0),
        rule(
            this->functor("shallow", {this->var(0)}),
            {this->functor("nat", {this->var(0)}), this->functor("add", {this->var(0), peano_2, nested_s2})},
            1),
        rule(this->functor("add", {this->atom("z"), this->var(0), this->var(0)}), {}, 1),
        rule(
            this->functor("add", {this->functor("s", {this->var(0)}), this->var(1), this->functor("s", {this->var(2)})}),
            {this->functor("add", {this->var(0), this->var(1), this->var(2)})},
            3),
    });
    const expr* nested_s5 = this->atom("z");
    for (uint32_t level = 0; level < 5; ++level)
        nested_s5 = this->functor("s", {nested_s5});
    modules.emplace_back(9302, std::vector<pud_module_id>{9301}, std::vector<rule>{
        rule(
            this->functor("reach", {this->var(0)}),
            {this->functor("nat", {nested_s5}), this->functor("add", {this->var(0), this->functor("s", {this->atom("z")}), nested_s5})},
            1),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < rng() % 30; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());

        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 9301;
            return;
        }
    }

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, TripleAppendReloadsFaithfullyAfterAnyNumberOfRandomUnfolds) {
    for (uint32_t seed = 0; seed < 6 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(seed);
    std::deque<pud_module> modules;
    modules.emplace_back(9401, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("app", {this->atom("nil"), this->var(0), this->var(0)}), {}, 1),
        rule(
            this->functor("app", {this->functor("cons", {this->var(0), this->var(1)}), this->var(2), this->functor("cons", {this->var(0), this->var(3)})}),
            {this->functor("app", {this->var(1), this->var(2), this->var(3)})},
            4),
    });
    const expr* list_abc = this->atom("nil");
    list_abc = this->functor("cons", {this->atom("c"), list_abc});
    list_abc = this->functor("cons", {this->atom("b"), list_abc});
    list_abc = this->functor("cons", {this->atom("a"), list_abc});
    const expr* list_de = this->functor("cons", {this->atom("d"), this->functor("cons", {this->atom("e"), this->atom("nil")})});
    const expr* list_pq = this->functor("cons", {this->atom("p"), this->functor("cons", {this->atom("q"), this->atom("nil")})});
    const expr* list_r = this->functor("cons", {this->atom("r"), this->atom("nil")});
    const expr* list_pqr = this->functor("cons", {this->atom("p"), this->functor("cons", {this->atom("q"), this->functor("cons", {this->atom("r"), this->atom("nil")})})});
    modules.emplace_back(9402, std::vector<pud_module_id>{9401}, std::vector<rule>{
        rule(
            this->atom("triple_split"),
            {
                this->functor("app", {this->var(0), this->var(1), list_abc}),
                this->functor("app", {this->var(1), this->var(2), list_de}),
                this->functor("app", {this->var(0), this->var(2), this->var(3)}),
            },
            4),
        rule(
            this->functor("nested_app", {this->var(0)}),
            {
                this->functor("app", {this->var(0), this->var(1), list_pq}),
                this->functor("app", {this->var(1), list_r, list_pqr}),
            },
            2),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < rng() % 30; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());

        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 9401;
            return;
        }
    }

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, SharedBinaryReloadsFaithfullyAfterAnyNumberOfRandomUnfolds) {
    for (uint32_t seed = 0; seed < 6 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(seed);
    std::deque<pud_module> modules;
    const expr* shared = this->atom("v");
    for (int level = 0; level < 4; ++level)
        shared = this->functor("pair", {shared, shared});
    modules.emplace_back(9501, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {this->atom("z")}), {}, 0),
        rule(this->functor("nat", {this->functor("s", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("unpack", {shared, this->var(0)}), {this->functor("same", {this->var(0), shared})}, 1),
        rule(this->functor("same", {this->var(0), this->var(0)}), {}, 1),
        rule(
            this->functor("both", {this->var(0), this->var(1)}),
            {this->functor("same", {this->var(0), this->var(1)}), this->functor("nat", {this->var(0)})},
            2),
    });
    const expr* peano_2 = this->functor("s", {this->functor("s", {this->atom("z")})});
    modules.emplace_back(9502, std::vector<pud_module_id>{9501}, std::vector<rule>{
        rule(this->functor("grounded", {this->var(0)}), {this->functor("unpack", {shared, this->var(0)})}, 1),
        rule(this->functor("nat_query", {this->var(0)}), {this->functor("both", {peano_2, this->var(0)})}, 1),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < rng() % 30; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());

        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 9501;
            return;
        }
    }

}

// Resuming: a reloaded runtime unfolds exactly as the original goes on unfolding.

TYPED_TEST(PudRuntimeReloadIntegrationTest, ResumingFromAxiomsAloneIsLikeNeverHavingReloaded) {
    std::mt19937 rng(20);
    std::deque<pud_module> modules;
    modules.emplace_back(2001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {this->atom("z")}), {}, 0),
        rule(this->functor("nat", {this->functor("s", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("add", {this->atom("z"), this->var(0), this->var(0)}), {}, 1),
        rule(
            this->functor("add", {this->functor("s", {this->var(0)}), this->var(1), this->functor("s", {this->var(2)})}),
            {this->functor("add", {this->var(0), this->var(1), this->var(2)})},
            3),
    });
    modules.emplace_back(2002, std::vector<pud_module_id>{2001}, std::vector<rule>{
        rule(
            this->functor("half", {this->var(0)}),
            {this->functor("add", {this->var(0), this->var(0),
                this->functor("s", {this->functor("s", {this->functor("s", {this->functor("s", {this->atom("z")})})})})})},
            1),
        rule(this->functor("number", {this->var(0)}), {this->functor("nat", {this->var(0)})}, 1),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());
    std::deque<pud_module>& resumed_modules = reloaded_modules;
    TypeParam& resumed_runtime = reloaded_runtime;
    std::set<pud_global_node_id>& resumed_live = reloaded_live;
    size_t lockstep_taken = 0;
    while (lockstep_taken < 30 && !this->HasFailure()) {
        EXPECT_EQ(resumed_live, live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            live.erase(refuted);
        const pud_frontier_change actual = resumed_runtime.unfold(leaf, goal);
        resumed_live.erase(leaf);
        resumed_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            resumed_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring)
            << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++lockstep_taken;
    }
    EXPECT_THAT(reference_pud_module_differences(modules, resumed_modules), IsEmpty());
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    EXPECT_EQ(resumed_live, reference_pud_frontier(resumed_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(resumed_modules), IsEmpty());

    EXPECT_EQ(lockstep_taken, 30u);

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, AResumedProgramUnfoldsToExhaustionLikeTheOriginal) {
    std::mt19937 rng(21);
    std::deque<pud_module> modules;
    modules.emplace_back(1001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("color", {this->atom("red")}), {}, 0),
        rule(this->functor("color", {this->atom("blue")}), {}, 0),
        rule(this->functor("color", {this->atom("green")}), {}, 0),
        rule(this->functor("warm", {this->atom("red")}), {}, 0),
        rule(this->functor("warm", {this->atom("orange")}), {}, 0),
        rule(
            this->functor("pair", {this->var(0), this->var(1)}),
            {this->functor("color", {this->var(0)}), this->functor("color", {this->var(1)})},
            2),
        rule(
            this->functor("warm_color", {this->var(0)}),
            {this->functor("color", {this->var(0)}), this->functor("warm", {this->var(0)})},
            1),
        rule(this->atom("impossible"), {this->functor("color", {this->atom("purple")})}, 0),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < 4; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());
    std::deque<pud_module>& resumed_modules = reloaded_modules;
    TypeParam& resumed_runtime = reloaded_runtime;
    std::set<pud_global_node_id>& resumed_live = reloaded_live;
    size_t lockstep_taken = 0;
    while (lockstep_taken < 2000 && !this->HasFailure()) {
        EXPECT_EQ(resumed_live, live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(0);
        const pud_frontier_change expected = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            live.erase(refuted);
        const pud_frontier_change actual = resumed_runtime.unfold(leaf, goal);
        resumed_live.erase(leaf);
        resumed_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            resumed_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring)
            << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++lockstep_taken;
    }
    EXPECT_THAT(reference_pud_module_differences(modules, resumed_modules), IsEmpty());
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    EXPECT_EQ(resumed_live, reference_pud_frontier(resumed_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(resumed_modules), IsEmpty());

    EXPECT_LT(lockstep_taken, 2000u);
    std::vector<pud_global_node_id> unfoldable;
    for (pud_global_node_id leaf : live) {
        const std::optional<pud_module_id> only = std::nullopt;
        if (only.has_value() && leaf.module != only.value())
            continue;
        if (runtime.get_rule_information(leaf).body.empty())
            continue;
        unfoldable.push_back(leaf);
    }
    EXPECT_TRUE(unfoldable.empty());
    std::vector<pud_global_node_id> resumed_unfoldable;
    for (pud_global_node_id leaf : resumed_live) {
        const std::optional<pud_module_id> only = std::nullopt;
        if (only.has_value() && leaf.module != only.value())
            continue;
        if (resumed_runtime.get_rule_information(leaf).body.empty())
            continue;
        resumed_unfoldable.push_back(leaf);
    }
    EXPECT_TRUE(resumed_unfoldable.empty());

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, ResumedArithmeticUnfoldsLikeTheOriginal) {
    std::mt19937 rng(22);
    std::deque<pud_module> modules;
    modules.emplace_back(2001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {this->atom("z")}), {}, 0),
        rule(this->functor("nat", {this->functor("s", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("add", {this->atom("z"), this->var(0), this->var(0)}), {}, 1),
        rule(
            this->functor("add", {this->functor("s", {this->var(0)}), this->var(1), this->functor("s", {this->var(2)})}),
            {this->functor("add", {this->var(0), this->var(1), this->var(2)})},
            3),
    });
    modules.emplace_back(2002, std::vector<pud_module_id>{2001}, std::vector<rule>{
        rule(
            this->functor("half", {this->var(0)}),
            {this->functor("add", {this->var(0), this->var(0),
                this->functor("s", {this->functor("s", {this->functor("s", {this->functor("s", {this->atom("z")})})})})})},
            1),
        rule(this->functor("number", {this->var(0)}), {this->functor("nat", {this->var(0)})}, 1),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < 20; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());
    std::deque<pud_module>& resumed_modules = reloaded_modules;
    TypeParam& resumed_runtime = reloaded_runtime;
    std::set<pud_global_node_id>& resumed_live = reloaded_live;
    size_t lockstep_taken = 0;
    while (lockstep_taken < 30 && !this->HasFailure()) {
        EXPECT_EQ(resumed_live, live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            live.erase(refuted);
        const pud_frontier_change actual = resumed_runtime.unfold(leaf, goal);
        resumed_live.erase(leaf);
        resumed_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            resumed_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring)
            << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++lockstep_taken;
    }
    EXPECT_THAT(reference_pud_module_differences(modules, resumed_modules), IsEmpty());
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    EXPECT_EQ(resumed_live, reference_pud_frontier(resumed_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(resumed_modules), IsEmpty());

    EXPECT_EQ(lockstep_taken, 30u);

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, ResumedListsUnfoldLikeTheOriginal) {
    std::mt19937 rng(23);
    std::deque<pud_module> modules;
    modules.emplace_back(3001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("app", {this->atom("nil"), this->var(0), this->var(0)}), {}, 1),
        rule(
            this->functor("app", {this->functor("cons", {this->var(0), this->var(1)}), this->var(2), this->functor("cons", {this->var(0), this->var(3)})}),
            {this->functor("app", {this->var(1), this->var(2), this->var(3)})},
            4),
    });
    const expr* list_abc = this->atom("nil");
    list_abc = this->functor("cons", {this->atom("c"), list_abc});
    list_abc = this->functor("cons", {this->atom("b"), list_abc});
    list_abc = this->functor("cons", {this->atom("a"), list_abc});
    const expr* list_a = this->functor("cons", {this->atom("a"), this->atom("nil")});
    const expr* list_ab = this->functor("cons", {this->atom("a"), this->functor("cons", {this->atom("b"), this->atom("nil")})});
    modules.emplace_back(3002, std::vector<pud_module_id>{3001}, std::vector<rule>{
        rule(
            this->functor("split", {this->var(0), this->var(1)}),
            {this->functor("app", {this->var(0), this->var(1), list_abc})},
            2),
        rule(
            this->functor("suffix", {this->var(0)}),
            {this->functor("app", {list_a, this->var(0), list_ab})},
            1),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < 15; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());
    std::deque<pud_module>& resumed_modules = reloaded_modules;
    TypeParam& resumed_runtime = reloaded_runtime;
    std::set<pud_global_node_id>& resumed_live = reloaded_live;
    size_t lockstep_taken = 0;
    while (lockstep_taken < 30 && !this->HasFailure()) {
        EXPECT_EQ(resumed_live, live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            live.erase(refuted);
        const pud_frontier_change actual = resumed_runtime.unfold(leaf, goal);
        resumed_live.erase(leaf);
        resumed_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            resumed_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring)
            << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++lockstep_taken;
    }
    EXPECT_THAT(reference_pud_module_differences(modules, resumed_modules), IsEmpty());
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    EXPECT_EQ(resumed_live, reference_pud_frontier(resumed_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(resumed_modules), IsEmpty());

    EXPECT_EQ(lockstep_taken, 30u);

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, ResumedGraphQueriesUnfoldToExhaustionLikeTheOriginal) {
    std::mt19937 rng(24);
    std::deque<pud_module> modules;
    modules.emplace_back(4001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("edge", {this->atom("a"), this->atom("b")}), {}, 0),
        rule(this->functor("edge", {this->atom("b"), this->atom("c")}), {}, 0),
        rule(this->functor("edge", {this->atom("c"), this->atom("d")}), {}, 0),
        rule(this->functor("edge", {this->atom("b"), this->atom("e")}), {}, 0),
        rule(this->functor("path", {this->var(0), this->var(1)}), {this->functor("edge", {this->var(0), this->var(1)})}, 2),
        rule(
            this->functor("path", {this->var(0), this->var(2)}),
            {this->functor("edge", {this->var(0), this->var(1)}), this->functor("path", {this->var(1), this->var(2)})},
            3),
    });
    modules.emplace_back(4002, std::vector<pud_module_id>{4001}, std::vector<rule>{
        rule(this->functor("reach", {this->var(0)}), {this->functor("path", {this->atom("a"), this->var(0)})}, 1),
        rule(this->atom("unreachable"), {this->functor("path", {this->atom("d"), this->atom("a")})}, 0),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < 10; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());
    std::deque<pud_module>& resumed_modules = reloaded_modules;
    TypeParam& resumed_runtime = reloaded_runtime;
    std::set<pud_global_node_id>& resumed_live = reloaded_live;
    size_t lockstep_taken = 0;
    while (lockstep_taken < 2000 && !this->HasFailure()) {
        EXPECT_EQ(resumed_live, live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = 4002;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(0);
        const pud_frontier_change expected = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            live.erase(refuted);
        const pud_frontier_change actual = resumed_runtime.unfold(leaf, goal);
        resumed_live.erase(leaf);
        resumed_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            resumed_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring)
            << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++lockstep_taken;
    }
    EXPECT_THAT(reference_pud_module_differences(modules, resumed_modules), IsEmpty());
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    EXPECT_EQ(resumed_live, reference_pud_frontier(resumed_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(resumed_modules), IsEmpty());

    EXPECT_LT(lockstep_taken, 2000u);
    std::vector<pud_global_node_id> unfoldable;
    for (pud_global_node_id leaf : live) {
        const std::optional<pud_module_id> only = 4002;
        if (only.has_value() && leaf.module != only.value())
            continue;
        if (runtime.get_rule_information(leaf).body.empty())
            continue;
        unfoldable.push_back(leaf);
    }
    EXPECT_TRUE(unfoldable.empty());
    std::vector<pud_global_node_id> resumed_unfoldable;
    for (pud_global_node_id leaf : resumed_live) {
        const std::optional<pud_module_id> only = 4002;
        if (only.has_value() && leaf.module != only.value())
            continue;
        if (resumed_runtime.get_rule_information(leaf).body.empty())
            continue;
        resumed_unfoldable.push_back(leaf);
    }
    EXPECT_TRUE(resumed_unfoldable.empty());

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, ResumedTiersUnfoldLikeTheOriginal) {
    std::mt19937 rng(25);
    std::deque<pud_module> modules;
    modules.emplace_back(5001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("p", {this->atom("a")}), {}, 0),
        rule(this->functor("p", {this->atom("b")}), {}, 0),
    });
    modules.emplace_back(5002, std::vector<pud_module_id>{5001}, std::vector<rule>{
        rule(this->functor("r", {this->var(0)}), {this->functor("p", {this->var(0)})}, 1),
        rule(this->functor("r", {this->atom("c")}), {}, 0),
    });
    modules.emplace_back(5003, std::vector<pud_module_id>{5002}, std::vector<rule>{
        rule(this->functor("q", {this->var(0)}), {this->functor("r", {this->var(0)}), this->functor("p", {this->var(0)})}, 1),
        rule(this->functor("only_c", {this->var(0)}), {this->functor("r", {this->var(0)}), this->functor("s", {this->var(0)})}, 1),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < 3; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());
    std::deque<pud_module>& resumed_modules = reloaded_modules;
    TypeParam& resumed_runtime = reloaded_runtime;
    std::set<pud_global_node_id>& resumed_live = reloaded_live;
    size_t lockstep_taken = 0;
    while (lockstep_taken < 2000 && !this->HasFailure()) {
        EXPECT_EQ(resumed_live, live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(0);
        const pud_frontier_change expected = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            live.erase(refuted);
        const pud_frontier_change actual = resumed_runtime.unfold(leaf, goal);
        resumed_live.erase(leaf);
        resumed_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            resumed_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring)
            << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++lockstep_taken;
    }
    EXPECT_THAT(reference_pud_module_differences(modules, resumed_modules), IsEmpty());
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    EXPECT_EQ(resumed_live, reference_pud_frontier(resumed_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(resumed_modules), IsEmpty());

    EXPECT_LT(lockstep_taken, 2000u);

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, ResumedSiblingModulesUnfoldLikeTheOriginal) {
    std::mt19937 rng(26);
    std::deque<pud_module> modules;
    modules.emplace_back(6001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {this->atom("z")}), {}, 0),
        rule(this->functor("nat", {this->functor("s", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
    });
    modules.emplace_back(6002, std::vector<pud_module_id>{6001}, std::vector<rule>{
        rule(this->functor("even", {this->atom("z")}), {}, 0),
        rule(this->functor("even", {this->functor("s", {this->functor("s", {this->var(0)})})}), {this->functor("even", {this->var(0)})}, 1),
    });
    const expr* peano_1 = this->functor("s", {this->atom("z")});
    modules.emplace_back(6003, std::vector<pud_module_id>{6001}, std::vector<rule>{
        rule(this->functor("odd", {peano_1}), {}, 0),
        rule(this->functor("odd", {this->functor("s", {this->functor("s", {this->var(0)})})}), {this->functor("odd", {this->var(0)})}, 1),
    });
    modules.emplace_back(6004, std::vector<pud_module_id>{6002, 6003}, std::vector<rule>{
        rule(this->functor("even_nat", {this->var(0)}), {this->functor("even", {this->var(0)}), this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("odd_nat", {this->var(0)}), {this->functor("odd", {this->var(0)}), this->functor("nat", {this->var(0)})}, 1),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < 20; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());
    std::deque<pud_module>& resumed_modules = reloaded_modules;
    TypeParam& resumed_runtime = reloaded_runtime;
    std::set<pud_global_node_id>& resumed_live = reloaded_live;
    size_t lockstep_taken = 0;
    while (lockstep_taken < 30 && !this->HasFailure()) {
        EXPECT_EQ(resumed_live, live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            live.erase(refuted);
        const pud_frontier_change actual = resumed_runtime.unfold(leaf, goal);
        resumed_live.erase(leaf);
        resumed_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            resumed_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring)
            << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++lockstep_taken;
    }
    EXPECT_THAT(reference_pud_module_differences(modules, resumed_modules), IsEmpty());
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    EXPECT_EQ(resumed_live, reference_pud_frontier(resumed_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(resumed_modules), IsEmpty());

    EXPECT_EQ(lockstep_taken, 30u);

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, ResumedRefutationsCascadeLikeTheOriginal) {
    std::mt19937 rng(27);
    std::deque<pud_module> modules;
    modules.emplace_back(7001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("a", {this->atom("one")}), {}, 0),
        rule(this->functor("a", {this->atom("two")}), {}, 0),
        rule(this->functor("b", {this->atom("two")}), {}, 0),
        rule(this->functor("d", {this->atom("one")}), {}, 0),
        rule(this->functor("c", {this->var(0)}), {this->functor("a", {this->var(0)}), this->functor("b", {this->var(0)}), this->functor("d", {this->var(0)})}, 1),
        rule(this->functor("e", {this->var(0)}), {this->functor("c", {this->var(0)})}, 1),
        rule(this->atom("f"), {this->atom("missing")}, 0),
        rule(this->functor("g", {this->var(0)}), {this->functor("a", {this->var(0)}), this->functor("missing", {this->var(0)})}, 1),
        rule(this->functor("h", {this->var(0)}), {this->functor("a", {this->var(0)}), this->functor("d", {this->var(0)})}, 1),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < 2; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());
    std::deque<pud_module>& resumed_modules = reloaded_modules;
    TypeParam& resumed_runtime = reloaded_runtime;
    std::set<pud_global_node_id>& resumed_live = reloaded_live;
    size_t lockstep_taken = 0;
    while (lockstep_taken < 2000 && !this->HasFailure()) {
        EXPECT_EQ(resumed_live, live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(0);
        const pud_frontier_change expected = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            live.erase(refuted);
        const pud_frontier_change actual = resumed_runtime.unfold(leaf, goal);
        resumed_live.erase(leaf);
        resumed_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            resumed_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring)
            << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++lockstep_taken;
    }
    EXPECT_THAT(reference_pud_module_differences(modules, resumed_modules), IsEmpty());
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    EXPECT_EQ(resumed_live, reference_pud_frontier(resumed_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(resumed_modules), IsEmpty());

    EXPECT_LT(lockstep_taken, 2000u);

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, AResumedUnfoldInADependencyRefutesTheDependentsLeavesLikeTheOriginal) {
    std::deque<pud_module> modules;
    modules.emplace_back(8001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("base", {this->atom("c")}), {}, 0),
        rule(this->functor("check", {this->atom("a")}), {}, 0),
        rule(this->functor("mid", {this->var(0)}), {this->functor("base", {this->var(0)}), this->functor("check", {this->var(0)})}, 1),
    });
    modules.emplace_back(8002, std::vector<pud_module_id>{8001}, std::vector<rule>{
        rule(this->atom("top"), {this->functor("mid", {this->atom("c")})}, 0),
        rule(this->atom("other"), {this->functor("check", {this->atom("a")})}, 0),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());
    std::deque<pud_module>& resumed_modules = reloaded_modules;
    TypeParam& resumed_runtime = reloaded_runtime;
    std::set<pud_global_node_id>& resumed_live = reloaded_live;

    const pud_global_node_id mid{.module = 8001, .node = 2};
    const pud_frontier_change expected = runtime.unfold(mid, 0);
    live.erase(mid);
    live.insert(expected.offspring.begin(), expected.offspring.end());
    for (pud_global_node_id refuted : expected.refuted)
        live.erase(refuted);
    const pud_frontier_change actual = resumed_runtime.unfold(mid, 0);
    resumed_live.erase(mid);
    resumed_live.insert(actual.offspring.begin(), actual.offspring.end());
    for (pud_global_node_id refuted : actual.refuted)
        resumed_live.erase(refuted);
    EXPECT_EQ(actual.offspring, expected.offspring);
    EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
    const pud_module* dep_module = nullptr;
    for (const pud_module& module : resumed_modules) {
        if (module.get_module_id() == 8002)
            dep_module = &module;
    }
    EXPECT_TRUE(dep_module->check_refuted(0));
    EXPECT_THAT(reference_pud_module_differences(modules, resumed_modules), IsEmpty());

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, AResumedUnfoldInADependencyKeepsADependentsQueryResolvable) {
    const expr* peano_3 = this->atom("z");
    for (uint32_t step = 0; step < 3; ++step)
        peano_3 = this->functor("s", {peano_3});
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {this->atom("z")}), {}, 0),
        rule(this->functor("nat", {this->functor("s", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
    });
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->atom("q"), {this->functor("nat", {peano_3})}, 0),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());
    std::deque<pud_module>& resumed_modules = reloaded_modules;
    TypeParam& resumed_runtime = reloaded_runtime;
    std::set<pud_global_node_id>& resumed_live = reloaded_live;

    const pud_global_node_id step_id{.module = 1, .node = 1};
    const pud_frontier_change step_orig = runtime.unfold(step_id, 0);
    live.erase(step_id);
    live.insert(step_orig.offspring.begin(), step_orig.offspring.end());
    for (pud_global_node_id refuted : step_orig.refuted)
        live.erase(refuted);
    const pud_frontier_change step_res = resumed_runtime.unfold(step_id, 0);
    resumed_live.erase(step_id);
    resumed_live.insert(step_res.offspring.begin(), step_res.offspring.end());
    for (pud_global_node_id refuted : step_res.refuted)
        resumed_live.erase(refuted);
    const pud_global_node_id query{.module = 2, .node = 0};
    const pud_frontier_change expected = runtime.unfold(query, 0);
    live.erase(query);
    live.insert(expected.offspring.begin(), expected.offspring.end());
    for (pud_global_node_id refuted : expected.refuted)
        live.erase(refuted);
    const pud_frontier_change actual = resumed_runtime.unfold(query, 0);
    resumed_live.erase(query);
    resumed_live.insert(actual.offspring.begin(), actual.offspring.end());
    for (pud_global_node_id refuted : actual.refuted)
        resumed_live.erase(refuted);
    ASSERT_EQ(actual.offspring.size(), 1u);
    EXPECT_EQ(actual.offspring, expected.offspring);
    EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
    EXPECT_THAT(reference_pud_module_differences(modules, resumed_modules), IsEmpty());

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, ReloadingAfterEveryUnfoldKeepsUnfoldingLikeTheOriginal) {
    std::mt19937 rng(28);
    std::deque<pud_module> modules;
    modules.emplace_back(2001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {this->atom("z")}), {}, 0),
        rule(this->functor("nat", {this->functor("s", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("add", {this->atom("z"), this->var(0), this->var(0)}), {}, 1),
        rule(
            this->functor("add", {this->functor("s", {this->var(0)}), this->var(1), this->functor("s", {this->var(2)})}),
            {this->functor("add", {this->var(0), this->var(1), this->var(2)})},
            3),
    });
    modules.emplace_back(2002, std::vector<pud_module_id>{2001}, std::vector<rule>{
        rule(
            this->functor("half", {this->var(0)}),
            {this->functor("add", {this->var(0), this->var(0),
                this->functor("s", {this->functor("s", {this->functor("s", {this->functor("s", {this->atom("z")})})})})})},
            1),
        rule(this->functor("number", {this->var(0)}), {this->functor("nat", {this->var(0)})}, 1),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());
    std::deque<pud_module> resumed_modules(reloaded_modules.begin(), reloaded_modules.end());
    auto resumed_runtime = std::make_unique<TypeParam>(this->exprs);
    std::set<pud_global_node_id> resumed_live = reloaded_live;
    for (pud_module& copy : resumed_modules) {
        install_shared_functor_names(copy, this->functors);
        resumed_runtime->register_module(copy);
    }

    for (int unfold_index = 0; unfold_index < 20 && !this->HasFailure(); ++unfold_index) {
        EXPECT_EQ(resumed_live, live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty()) {
            ASSERT_TRUE(false) << "unfold " << unfold_index;
            break;
        }
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            live.erase(refuted);
        const pud_frontier_change actual = resumed_runtime->unfold(leaf, goal);
        resumed_live.erase(leaf);
        resumed_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            resumed_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring)
            << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));

        std::deque<pud_module> next_modules(resumed_modules.begin(), resumed_modules.end());
        TypeParam next_runtime(this->exprs);
        std::set<pud_global_node_id> next_live;
        std::vector<pud_global_node_id> refuted_on_next;
        for (pud_module& copy : next_modules) {
            install_shared_functor_names(copy, this->functors);
            const pud_frontier_change change = next_runtime.register_module(copy);
            refuted_on_next.insert(refuted_on_next.end(), change.refuted.begin(), change.refuted.end());
            next_live.insert(change.offspring.begin(), change.offspring.end());
            for (pud_global_node_id refuted : change.refuted)
                next_live.erase(refuted);
        }
        EXPECT_EQ(next_live, resumed_live);
        EXPECT_EQ(next_live, reference_pud_frontier(next_modules));
        EXPECT_THAT(reference_pud_stranded_nodes(next_modules), IsEmpty());
        EXPECT_THAT(refuted_on_next, IsEmpty());
        EXPECT_THAT(reference_pud_module_differences(resumed_modules, next_modules), IsEmpty());
        resumed_modules = std::move(next_modules);
        resumed_live = std::move(next_live);
        resumed_runtime = std::make_unique<TypeParam>(this->exprs);
        for (pud_module& copy : resumed_modules) {
            install_shared_functor_names(copy, this->functors);
            resumed_runtime->register_module(copy);
        }
    }
    EXPECT_THAT(reference_pud_module_differences(modules, resumed_modules), IsEmpty());

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, ReloadingEveryFewUnfoldsKeepsUnfoldingLikeTheOriginal) {
    std::mt19937 rng(29);
    std::deque<pud_module> modules;
    modules.emplace_back(3001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("app", {this->atom("nil"), this->var(0), this->var(0)}), {}, 1),
        rule(
            this->functor("app", {this->functor("cons", {this->var(0), this->var(1)}), this->var(2), this->functor("cons", {this->var(0), this->var(3)})}),
            {this->functor("app", {this->var(1), this->var(2), this->var(3)})},
            4),
    });
    const expr* list_abc = this->atom("nil");
    list_abc = this->functor("cons", {this->atom("c"), list_abc});
    list_abc = this->functor("cons", {this->atom("b"), list_abc});
    list_abc = this->functor("cons", {this->atom("a"), list_abc});
    const expr* list_a = this->functor("cons", {this->atom("a"), this->atom("nil")});
    const expr* list_ab = this->functor("cons", {this->atom("a"), this->functor("cons", {this->atom("b"), this->atom("nil")})});
    modules.emplace_back(3002, std::vector<pud_module_id>{3001}, std::vector<rule>{
        rule(
            this->functor("split", {this->var(0), this->var(1)}),
            {this->functor("app", {this->var(0), this->var(1), list_abc})},
            2),
        rule(
            this->functor("suffix", {this->var(0)}),
            {this->functor("app", {list_a, this->var(0), list_ab})},
            1),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());
    std::deque<pud_module> resumed_modules(reloaded_modules.begin(), reloaded_modules.end());
    auto resumed_runtime = std::make_unique<TypeParam>(this->exprs);
    std::set<pud_global_node_id> resumed_live = reloaded_live;
    for (pud_module& copy : resumed_modules) {
        install_shared_functor_names(copy, this->functors);
        resumed_runtime->register_module(copy);
    }

    for (int generation = 0; generation < 6 && !this->HasFailure(); ++generation) {
        size_t lockstep_taken = 0;
        while (lockstep_taken < 6 && !this->HasFailure()) {
            EXPECT_EQ(resumed_live, live);
            std::vector<pud_global_node_id> unfoldable;
            for (pud_global_node_id leaf : live) {
                const std::optional<pud_module_id> only = std::nullopt;
                if (only.has_value() && leaf.module != only.value())
                    continue;
                if (runtime.get_rule_information(leaf).body.empty())
                    continue;
                unfoldable.push_back(leaf);
            }
            if (unfoldable.empty())
                break;
            const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
            std::vector<pud_body_goal_id> goal_ids;
            for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
                goal_ids.push_back(goal_id);
            std::sort(goal_ids.begin(), goal_ids.end());
            const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
            const pud_frontier_change expected = runtime.unfold(leaf, goal);
            live.erase(leaf);
            live.insert(expected.offspring.begin(), expected.offspring.end());
            for (pud_global_node_id refuted : expected.refuted)
                live.erase(refuted);
            const pud_frontier_change actual = resumed_runtime->unfold(leaf, goal);
            resumed_live.erase(leaf);
            resumed_live.insert(actual.offspring.begin(), actual.offspring.end());
            for (pud_global_node_id refuted : actual.refuted)
                resumed_live.erase(refuted);
            EXPECT_EQ(actual.offspring, expected.offspring)
                << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
            EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
            ++lockstep_taken;
        }
        EXPECT_THAT(reference_pud_module_differences(modules, resumed_modules), IsEmpty());
        EXPECT_EQ(live, reference_pud_frontier(modules));
        EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
        EXPECT_EQ(resumed_live, reference_pud_frontier(resumed_modules));
        EXPECT_THAT(reference_pud_stranded_nodes(resumed_modules), IsEmpty());

        std::deque<pud_module> next_modules(resumed_modules.begin(), resumed_modules.end());
        TypeParam next_runtime(this->exprs);
        std::set<pud_global_node_id> next_live;
        std::vector<pud_global_node_id> refuted_on_next;
        for (pud_module& copy : next_modules) {
            install_shared_functor_names(copy, this->functors);
            const pud_frontier_change change = next_runtime.register_module(copy);
            refuted_on_next.insert(refuted_on_next.end(), change.refuted.begin(), change.refuted.end());
            next_live.insert(change.offspring.begin(), change.offspring.end());
            for (pud_global_node_id refuted : change.refuted)
                next_live.erase(refuted);
        }
        EXPECT_EQ(next_live, resumed_live);
        EXPECT_EQ(next_live, reference_pud_frontier(next_modules));
        EXPECT_THAT(reference_pud_stranded_nodes(next_modules), IsEmpty());
        EXPECT_THAT(refuted_on_next, IsEmpty());
        EXPECT_THAT(reference_pud_module_differences(resumed_modules, next_modules), IsEmpty());
        resumed_modules = std::move(next_modules);
        resumed_live = std::move(next_live);
        resumed_runtime = std::make_unique<TypeParam>(this->exprs);
        for (pud_module& copy : resumed_modules) {
            install_shared_functor_names(copy, this->functors);
            resumed_runtime->register_module(copy);
        }
    }
    EXPECT_EQ(resumed_live, live);

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, AResumedRuntimeCanItselfBeReloadedAndResumed) {
    std::mt19937 rng(30);
    std::deque<pud_module> modules;
    modules.emplace_back(4001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("edge", {this->atom("a"), this->atom("b")}), {}, 0),
        rule(this->functor("edge", {this->atom("b"), this->atom("c")}), {}, 0),
        rule(this->functor("edge", {this->atom("c"), this->atom("d")}), {}, 0),
        rule(this->functor("edge", {this->atom("b"), this->atom("e")}), {}, 0),
        rule(this->functor("path", {this->var(0), this->var(1)}), {this->functor("edge", {this->var(0), this->var(1)})}, 2),
        rule(
            this->functor("path", {this->var(0), this->var(2)}),
            {this->functor("edge", {this->var(0), this->var(1)}), this->functor("path", {this->var(1), this->var(2)})},
            3),
    });
    modules.emplace_back(4002, std::vector<pud_module_id>{4001}, std::vector<rule>{
        rule(this->functor("reach", {this->var(0)}), {this->functor("path", {this->atom("a"), this->var(0)})}, 1),
        rule(this->atom("unreachable"), {this->functor("path", {this->atom("d"), this->atom("a")})}, 0),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < 10; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());
    std::deque<pud_module>& resumed_modules = reloaded_modules;
    TypeParam& resumed_runtime = reloaded_runtime;
    std::set<pud_global_node_id>& resumed_live = reloaded_live;
    size_t lockstep_taken = 0;
    while (lockstep_taken < 15 && !this->HasFailure()) {
        EXPECT_EQ(resumed_live, live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            live.erase(refuted);
        const pud_frontier_change actual = resumed_runtime.unfold(leaf, goal);
        resumed_live.erase(leaf);
        resumed_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            resumed_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring)
            << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++lockstep_taken;
    }
    EXPECT_THAT(reference_pud_module_differences(modules, resumed_modules), IsEmpty());
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    EXPECT_EQ(resumed_live, reference_pud_frontier(resumed_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(resumed_modules), IsEmpty());

    std::deque<pud_module> second_modules(resumed_modules.begin(), resumed_modules.end());
    TypeParam second_runtime(this->exprs);
    std::set<pud_global_node_id> second_live;
    std::vector<pud_global_node_id> refuted_on_second;
    for (pud_module& copy : second_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = second_runtime.register_module(copy);
        refuted_on_second.insert(refuted_on_second.end(), change.refuted.begin(), change.refuted.end());
        second_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            second_live.erase(refuted);
    }
    EXPECT_EQ(second_live, resumed_live);
    EXPECT_EQ(second_live, reference_pud_frontier(second_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(second_modules), IsEmpty());
    EXPECT_THAT(refuted_on_second, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(resumed_modules, second_modules), IsEmpty());
    size_t second_lockstep_taken = 0;
    while (second_lockstep_taken < 15 && !this->HasFailure()) {
        EXPECT_EQ(second_live, live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            live.erase(refuted);
        const pud_frontier_change actual = second_runtime.unfold(leaf, goal);
        second_live.erase(leaf);
        second_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            second_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring)
            << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++second_lockstep_taken;
    }
    EXPECT_THAT(reference_pud_module_differences(modules, second_modules), IsEmpty());
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    EXPECT_EQ(second_live, reference_pud_frontier(second_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(second_modules), IsEmpty());

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, ResumedPeanoMulContinuesLikeTheOriginal) {
    std::mt19937 rng(70);
    std::deque<pud_module> modules;
    modules.emplace_back(9001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {this->atom("z")}), {}, 0),
        rule(this->functor("nat", {this->functor("s", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("add", {this->atom("z"), this->var(0), this->var(0)}), {}, 1),
        rule(
            this->functor("add", {this->functor("s", {this->var(0)}), this->var(1), this->functor("s", {this->var(2)})}),
            {this->functor("add", {this->var(0), this->var(1), this->var(2)})},
            3),
        rule(this->functor("mul", {this->atom("z"), this->var(0), this->atom("z")}), {}, 1),
        rule(
            this->functor("mul", {this->functor("s", {this->var(0)}), this->var(1), this->var(2)}),
            {this->functor("mul", {this->var(0), this->var(1), this->var(3)}), this->functor("add", {this->var(3), this->var(1), this->var(2)})},
            4),
    });
    const expr* peano_6 = this->atom("z");
    for (uint32_t step = 0; step < 6; ++step)
        peano_6 = this->functor("s", {peano_6});
    const expr* peano_3 = this->functor("s", {this->functor("s", {this->functor("s", {this->atom("z")})})});
    const expr* peano_9 = this->atom("z");
    for (uint32_t step = 0; step < 9; ++step)
        peano_9 = this->functor("s", {peano_9});
    const expr* peano_2 = this->functor("s", {this->functor("s", {this->atom("z")})});
    modules.emplace_back(9002, std::vector<pud_module_id>{9001}, std::vector<rule>{
        rule(this->functor("factor", {this->var(0), this->var(1)}), {this->functor("mul", {this->var(0), this->var(1), peano_6})}, 2),
        rule(this->functor("triple", {this->var(0)}), {this->functor("mul", {peano_3, this->var(0), peano_9})}, 1),
        rule(
            this->functor("add_mul", {this->var(0), this->var(1), this->var(2)}),
            {this->functor("add", {this->var(0), this->var(1), this->var(2)}), this->functor("mul", {this->var(0), peano_2, this->var(3)})},
            4),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < 35; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());
    std::deque<pud_module>& resumed_modules = reloaded_modules;
    TypeParam& resumed_runtime = reloaded_runtime;
    std::set<pud_global_node_id>& resumed_live = reloaded_live;
    size_t lockstep_taken = 0;
    while (lockstep_taken < 50 && !this->HasFailure()) {
        EXPECT_EQ(resumed_live, live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            live.erase(refuted);
        const pud_frontier_change actual = resumed_runtime.unfold(leaf, goal);
        resumed_live.erase(leaf);
        resumed_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            resumed_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring)
            << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++lockstep_taken;
    }
    EXPECT_THAT(reference_pud_module_differences(modules, resumed_modules), IsEmpty());
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    EXPECT_EQ(resumed_live, reference_pud_frontier(resumed_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(resumed_modules), IsEmpty());

    EXPECT_EQ(lockstep_taken, 50u);

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, ResumedTripleAppendContinuesLikeTheOriginal) {
    std::mt19937 rng(71);
    std::deque<pud_module> modules;
    modules.emplace_back(9401, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("app", {this->atom("nil"), this->var(0), this->var(0)}), {}, 1),
        rule(
            this->functor("app", {this->functor("cons", {this->var(0), this->var(1)}), this->var(2), this->functor("cons", {this->var(0), this->var(3)})}),
            {this->functor("app", {this->var(1), this->var(2), this->var(3)})},
            4),
    });
    const expr* list_abc = this->atom("nil");
    list_abc = this->functor("cons", {this->atom("c"), list_abc});
    list_abc = this->functor("cons", {this->atom("b"), list_abc});
    list_abc = this->functor("cons", {this->atom("a"), list_abc});
    const expr* list_de = this->functor("cons", {this->atom("d"), this->functor("cons", {this->atom("e"), this->atom("nil")})});
    const expr* list_pq = this->functor("cons", {this->atom("p"), this->functor("cons", {this->atom("q"), this->atom("nil")})});
    const expr* list_r = this->functor("cons", {this->atom("r"), this->atom("nil")});
    const expr* list_pqr = this->functor("cons", {this->atom("p"), this->functor("cons", {this->atom("q"), this->functor("cons", {this->atom("r"), this->atom("nil")})})});
    modules.emplace_back(9402, std::vector<pud_module_id>{9401}, std::vector<rule>{
        rule(
            this->atom("triple_split"),
            {
                this->functor("app", {this->var(0), this->var(1), list_abc}),
                this->functor("app", {this->var(1), this->var(2), list_de}),
                this->functor("app", {this->var(0), this->var(2), this->var(3)}),
            },
            4),
        rule(
            this->functor("nested_app", {this->var(0)}),
            {
                this->functor("app", {this->var(0), this->var(1), list_pq}),
                this->functor("app", {this->var(1), list_r, list_pqr}),
            },
            2),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < 25; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());
    std::deque<pud_module>& resumed_modules = reloaded_modules;
    TypeParam& resumed_runtime = reloaded_runtime;
    std::set<pud_global_node_id>& resumed_live = reloaded_live;
    size_t lockstep_taken = 0;
    while (lockstep_taken < 45 && !this->HasFailure()) {
        EXPECT_EQ(resumed_live, live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            live.erase(refuted);
        const pud_frontier_change actual = resumed_runtime.unfold(leaf, goal);
        resumed_live.erase(leaf);
        resumed_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            resumed_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring)
            << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++lockstep_taken;
    }
    EXPECT_THAT(reference_pud_module_differences(modules, resumed_modules), IsEmpty());
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    EXPECT_EQ(resumed_live, reference_pud_frontier(resumed_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(resumed_modules), IsEmpty());

    EXPECT_EQ(lockstep_taken, 45u);

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, ResumedMirrorTreesUnfoldToExhaustionLikeTheOriginal) {
    std::mt19937 rng(72);
    std::deque<pud_module> modules;
    const expr* sample =
        this->functor("branch", {this->functor("leaf", {this->atom("l")}), this->functor("branch", {this->functor("leaf", {this->atom("m")}), this->functor("leaf", {this->atom("r")})})});
    modules.emplace_back(9201, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("mirror", {this->functor("leaf", {this->var(0)}), this->functor("leaf", {this->var(0)})}), {}, 1),
        rule(
            this->functor("mirror", {this->functor("branch", {this->var(0), this->var(1)}), this->functor("branch", {this->var(2), this->var(3)})}),
            {this->functor("mirror", {this->var(0), this->var(3)}), this->functor("mirror", {this->var(1), this->var(2)})},
            4),
    });
    modules.emplace_back(9202, std::vector<pud_module_id>{9201}, std::vector<rule>{
        rule(this->functor("flip", {this->var(0)}), {this->functor("mirror", {sample, this->var(0)})}, 1),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < 5; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());
    std::deque<pud_module>& resumed_modules = reloaded_modules;
    TypeParam& resumed_runtime = reloaded_runtime;
    std::set<pud_global_node_id>& resumed_live = reloaded_live;
    size_t lockstep_taken = 0;
    while (lockstep_taken < 2000 && !this->HasFailure()) {
        EXPECT_EQ(resumed_live, live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = 9202;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(0);
        const pud_frontier_change expected = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            live.erase(refuted);
        const pud_frontier_change actual = resumed_runtime.unfold(leaf, goal);
        resumed_live.erase(leaf);
        resumed_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            resumed_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring)
            << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++lockstep_taken;
    }
    EXPECT_THAT(reference_pud_module_differences(modules, resumed_modules), IsEmpty());
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    EXPECT_EQ(resumed_live, reference_pud_frontier(resumed_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(resumed_modules), IsEmpty());

    EXPECT_LT(lockstep_taken, 2000u);

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, PeanoMulResumesAfterHeavyUnfolds) {
    for (uint32_t seed = 0; seed < 3 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(400 + seed);
    std::deque<pud_module> modules;
    modules.emplace_back(9001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {this->atom("z")}), {}, 0),
        rule(this->functor("nat", {this->functor("s", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("add", {this->atom("z"), this->var(0), this->var(0)}), {}, 1),
        rule(
            this->functor("add", {this->functor("s", {this->var(0)}), this->var(1), this->functor("s", {this->var(2)})}),
            {this->functor("add", {this->var(0), this->var(1), this->var(2)})},
            3),
        rule(this->functor("mul", {this->atom("z"), this->var(0), this->atom("z")}), {}, 1),
        rule(
            this->functor("mul", {this->functor("s", {this->var(0)}), this->var(1), this->var(2)}),
            {this->functor("mul", {this->var(0), this->var(1), this->var(3)}), this->functor("add", {this->var(3), this->var(1), this->var(2)})},
            4),
    });
    const expr* peano_6 = this->atom("z");
    for (uint32_t step = 0; step < 6; ++step)
        peano_6 = this->functor("s", {peano_6});
    const expr* peano_3 = this->functor("s", {this->functor("s", {this->functor("s", {this->atom("z")})})});
    const expr* peano_9 = this->atom("z");
    for (uint32_t step = 0; step < 9; ++step)
        peano_9 = this->functor("s", {peano_9});
    const expr* peano_2 = this->functor("s", {this->functor("s", {this->atom("z")})});
    modules.emplace_back(9002, std::vector<pud_module_id>{9001}, std::vector<rule>{
        rule(this->functor("factor", {this->var(0), this->var(1)}), {this->functor("mul", {this->var(0), this->var(1), peano_6})}, 2),
        rule(this->functor("triple", {this->var(0)}), {this->functor("mul", {peano_3, this->var(0), peano_9})}, 1),
        rule(
            this->functor("add_mul", {this->var(0), this->var(1), this->var(2)}),
            {this->functor("add", {this->var(0), this->var(1), this->var(2)}), this->functor("mul", {this->var(0), peano_2, this->var(3)})},
            4),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < 20 + rng() % 30; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());
    std::deque<pud_module>& resumed_modules = reloaded_modules;
    TypeParam& resumed_runtime = reloaded_runtime;
    std::set<pud_global_node_id>& resumed_live = reloaded_live;
    size_t lockstep_taken = 0;
    while (lockstep_taken < 35 && !this->HasFailure()) {
        EXPECT_EQ(resumed_live, live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            live.erase(refuted);
        const pud_frontier_change actual = resumed_runtime.unfold(leaf, goal);
        resumed_live.erase(leaf);
        resumed_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            resumed_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring)
            << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++lockstep_taken;
    }
    EXPECT_THAT(reference_pud_module_differences(modules, resumed_modules), IsEmpty());
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    EXPECT_EQ(resumed_live, reference_pud_frontier(resumed_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(resumed_modules), IsEmpty());

        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 9001;
            return;
        }
    }

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, ListMembersResumesAfterHeavyUnfolds) {
    for (uint32_t seed = 0; seed < 3 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(400 + seed);
    std::deque<pud_module> modules;
    modules.emplace_back(9101, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("mem", {this->var(0), this->functor("cons", {this->var(0), this->var(1)})}), {}, 2),
        rule(
            this->functor("mem", {this->var(0), this->functor("cons", {this->var(1), this->var(2)})}),
            {this->functor("mem", {this->var(0), this->var(2)})},
            3),
    });
    const expr* list_xyz = this->atom("nil");
    list_xyz = this->functor("cons", {this->atom("z"), list_xyz});
    list_xyz = this->functor("cons", {this->atom("y"), list_xyz});
    list_xyz = this->functor("cons", {this->atom("x"), list_xyz});
    const expr* list_abcd = this->atom("nil");
    list_abcd = this->functor("cons", {this->atom("d"), list_abcd});
    list_abcd = this->functor("cons", {this->atom("c"), list_abcd});
    list_abcd = this->functor("cons", {this->atom("b"), list_abcd});
    list_abcd = this->functor("cons", {this->atom("a"), list_abcd});
    const expr* list_bcd = this->functor("cons", {this->atom("b"), this->functor("cons", {this->atom("c"), this->functor("cons", {this->atom("d"), this->atom("nil")})})});
    modules.emplace_back(9102, std::vector<pud_module_id>{9101}, std::vector<rule>{
        rule(
            this->functor("every", {this->var(0)}),
            {this->functor("mem", {this->var(0), list_xyz})},
            1),
        rule(
            this->functor("pair_mem", {this->var(0), this->var(1)}),
            {
                this->functor("mem", {this->var(0), list_abcd}),
                this->functor("mem", {this->var(1), list_bcd}),
            },
            2),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < 20 + rng() % 30; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());
    std::deque<pud_module>& resumed_modules = reloaded_modules;
    TypeParam& resumed_runtime = reloaded_runtime;
    std::set<pud_global_node_id>& resumed_live = reloaded_live;
    size_t lockstep_taken = 0;
    while (lockstep_taken < 35 && !this->HasFailure()) {
        EXPECT_EQ(resumed_live, live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            live.erase(refuted);
        const pud_frontier_change actual = resumed_runtime.unfold(leaf, goal);
        resumed_live.erase(leaf);
        resumed_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            resumed_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring)
            << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++lockstep_taken;
    }
    EXPECT_THAT(reference_pud_module_differences(modules, resumed_modules), IsEmpty());
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    EXPECT_EQ(resumed_live, reference_pud_frontier(resumed_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(resumed_modules), IsEmpty());

        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 9101;
            return;
        }
    }

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, MirrorTreesResumesAfterHeavyUnfolds) {
    for (uint32_t seed = 0; seed < 3 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(400 + seed);
    std::deque<pud_module> modules;
    const expr* sample =
        this->functor("branch", {this->functor("leaf", {this->atom("l")}), this->functor("branch", {this->functor("leaf", {this->atom("m")}), this->functor("leaf", {this->atom("r")})})});
    modules.emplace_back(9201, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("mirror", {this->functor("leaf", {this->var(0)}), this->functor("leaf", {this->var(0)})}), {}, 1),
        rule(
            this->functor("mirror", {this->functor("branch", {this->var(0), this->var(1)}), this->functor("branch", {this->var(2), this->var(3)})}),
            {this->functor("mirror", {this->var(0), this->var(3)}), this->functor("mirror", {this->var(1), this->var(2)})},
            4),
    });
    modules.emplace_back(9202, std::vector<pud_module_id>{9201}, std::vector<rule>{
        rule(this->functor("flip", {this->var(0)}), {this->functor("mirror", {sample, this->var(0)})}, 1),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < 20 + rng() % 30; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());
    std::deque<pud_module>& resumed_modules = reloaded_modules;
    TypeParam& resumed_runtime = reloaded_runtime;
    std::set<pud_global_node_id>& resumed_live = reloaded_live;
    size_t lockstep_taken = 0;
    while (lockstep_taken < 35 && !this->HasFailure()) {
        EXPECT_EQ(resumed_live, live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            live.erase(refuted);
        const pud_frontier_change actual = resumed_runtime.unfold(leaf, goal);
        resumed_live.erase(leaf);
        resumed_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            resumed_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring)
            << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++lockstep_taken;
    }
    EXPECT_THAT(reference_pud_module_differences(modules, resumed_modules), IsEmpty());
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    EXPECT_EQ(resumed_live, reference_pud_frontier(resumed_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(resumed_modules), IsEmpty());

        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 9201;
            return;
        }
    }

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, NestedUnaryChainResumesAfterHeavyUnfolds) {
    for (uint32_t seed = 0; seed < 3 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(400 + seed);
    std::deque<pud_module> modules;
    const expr* nested_s8 = this->atom("z");
    for (uint32_t level = 0; level < 8; ++level)
        nested_s8 = this->functor("s", {nested_s8});
    const expr* nested_s3 = this->atom("z");
    for (uint32_t level = 0; level < 3; ++level)
        nested_s3 = this->functor("s", {nested_s3});
    const expr* nested_s2 = this->functor("s", {this->functor("s", {nested_s3})});
    const expr* peano_2 = this->functor("s", {this->functor("s", {this->atom("z")})});
    modules.emplace_back(9301, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {this->atom("z")}), {}, 0),
        rule(this->functor("nat", {this->functor("s", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(
            this->functor("deep_nat", {nested_s8}),
            {this->functor("nat", {nested_s8})},
            0),
        rule(
            this->functor("shallow", {this->var(0)}),
            {this->functor("nat", {this->var(0)}), this->functor("add", {this->var(0), peano_2, nested_s2})},
            1),
        rule(this->functor("add", {this->atom("z"), this->var(0), this->var(0)}), {}, 1),
        rule(
            this->functor("add", {this->functor("s", {this->var(0)}), this->var(1), this->functor("s", {this->var(2)})}),
            {this->functor("add", {this->var(0), this->var(1), this->var(2)})},
            3),
    });
    const expr* nested_s5 = this->atom("z");
    for (uint32_t level = 0; level < 5; ++level)
        nested_s5 = this->functor("s", {nested_s5});
    modules.emplace_back(9302, std::vector<pud_module_id>{9301}, std::vector<rule>{
        rule(
            this->functor("reach", {this->var(0)}),
            {this->functor("nat", {nested_s5}), this->functor("add", {this->var(0), this->functor("s", {this->atom("z")}), nested_s5})},
            1),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < 20 + rng() % 30; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());
    std::deque<pud_module>& resumed_modules = reloaded_modules;
    TypeParam& resumed_runtime = reloaded_runtime;
    std::set<pud_global_node_id>& resumed_live = reloaded_live;
    size_t lockstep_taken = 0;
    while (lockstep_taken < 35 && !this->HasFailure()) {
        EXPECT_EQ(resumed_live, live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            live.erase(refuted);
        const pud_frontier_change actual = resumed_runtime.unfold(leaf, goal);
        resumed_live.erase(leaf);
        resumed_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            resumed_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring)
            << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++lockstep_taken;
    }
    EXPECT_THAT(reference_pud_module_differences(modules, resumed_modules), IsEmpty());
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    EXPECT_EQ(resumed_live, reference_pud_frontier(resumed_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(resumed_modules), IsEmpty());

        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 9301;
            return;
        }
    }

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, TripleAppendResumesAfterHeavyUnfolds) {
    for (uint32_t seed = 0; seed < 3 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(400 + seed);
    std::deque<pud_module> modules;
    modules.emplace_back(9401, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("app", {this->atom("nil"), this->var(0), this->var(0)}), {}, 1),
        rule(
            this->functor("app", {this->functor("cons", {this->var(0), this->var(1)}), this->var(2), this->functor("cons", {this->var(0), this->var(3)})}),
            {this->functor("app", {this->var(1), this->var(2), this->var(3)})},
            4),
    });
    const expr* list_abc = this->atom("nil");
    list_abc = this->functor("cons", {this->atom("c"), list_abc});
    list_abc = this->functor("cons", {this->atom("b"), list_abc});
    list_abc = this->functor("cons", {this->atom("a"), list_abc});
    const expr* list_de = this->functor("cons", {this->atom("d"), this->functor("cons", {this->atom("e"), this->atom("nil")})});
    const expr* list_pq = this->functor("cons", {this->atom("p"), this->functor("cons", {this->atom("q"), this->atom("nil")})});
    const expr* list_r = this->functor("cons", {this->atom("r"), this->atom("nil")});
    const expr* list_pqr = this->functor("cons", {this->atom("p"), this->functor("cons", {this->atom("q"), this->functor("cons", {this->atom("r"), this->atom("nil")})})});
    modules.emplace_back(9402, std::vector<pud_module_id>{9401}, std::vector<rule>{
        rule(
            this->atom("triple_split"),
            {
                this->functor("app", {this->var(0), this->var(1), list_abc}),
                this->functor("app", {this->var(1), this->var(2), list_de}),
                this->functor("app", {this->var(0), this->var(2), this->var(3)}),
            },
            4),
        rule(
            this->functor("nested_app", {this->var(0)}),
            {
                this->functor("app", {this->var(0), this->var(1), list_pq}),
                this->functor("app", {this->var(1), list_r, list_pqr}),
            },
            2),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < 20 + rng() % 30; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());
    std::deque<pud_module>& resumed_modules = reloaded_modules;
    TypeParam& resumed_runtime = reloaded_runtime;
    std::set<pud_global_node_id>& resumed_live = reloaded_live;
    size_t lockstep_taken = 0;
    while (lockstep_taken < 35 && !this->HasFailure()) {
        EXPECT_EQ(resumed_live, live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            live.erase(refuted);
        const pud_frontier_change actual = resumed_runtime.unfold(leaf, goal);
        resumed_live.erase(leaf);
        resumed_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            resumed_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring)
            << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++lockstep_taken;
    }
    EXPECT_THAT(reference_pud_module_differences(modules, resumed_modules), IsEmpty());
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    EXPECT_EQ(resumed_live, reference_pud_frontier(resumed_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(resumed_modules), IsEmpty());

        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 9401;
            return;
        }
    }

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, SharedBinaryResumesAfterHeavyUnfolds) {
    for (uint32_t seed = 0; seed < 3 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(400 + seed);
    std::deque<pud_module> modules;
    const expr* shared = this->atom("v");
    for (int level = 0; level < 4; ++level)
        shared = this->functor("pair", {shared, shared});
    modules.emplace_back(9501, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {this->atom("z")}), {}, 0),
        rule(this->functor("nat", {this->functor("s", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("unpack", {shared, this->var(0)}), {this->functor("same", {this->var(0), shared})}, 1),
        rule(this->functor("same", {this->var(0), this->var(0)}), {}, 1),
        rule(
            this->functor("both", {this->var(0), this->var(1)}),
            {this->functor("same", {this->var(0), this->var(1)}), this->functor("nat", {this->var(0)})},
            2),
    });
    const expr* peano_2 = this->functor("s", {this->functor("s", {this->atom("z")})});
    modules.emplace_back(9502, std::vector<pud_module_id>{9501}, std::vector<rule>{
        rule(this->functor("grounded", {this->var(0)}), {this->functor("unpack", {shared, this->var(0)})}, 1),
        rule(this->functor("nat_query", {this->var(0)}), {this->functor("both", {peano_2, this->var(0)})}, 1),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < 20 + rng() % 30; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());
    std::deque<pud_module>& resumed_modules = reloaded_modules;
    TypeParam& resumed_runtime = reloaded_runtime;
    std::set<pud_global_node_id>& resumed_live = reloaded_live;
    size_t lockstep_taken = 0;
    while (lockstep_taken < 35 && !this->HasFailure()) {
        EXPECT_EQ(resumed_live, live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            live.erase(refuted);
        const pud_frontier_change actual = resumed_runtime.unfold(leaf, goal);
        resumed_live.erase(leaf);
        resumed_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            resumed_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring)
            << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++lockstep_taken;
    }
    EXPECT_THAT(reference_pud_module_differences(modules, resumed_modules), IsEmpty());
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    EXPECT_EQ(resumed_live, reference_pud_frontier(resumed_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(resumed_modules), IsEmpty());

        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 9501;
            return;
        }
    }

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, ColorsResumesLikeTheOriginalFromAnyPoint) {
    for (uint32_t seed = 0; seed < 5 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(100 + seed);
    std::deque<pud_module> modules;
    modules.emplace_back(1001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("color", {this->atom("red")}), {}, 0),
        rule(this->functor("color", {this->atom("blue")}), {}, 0),
        rule(this->functor("color", {this->atom("green")}), {}, 0),
        rule(this->functor("warm", {this->atom("red")}), {}, 0),
        rule(this->functor("warm", {this->atom("orange")}), {}, 0),
        rule(
            this->functor("pair", {this->var(0), this->var(1)}),
            {this->functor("color", {this->var(0)}), this->functor("color", {this->var(1)})},
            2),
        rule(
            this->functor("warm_color", {this->var(0)}),
            {this->functor("color", {this->var(0)}), this->functor("warm", {this->var(0)})},
            1),
        rule(this->atom("impossible"), {this->functor("color", {this->atom("purple")})}, 0),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < rng() % 25; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());
    std::deque<pud_module>& resumed_modules = reloaded_modules;
    TypeParam& resumed_runtime = reloaded_runtime;
    std::set<pud_global_node_id>& resumed_live = reloaded_live;
    size_t lockstep_taken = 0;
    while (lockstep_taken < 20 && !this->HasFailure()) {
        EXPECT_EQ(resumed_live, live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            live.erase(refuted);
        const pud_frontier_change actual = resumed_runtime.unfold(leaf, goal);
        resumed_live.erase(leaf);
        resumed_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            resumed_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring)
            << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++lockstep_taken;
    }
    EXPECT_THAT(reference_pud_module_differences(modules, resumed_modules), IsEmpty());
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    EXPECT_EQ(resumed_live, reference_pud_frontier(resumed_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(resumed_modules), IsEmpty());

        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 1001;
            return;
        }
    }

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, ArithmeticResumesLikeTheOriginalFromAnyPoint) {
    for (uint32_t seed = 0; seed < 5 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(100 + seed);
    std::deque<pud_module> modules;
    modules.emplace_back(2001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {this->atom("z")}), {}, 0),
        rule(this->functor("nat", {this->functor("s", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("add", {this->atom("z"), this->var(0), this->var(0)}), {}, 1),
        rule(
            this->functor("add", {this->functor("s", {this->var(0)}), this->var(1), this->functor("s", {this->var(2)})}),
            {this->functor("add", {this->var(0), this->var(1), this->var(2)})},
            3),
    });
    modules.emplace_back(2002, std::vector<pud_module_id>{2001}, std::vector<rule>{
        rule(
            this->functor("half", {this->var(0)}),
            {this->functor("add", {this->var(0), this->var(0),
                this->functor("s", {this->functor("s", {this->functor("s", {this->functor("s", {this->atom("z")})})})})})},
            1),
        rule(this->functor("number", {this->var(0)}), {this->functor("nat", {this->var(0)})}, 1),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < rng() % 25; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());
    std::deque<pud_module>& resumed_modules = reloaded_modules;
    TypeParam& resumed_runtime = reloaded_runtime;
    std::set<pud_global_node_id>& resumed_live = reloaded_live;
    size_t lockstep_taken = 0;
    while (lockstep_taken < 20 && !this->HasFailure()) {
        EXPECT_EQ(resumed_live, live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            live.erase(refuted);
        const pud_frontier_change actual = resumed_runtime.unfold(leaf, goal);
        resumed_live.erase(leaf);
        resumed_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            resumed_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring)
            << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++lockstep_taken;
    }
    EXPECT_THAT(reference_pud_module_differences(modules, resumed_modules), IsEmpty());
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    EXPECT_EQ(resumed_live, reference_pud_frontier(resumed_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(resumed_modules), IsEmpty());

        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 2001;
            return;
        }
    }

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, ListsResumesLikeTheOriginalFromAnyPoint) {
    for (uint32_t seed = 0; seed < 5 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(100 + seed);
    std::deque<pud_module> modules;
    modules.emplace_back(3001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("app", {this->atom("nil"), this->var(0), this->var(0)}), {}, 1),
        rule(
            this->functor("app", {this->functor("cons", {this->var(0), this->var(1)}), this->var(2), this->functor("cons", {this->var(0), this->var(3)})}),
            {this->functor("app", {this->var(1), this->var(2), this->var(3)})},
            4),
    });
    const expr* list_abc = this->atom("nil");
    list_abc = this->functor("cons", {this->atom("c"), list_abc});
    list_abc = this->functor("cons", {this->atom("b"), list_abc});
    list_abc = this->functor("cons", {this->atom("a"), list_abc});
    const expr* list_a = this->functor("cons", {this->atom("a"), this->atom("nil")});
    const expr* list_ab = this->functor("cons", {this->atom("a"), this->functor("cons", {this->atom("b"), this->atom("nil")})});
    modules.emplace_back(3002, std::vector<pud_module_id>{3001}, std::vector<rule>{
        rule(
            this->functor("split", {this->var(0), this->var(1)}),
            {this->functor("app", {this->var(0), this->var(1), list_abc})},
            2),
        rule(
            this->functor("suffix", {this->var(0)}),
            {this->functor("app", {list_a, this->var(0), list_ab})},
            1),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < rng() % 25; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());
    std::deque<pud_module>& resumed_modules = reloaded_modules;
    TypeParam& resumed_runtime = reloaded_runtime;
    std::set<pud_global_node_id>& resumed_live = reloaded_live;
    size_t lockstep_taken = 0;
    while (lockstep_taken < 20 && !this->HasFailure()) {
        EXPECT_EQ(resumed_live, live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            live.erase(refuted);
        const pud_frontier_change actual = resumed_runtime.unfold(leaf, goal);
        resumed_live.erase(leaf);
        resumed_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            resumed_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring)
            << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++lockstep_taken;
    }
    EXPECT_THAT(reference_pud_module_differences(modules, resumed_modules), IsEmpty());
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    EXPECT_EQ(resumed_live, reference_pud_frontier(resumed_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(resumed_modules), IsEmpty());

        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 3001;
            return;
        }
    }

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, GraphResumesLikeTheOriginalFromAnyPoint) {
    for (uint32_t seed = 0; seed < 5 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(100 + seed);
    std::deque<pud_module> modules;
    modules.emplace_back(4001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("edge", {this->atom("a"), this->atom("b")}), {}, 0),
        rule(this->functor("edge", {this->atom("b"), this->atom("c")}), {}, 0),
        rule(this->functor("edge", {this->atom("c"), this->atom("d")}), {}, 0),
        rule(this->functor("edge", {this->atom("b"), this->atom("e")}), {}, 0),
        rule(this->functor("path", {this->var(0), this->var(1)}), {this->functor("edge", {this->var(0), this->var(1)})}, 2),
        rule(
            this->functor("path", {this->var(0), this->var(2)}),
            {this->functor("edge", {this->var(0), this->var(1)}), this->functor("path", {this->var(1), this->var(2)})},
            3),
    });
    modules.emplace_back(4002, std::vector<pud_module_id>{4001}, std::vector<rule>{
        rule(this->functor("reach", {this->var(0)}), {this->functor("path", {this->atom("a"), this->var(0)})}, 1),
        rule(this->atom("unreachable"), {this->functor("path", {this->atom("d"), this->atom("a")})}, 0),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < rng() % 25; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());
    std::deque<pud_module>& resumed_modules = reloaded_modules;
    TypeParam& resumed_runtime = reloaded_runtime;
    std::set<pud_global_node_id>& resumed_live = reloaded_live;
    size_t lockstep_taken = 0;
    while (lockstep_taken < 20 && !this->HasFailure()) {
        EXPECT_EQ(resumed_live, live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            live.erase(refuted);
        const pud_frontier_change actual = resumed_runtime.unfold(leaf, goal);
        resumed_live.erase(leaf);
        resumed_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            resumed_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring)
            << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++lockstep_taken;
    }
    EXPECT_THAT(reference_pud_module_differences(modules, resumed_modules), IsEmpty());
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    EXPECT_EQ(resumed_live, reference_pud_frontier(resumed_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(resumed_modules), IsEmpty());

        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 4001;
            return;
        }
    }

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, TiersResumesLikeTheOriginalFromAnyPoint) {
    for (uint32_t seed = 0; seed < 5 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(100 + seed);
    std::deque<pud_module> modules;
    modules.emplace_back(5001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("p", {this->atom("a")}), {}, 0),
        rule(this->functor("p", {this->atom("b")}), {}, 0),
    });
    modules.emplace_back(5002, std::vector<pud_module_id>{5001}, std::vector<rule>{
        rule(this->functor("r", {this->var(0)}), {this->functor("p", {this->var(0)})}, 1),
        rule(this->functor("r", {this->atom("c")}), {}, 0),
    });
    modules.emplace_back(5003, std::vector<pud_module_id>{5002}, std::vector<rule>{
        rule(this->functor("q", {this->var(0)}), {this->functor("r", {this->var(0)}), this->functor("p", {this->var(0)})}, 1),
        rule(this->functor("only_c", {this->var(0)}), {this->functor("r", {this->var(0)}), this->functor("s", {this->var(0)})}, 1),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < rng() % 25; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());
    std::deque<pud_module>& resumed_modules = reloaded_modules;
    TypeParam& resumed_runtime = reloaded_runtime;
    std::set<pud_global_node_id>& resumed_live = reloaded_live;
    size_t lockstep_taken = 0;
    while (lockstep_taken < 20 && !this->HasFailure()) {
        EXPECT_EQ(resumed_live, live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            live.erase(refuted);
        const pud_frontier_change actual = resumed_runtime.unfold(leaf, goal);
        resumed_live.erase(leaf);
        resumed_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            resumed_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring)
            << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++lockstep_taken;
    }
    EXPECT_THAT(reference_pud_module_differences(modules, resumed_modules), IsEmpty());
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    EXPECT_EQ(resumed_live, reference_pud_frontier(resumed_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(resumed_modules), IsEmpty());

        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 5001;
            return;
        }
    }

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, SiblingsResumesLikeTheOriginalFromAnyPoint) {
    for (uint32_t seed = 0; seed < 5 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(100 + seed);
    std::deque<pud_module> modules;
    modules.emplace_back(6001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {this->atom("z")}), {}, 0),
        rule(this->functor("nat", {this->functor("s", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
    });
    modules.emplace_back(6002, std::vector<pud_module_id>{6001}, std::vector<rule>{
        rule(this->functor("even", {this->atom("z")}), {}, 0),
        rule(this->functor("even", {this->functor("s", {this->functor("s", {this->var(0)})})}), {this->functor("even", {this->var(0)})}, 1),
    });
    const expr* peano_1 = this->functor("s", {this->atom("z")});
    modules.emplace_back(6003, std::vector<pud_module_id>{6001}, std::vector<rule>{
        rule(this->functor("odd", {peano_1}), {}, 0),
        rule(this->functor("odd", {this->functor("s", {this->functor("s", {this->var(0)})})}), {this->functor("odd", {this->var(0)})}, 1),
    });
    modules.emplace_back(6004, std::vector<pud_module_id>{6002, 6003}, std::vector<rule>{
        rule(this->functor("even_nat", {this->var(0)}), {this->functor("even", {this->var(0)}), this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("odd_nat", {this->var(0)}), {this->functor("odd", {this->var(0)}), this->functor("nat", {this->var(0)})}, 1),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < rng() % 25; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());
    std::deque<pud_module>& resumed_modules = reloaded_modules;
    TypeParam& resumed_runtime = reloaded_runtime;
    std::set<pud_global_node_id>& resumed_live = reloaded_live;
    size_t lockstep_taken = 0;
    while (lockstep_taken < 20 && !this->HasFailure()) {
        EXPECT_EQ(resumed_live, live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            live.erase(refuted);
        const pud_frontier_change actual = resumed_runtime.unfold(leaf, goal);
        resumed_live.erase(leaf);
        resumed_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            resumed_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring)
            << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++lockstep_taken;
    }
    EXPECT_THAT(reference_pud_module_differences(modules, resumed_modules), IsEmpty());
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    EXPECT_EQ(resumed_live, reference_pud_frontier(resumed_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(resumed_modules), IsEmpty());

        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 6001;
            return;
        }
    }

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, RefutationsResumesLikeTheOriginalFromAnyPoint) {
    for (uint32_t seed = 0; seed < 5 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(100 + seed);
    std::deque<pud_module> modules;
    modules.emplace_back(7001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("a", {this->atom("one")}), {}, 0),
        rule(this->functor("a", {this->atom("two")}), {}, 0),
        rule(this->functor("b", {this->atom("two")}), {}, 0),
        rule(this->functor("d", {this->atom("one")}), {}, 0),
        rule(this->functor("c", {this->var(0)}), {this->functor("a", {this->var(0)}), this->functor("b", {this->var(0)}), this->functor("d", {this->var(0)})}, 1),
        rule(this->functor("e", {this->var(0)}), {this->functor("c", {this->var(0)})}, 1),
        rule(this->atom("f"), {this->atom("missing")}, 0),
        rule(this->functor("g", {this->var(0)}), {this->functor("a", {this->var(0)}), this->functor("missing", {this->var(0)})}, 1),
        rule(this->functor("h", {this->var(0)}), {this->functor("a", {this->var(0)}), this->functor("d", {this->var(0)})}, 1),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < rng() % 25; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());
    std::deque<pud_module>& resumed_modules = reloaded_modules;
    TypeParam& resumed_runtime = reloaded_runtime;
    std::set<pud_global_node_id>& resumed_live = reloaded_live;
    size_t lockstep_taken = 0;
    while (lockstep_taken < 20 && !this->HasFailure()) {
        EXPECT_EQ(resumed_live, live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            live.erase(refuted);
        const pud_frontier_change actual = resumed_runtime.unfold(leaf, goal);
        resumed_live.erase(leaf);
        resumed_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            resumed_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring)
            << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++lockstep_taken;
    }
    EXPECT_THAT(reference_pud_module_differences(modules, resumed_modules), IsEmpty());
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    EXPECT_EQ(resumed_live, reference_pud_frontier(resumed_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(resumed_modules), IsEmpty());

        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 7001;
            return;
        }
    }

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, CascadeResumesLikeTheOriginalFromAnyPoint) {
    for (uint32_t seed = 0; seed < 5 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(100 + seed);
    std::deque<pud_module> modules;
    modules.emplace_back(8001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("base", {this->atom("c")}), {}, 0),
        rule(this->functor("check", {this->atom("a")}), {}, 0),
        rule(this->functor("mid", {this->var(0)}), {this->functor("base", {this->var(0)}), this->functor("check", {this->var(0)})}, 1),
    });
    modules.emplace_back(8002, std::vector<pud_module_id>{8001}, std::vector<rule>{
        rule(this->atom("top"), {this->functor("mid", {this->atom("c")})}, 0),
        rule(this->atom("other"), {this->functor("check", {this->atom("a")})}, 0),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < rng() % 25; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());
    std::deque<pud_module>& resumed_modules = reloaded_modules;
    TypeParam& resumed_runtime = reloaded_runtime;
    std::set<pud_global_node_id>& resumed_live = reloaded_live;
    size_t lockstep_taken = 0;
    while (lockstep_taken < 20 && !this->HasFailure()) {
        EXPECT_EQ(resumed_live, live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            live.erase(refuted);
        const pud_frontier_change actual = resumed_runtime.unfold(leaf, goal);
        resumed_live.erase(leaf);
        resumed_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            resumed_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring)
            << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++lockstep_taken;
    }
    EXPECT_THAT(reference_pud_module_differences(modules, resumed_modules), IsEmpty());
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    EXPECT_EQ(resumed_live, reference_pud_frontier(resumed_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(resumed_modules), IsEmpty());

        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 8001;
            return;
        }
    }

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, PeanoMulResumesLikeTheOriginalFromAnyPoint) {
    for (uint32_t seed = 0; seed < 5 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(100 + seed);
    std::deque<pud_module> modules;
    modules.emplace_back(9001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {this->atom("z")}), {}, 0),
        rule(this->functor("nat", {this->functor("s", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("add", {this->atom("z"), this->var(0), this->var(0)}), {}, 1),
        rule(
            this->functor("add", {this->functor("s", {this->var(0)}), this->var(1), this->functor("s", {this->var(2)})}),
            {this->functor("add", {this->var(0), this->var(1), this->var(2)})},
            3),
        rule(this->functor("mul", {this->atom("z"), this->var(0), this->atom("z")}), {}, 1),
        rule(
            this->functor("mul", {this->functor("s", {this->var(0)}), this->var(1), this->var(2)}),
            {this->functor("mul", {this->var(0), this->var(1), this->var(3)}), this->functor("add", {this->var(3), this->var(1), this->var(2)})},
            4),
    });
    const expr* peano_6 = this->atom("z");
    for (uint32_t step = 0; step < 6; ++step)
        peano_6 = this->functor("s", {peano_6});
    const expr* peano_3 = this->functor("s", {this->functor("s", {this->functor("s", {this->atom("z")})})});
    const expr* peano_9 = this->atom("z");
    for (uint32_t step = 0; step < 9; ++step)
        peano_9 = this->functor("s", {peano_9});
    const expr* peano_2 = this->functor("s", {this->functor("s", {this->atom("z")})});
    modules.emplace_back(9002, std::vector<pud_module_id>{9001}, std::vector<rule>{
        rule(this->functor("factor", {this->var(0), this->var(1)}), {this->functor("mul", {this->var(0), this->var(1), peano_6})}, 2),
        rule(this->functor("triple", {this->var(0)}), {this->functor("mul", {peano_3, this->var(0), peano_9})}, 1),
        rule(
            this->functor("add_mul", {this->var(0), this->var(1), this->var(2)}),
            {this->functor("add", {this->var(0), this->var(1), this->var(2)}), this->functor("mul", {this->var(0), peano_2, this->var(3)})},
            4),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < rng() % 25; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());
    std::deque<pud_module>& resumed_modules = reloaded_modules;
    TypeParam& resumed_runtime = reloaded_runtime;
    std::set<pud_global_node_id>& resumed_live = reloaded_live;
    size_t lockstep_taken = 0;
    while (lockstep_taken < 20 && !this->HasFailure()) {
        EXPECT_EQ(resumed_live, live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            live.erase(refuted);
        const pud_frontier_change actual = resumed_runtime.unfold(leaf, goal);
        resumed_live.erase(leaf);
        resumed_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            resumed_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring)
            << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++lockstep_taken;
    }
    EXPECT_THAT(reference_pud_module_differences(modules, resumed_modules), IsEmpty());
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    EXPECT_EQ(resumed_live, reference_pud_frontier(resumed_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(resumed_modules), IsEmpty());

        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 9001;
            return;
        }
    }

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, ListMembersResumesLikeTheOriginalFromAnyPoint) {
    for (uint32_t seed = 0; seed < 5 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(100 + seed);
    std::deque<pud_module> modules;
    modules.emplace_back(9101, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("mem", {this->var(0), this->functor("cons", {this->var(0), this->var(1)})}), {}, 2),
        rule(
            this->functor("mem", {this->var(0), this->functor("cons", {this->var(1), this->var(2)})}),
            {this->functor("mem", {this->var(0), this->var(2)})},
            3),
    });
    const expr* list_xyz = this->atom("nil");
    list_xyz = this->functor("cons", {this->atom("z"), list_xyz});
    list_xyz = this->functor("cons", {this->atom("y"), list_xyz});
    list_xyz = this->functor("cons", {this->atom("x"), list_xyz});
    const expr* list_abcd = this->atom("nil");
    list_abcd = this->functor("cons", {this->atom("d"), list_abcd});
    list_abcd = this->functor("cons", {this->atom("c"), list_abcd});
    list_abcd = this->functor("cons", {this->atom("b"), list_abcd});
    list_abcd = this->functor("cons", {this->atom("a"), list_abcd});
    const expr* list_bcd = this->functor("cons", {this->atom("b"), this->functor("cons", {this->atom("c"), this->functor("cons", {this->atom("d"), this->atom("nil")})})});
    modules.emplace_back(9102, std::vector<pud_module_id>{9101}, std::vector<rule>{
        rule(
            this->functor("every", {this->var(0)}),
            {this->functor("mem", {this->var(0), list_xyz})},
            1),
        rule(
            this->functor("pair_mem", {this->var(0), this->var(1)}),
            {
                this->functor("mem", {this->var(0), list_abcd}),
                this->functor("mem", {this->var(1), list_bcd}),
            },
            2),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < rng() % 25; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());
    std::deque<pud_module>& resumed_modules = reloaded_modules;
    TypeParam& resumed_runtime = reloaded_runtime;
    std::set<pud_global_node_id>& resumed_live = reloaded_live;
    size_t lockstep_taken = 0;
    while (lockstep_taken < 20 && !this->HasFailure()) {
        EXPECT_EQ(resumed_live, live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            live.erase(refuted);
        const pud_frontier_change actual = resumed_runtime.unfold(leaf, goal);
        resumed_live.erase(leaf);
        resumed_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            resumed_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring)
            << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++lockstep_taken;
    }
    EXPECT_THAT(reference_pud_module_differences(modules, resumed_modules), IsEmpty());
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    EXPECT_EQ(resumed_live, reference_pud_frontier(resumed_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(resumed_modules), IsEmpty());

        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 9101;
            return;
        }
    }

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, MirrorTreesResumesLikeTheOriginalFromAnyPoint) {
    for (uint32_t seed = 0; seed < 5 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(100 + seed);
    std::deque<pud_module> modules;
    const expr* sample =
        this->functor("branch", {this->functor("leaf", {this->atom("l")}), this->functor("branch", {this->functor("leaf", {this->atom("m")}), this->functor("leaf", {this->atom("r")})})});
    modules.emplace_back(9201, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("mirror", {this->functor("leaf", {this->var(0)}), this->functor("leaf", {this->var(0)})}), {}, 1),
        rule(
            this->functor("mirror", {this->functor("branch", {this->var(0), this->var(1)}), this->functor("branch", {this->var(2), this->var(3)})}),
            {this->functor("mirror", {this->var(0), this->var(3)}), this->functor("mirror", {this->var(1), this->var(2)})},
            4),
    });
    modules.emplace_back(9202, std::vector<pud_module_id>{9201}, std::vector<rule>{
        rule(this->functor("flip", {this->var(0)}), {this->functor("mirror", {sample, this->var(0)})}, 1),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < rng() % 25; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());
    std::deque<pud_module>& resumed_modules = reloaded_modules;
    TypeParam& resumed_runtime = reloaded_runtime;
    std::set<pud_global_node_id>& resumed_live = reloaded_live;
    size_t lockstep_taken = 0;
    while (lockstep_taken < 20 && !this->HasFailure()) {
        EXPECT_EQ(resumed_live, live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            live.erase(refuted);
        const pud_frontier_change actual = resumed_runtime.unfold(leaf, goal);
        resumed_live.erase(leaf);
        resumed_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            resumed_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring)
            << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++lockstep_taken;
    }
    EXPECT_THAT(reference_pud_module_differences(modules, resumed_modules), IsEmpty());
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    EXPECT_EQ(resumed_live, reference_pud_frontier(resumed_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(resumed_modules), IsEmpty());

        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 9201;
            return;
        }
    }

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, NestedUnaryChainResumesLikeTheOriginalFromAnyPoint) {
    for (uint32_t seed = 0; seed < 5 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(100 + seed);
    std::deque<pud_module> modules;
    const expr* nested_s8 = this->atom("z");
    for (uint32_t level = 0; level < 8; ++level)
        nested_s8 = this->functor("s", {nested_s8});
    const expr* nested_s3 = this->atom("z");
    for (uint32_t level = 0; level < 3; ++level)
        nested_s3 = this->functor("s", {nested_s3});
    const expr* nested_s2 = this->functor("s", {this->functor("s", {nested_s3})});
    const expr* peano_2 = this->functor("s", {this->functor("s", {this->atom("z")})});
    modules.emplace_back(9301, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {this->atom("z")}), {}, 0),
        rule(this->functor("nat", {this->functor("s", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(
            this->functor("deep_nat", {nested_s8}),
            {this->functor("nat", {nested_s8})},
            0),
        rule(
            this->functor("shallow", {this->var(0)}),
            {this->functor("nat", {this->var(0)}), this->functor("add", {this->var(0), peano_2, nested_s2})},
            1),
        rule(this->functor("add", {this->atom("z"), this->var(0), this->var(0)}), {}, 1),
        rule(
            this->functor("add", {this->functor("s", {this->var(0)}), this->var(1), this->functor("s", {this->var(2)})}),
            {this->functor("add", {this->var(0), this->var(1), this->var(2)})},
            3),
    });
    const expr* nested_s5 = this->atom("z");
    for (uint32_t level = 0; level < 5; ++level)
        nested_s5 = this->functor("s", {nested_s5});
    modules.emplace_back(9302, std::vector<pud_module_id>{9301}, std::vector<rule>{
        rule(
            this->functor("reach", {this->var(0)}),
            {this->functor("nat", {nested_s5}), this->functor("add", {this->var(0), this->functor("s", {this->atom("z")}), nested_s5})},
            1),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < rng() % 25; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());
    std::deque<pud_module>& resumed_modules = reloaded_modules;
    TypeParam& resumed_runtime = reloaded_runtime;
    std::set<pud_global_node_id>& resumed_live = reloaded_live;
    size_t lockstep_taken = 0;
    while (lockstep_taken < 20 && !this->HasFailure()) {
        EXPECT_EQ(resumed_live, live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            live.erase(refuted);
        const pud_frontier_change actual = resumed_runtime.unfold(leaf, goal);
        resumed_live.erase(leaf);
        resumed_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            resumed_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring)
            << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++lockstep_taken;
    }
    EXPECT_THAT(reference_pud_module_differences(modules, resumed_modules), IsEmpty());
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    EXPECT_EQ(resumed_live, reference_pud_frontier(resumed_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(resumed_modules), IsEmpty());

        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 9301;
            return;
        }
    }

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, TripleAppendResumesLikeTheOriginalFromAnyPoint) {
    for (uint32_t seed = 0; seed < 5 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(100 + seed);
    std::deque<pud_module> modules;
    modules.emplace_back(9401, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("app", {this->atom("nil"), this->var(0), this->var(0)}), {}, 1),
        rule(
            this->functor("app", {this->functor("cons", {this->var(0), this->var(1)}), this->var(2), this->functor("cons", {this->var(0), this->var(3)})}),
            {this->functor("app", {this->var(1), this->var(2), this->var(3)})},
            4),
    });
    const expr* list_abc = this->atom("nil");
    list_abc = this->functor("cons", {this->atom("c"), list_abc});
    list_abc = this->functor("cons", {this->atom("b"), list_abc});
    list_abc = this->functor("cons", {this->atom("a"), list_abc});
    const expr* list_de = this->functor("cons", {this->atom("d"), this->functor("cons", {this->atom("e"), this->atom("nil")})});
    const expr* list_pq = this->functor("cons", {this->atom("p"), this->functor("cons", {this->atom("q"), this->atom("nil")})});
    const expr* list_r = this->functor("cons", {this->atom("r"), this->atom("nil")});
    const expr* list_pqr = this->functor("cons", {this->atom("p"), this->functor("cons", {this->atom("q"), this->functor("cons", {this->atom("r"), this->atom("nil")})})});
    modules.emplace_back(9402, std::vector<pud_module_id>{9401}, std::vector<rule>{
        rule(
            this->atom("triple_split"),
            {
                this->functor("app", {this->var(0), this->var(1), list_abc}),
                this->functor("app", {this->var(1), this->var(2), list_de}),
                this->functor("app", {this->var(0), this->var(2), this->var(3)}),
            },
            4),
        rule(
            this->functor("nested_app", {this->var(0)}),
            {
                this->functor("app", {this->var(0), this->var(1), list_pq}),
                this->functor("app", {this->var(1), list_r, list_pqr}),
            },
            2),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < rng() % 25; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());
    std::deque<pud_module>& resumed_modules = reloaded_modules;
    TypeParam& resumed_runtime = reloaded_runtime;
    std::set<pud_global_node_id>& resumed_live = reloaded_live;
    size_t lockstep_taken = 0;
    while (lockstep_taken < 20 && !this->HasFailure()) {
        EXPECT_EQ(resumed_live, live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            live.erase(refuted);
        const pud_frontier_change actual = resumed_runtime.unfold(leaf, goal);
        resumed_live.erase(leaf);
        resumed_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            resumed_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring)
            << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++lockstep_taken;
    }
    EXPECT_THAT(reference_pud_module_differences(modules, resumed_modules), IsEmpty());
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    EXPECT_EQ(resumed_live, reference_pud_frontier(resumed_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(resumed_modules), IsEmpty());

        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 9401;
            return;
        }
    }

}

TYPED_TEST(PudRuntimeReloadIntegrationTest, SharedBinaryResumesLikeTheOriginalFromAnyPoint) {
    for (uint32_t seed = 0; seed < 5 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(100 + seed);
    std::deque<pud_module> modules;
    const expr* shared = this->atom("v");
    for (int level = 0; level < 4; ++level)
        shared = this->functor("pair", {shared, shared});
    modules.emplace_back(9501, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {this->atom("z")}), {}, 0),
        rule(this->functor("nat", {this->functor("s", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("unpack", {shared, this->var(0)}), {this->functor("same", {this->var(0), shared})}, 1),
        rule(this->functor("same", {this->var(0), this->var(0)}), {}, 1),
        rule(
            this->functor("both", {this->var(0), this->var(1)}),
            {this->functor("same", {this->var(0), this->var(1)}), this->functor("nat", {this->var(0)})},
            2),
    });
    const expr* peano_2 = this->functor("s", {this->functor("s", {this->atom("z")})});
    modules.emplace_back(9502, std::vector<pud_module_id>{9501}, std::vector<rule>{
        rule(this->functor("grounded", {this->var(0)}), {this->functor("unpack", {shared, this->var(0)})}, 1),
        rule(this->functor("nat_query", {this->var(0)}), {this->functor("both", {peano_2, this->var(0)})}, 1),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    size_t taken = 0;
    for (; taken < rng() % 25; ++taken) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
    std::deque<pud_module> reloaded_modules(modules.begin(), modules.end());
    TypeParam reloaded_runtime(this->exprs);
    std::set<pud_global_node_id> reloaded_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : reloaded_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = reloaded_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        reloaded_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            reloaded_live.erase(refuted);
    }
    EXPECT_EQ(reloaded_live, live);
    EXPECT_EQ(reloaded_live, reference_pud_frontier(reloaded_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(reloaded_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(modules, reloaded_modules), IsEmpty());
    std::deque<pud_module>& resumed_modules = reloaded_modules;
    TypeParam& resumed_runtime = reloaded_runtime;
    std::set<pud_global_node_id>& resumed_live = reloaded_live;
    size_t lockstep_taken = 0;
    while (lockstep_taken < 20 && !this->HasFailure()) {
        EXPECT_EQ(resumed_live, live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : live) {
            const std::optional<pud_module_id> only = std::nullopt;
            if (only.has_value() && leaf.module != only.value())
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = runtime.unfold(leaf, goal);
        live.erase(leaf);
        live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            live.erase(refuted);
        const pud_frontier_change actual = resumed_runtime.unfold(leaf, goal);
        resumed_live.erase(leaf);
        resumed_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            resumed_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring)
            << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++lockstep_taken;
    }
    EXPECT_THAT(reference_pud_module_differences(modules, resumed_modules), IsEmpty());
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    EXPECT_EQ(resumed_live, reference_pud_frontier(resumed_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(resumed_modules), IsEmpty());

        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 9501;
            return;
        }
    }

}

