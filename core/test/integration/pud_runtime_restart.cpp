#include <algorithm>
#include <cstdint>
#include <deque>
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

// Restarting on the same modules: the old runtime is thrown away and a new one registers the very
// modules it left behind, then carries on unfolding where it left off. A twin built from the same
// program goes through the same unfolds without ever restarting, and every unfold of the restarted
// world must report exactly what the twin's does.
template<typename IRuntime>
struct PudRuntimeRestartIntegrationTest : public ::testing::Test {
    test_functors functors;
    expr_pool exprs;

    const expr* var(uint32_t index) { return exprs.make_var(index); }
    const expr* atom(const char* name) { return exprs.make_functor(functors.id(name), {}); }
    const expr* functor(const char* name, std::vector<const expr*> args) {
        return exprs.make_functor(functors.id(name), std::move(args));
    }
};

using restarted_runtimes_t = ::testing::Types<pud_runtime<pud_module, expr_pool>>;
TYPED_TEST_SUITE(PudRuntimeRestartIntegrationTest, restarted_runtimes_t);

TYPED_TEST(PudRuntimeRestartIntegrationTest, RestartingBeforeAnyUnfoldPutsBackTheAxiomFrontier) {
    std::deque<pud_module> twin_modules;
    std::deque<pud_module> restarted_modules;
    for (std::deque<pud_module>* modules : {&twin_modules, &restarted_modules}) {
        modules->emplace_back(1001, std::vector<pud_module_id>{}, std::vector<rule>{
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

    }
    TypeParam twin_runtime(this->exprs);
    std::optional<TypeParam> restarted_runtime;
    restarted_runtime.emplace(this->exprs);
    std::set<pud_global_node_id> twin_live;
    std::set<pud_global_node_id> restarted_live;
    for (pud_module& module : twin_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = twin_runtime.register_module(module);
        twin_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            twin_live.erase(refuted);
    }
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }
{
    restarted_runtime.reset();
    restarted_runtime.emplace(this->exprs);
    restarted_live.clear();
    std::vector<pud_global_node_id> refuted_on_restart;
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        refuted_on_restart.insert(refuted_on_restart.end(), change.refuted.begin(), change.refuted.end());
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }
    EXPECT_EQ(restarted_live, twin_live);
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    EXPECT_THAT(refuted_on_restart, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
}
}

TYPED_TEST(PudRuntimeRestartIntegrationTest, RestartingAProgramThatRefutedOnRegistrationRefutesNothingAgain) {
    std::deque<pud_module> twin_modules;
    std::deque<pud_module> restarted_modules;
    for (std::deque<pud_module>* modules : {&twin_modules, &restarted_modules}) {
        modules->emplace_back(7001, std::vector<pud_module_id>{}, std::vector<rule>{
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

    }
    std::vector<pud_global_node_id> refuted_on_registration;
    TypeParam twin_runtime(this->exprs);
    std::optional<TypeParam> restarted_runtime;
    restarted_runtime.emplace(this->exprs);
    std::set<pud_global_node_id> twin_live;
    std::set<pud_global_node_id> restarted_live;
    for (pud_module& module : twin_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = twin_runtime.register_module(module);
        twin_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            twin_live.erase(refuted);
    }
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        refuted_on_registration.insert(refuted_on_registration.end(), change.refuted.begin(), change.refuted.end());
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }
    ASSERT_THAT(refuted_on_registration, Not(IsEmpty()));
{
    restarted_runtime.reset();
    restarted_runtime.emplace(this->exprs);
    restarted_live.clear();
    std::vector<pud_global_node_id> refuted_on_restart;
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        refuted_on_restart.insert(refuted_on_restart.end(), change.refuted.begin(), change.refuted.end());
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }
    EXPECT_EQ(restarted_live, twin_live);
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    EXPECT_THAT(refuted_on_restart, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
}
}

TYPED_TEST(PudRuntimeRestartIntegrationTest, RestartedArithmeticContinuesLikeItsTwin) {
    std::mt19937 rng(40);
    std::deque<pud_module> twin_modules;
    std::deque<pud_module> restarted_modules;
    for (std::deque<pud_module>* modules : {&twin_modules, &restarted_modules}) {
        modules->emplace_back(2001, std::vector<pud_module_id>{}, std::vector<rule>{
            rule(this->functor("nat", {this->atom("z")}), {}, 0),
            rule(this->functor("nat", {this->functor("s", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
            rule(this->functor("add", {this->atom("z"), this->var(0), this->var(0)}), {}, 1),
            rule(
                this->functor("add", {this->functor("s", {this->var(0)}), this->var(1), this->functor("s", {this->var(2)})}),
                {this->functor("add", {this->var(0), this->var(1), this->var(2)})},
                3),
        });
        modules->emplace_back(2002, std::vector<pud_module_id>{2001}, std::vector<rule>{
            rule(
                this->functor("half", {this->var(0)}),
                {this->functor("add", {this->var(0), this->var(0),
                    this->functor("s", {this->functor("s", {this->functor("s", {this->functor("s", {this->atom("z")})})})})})},
                1),
            rule(this->functor("number", {this->var(0)}), {this->functor("nat", {this->var(0)})}, 1),
        });
    }
    TypeParam twin_runtime(this->exprs);
    std::optional<TypeParam> restarted_runtime;
    restarted_runtime.emplace(this->exprs);
    std::set<pud_global_node_id> twin_live;
    std::set<pud_global_node_id> restarted_live;
    for (pud_module& module : twin_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = twin_runtime.register_module(module);
        twin_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            twin_live.erase(refuted);
    }
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }

    size_t taken = 0;
    while (taken < 20 && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    ASSERT_EQ(taken, 20u);

    restarted_runtime.reset();
    restarted_runtime.emplace(this->exprs);
    restarted_live.clear();
    std::vector<pud_global_node_id> refuted_on_restart;
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        refuted_on_restart.insert(refuted_on_restart.end(), change.refuted.begin(), change.refuted.end());
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }
    EXPECT_EQ(restarted_live, twin_live);
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    EXPECT_THAT(refuted_on_restart, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());

    taken = 0;
    while (taken < 30 && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    EXPECT_EQ(taken, 30u);
}





TYPED_TEST(PudRuntimeRestartIntegrationTest, RestartedListsContinueLikeTheirTwin) {
    std::mt19937 rng(41);
    std::deque<pud_module> twin_modules;
    std::deque<pud_module> restarted_modules;
    for (std::deque<pud_module>* modules : {&twin_modules, &restarted_modules}) {
        modules->emplace_back(3001, std::vector<pud_module_id>{}, std::vector<rule>{
            rule(this->functor("app", {this->atom("nil"), this->var(0), this->var(0)}), {}, 1),
            rule(
                this->functor("app", {this->functor("cons", {this->var(0), this->var(1)}), this->var(2), this->functor("cons", {this->var(0), this->var(3)})}),
                {this->functor("app", {this->var(1), this->var(2), this->var(3)})},
                4),
        });
        modules->emplace_back(3002, std::vector<pud_module_id>{3001}, std::vector<rule>{
            rule(
                this->functor("split", {this->var(0), this->var(1)}),
                {this->functor("app", {this->var(0), this->var(1), this->functor("cons", {this->atom("a"), this->functor("cons", {this->atom("b"), this->functor("cons", {this->atom("c"), this->atom("nil")})})})})},
                2),
            rule(
                this->functor("suffix", {this->var(0)}),
                {this->functor("app", {this->functor("cons", {this->atom("a"), this->atom("nil")}), this->var(0), this->functor("cons", {this->atom("a"), this->functor("cons", {this->atom("b"), this->atom("nil")})})})},
                1),
        });

    }
    TypeParam twin_runtime(this->exprs);
    std::optional<TypeParam> restarted_runtime;
    restarted_runtime.emplace(this->exprs);
    std::set<pud_global_node_id> twin_live;
    std::set<pud_global_node_id> restarted_live;
    for (pud_module& module : twin_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = twin_runtime.register_module(module);
        twin_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            twin_live.erase(refuted);
    }
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }

{    size_t taken = 0;
    while (taken < 15 && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    ASSERT_EQ(taken, 15u);
}{
    restarted_runtime.reset();
    restarted_runtime.emplace(this->exprs);
    restarted_live.clear();
    std::vector<pud_global_node_id> refuted_on_restart;
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        refuted_on_restart.insert(refuted_on_restart.end(), change.refuted.begin(), change.refuted.end());
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }
    EXPECT_EQ(restarted_live, twin_live);
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    EXPECT_THAT(refuted_on_restart, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
}
{    size_t taken = 0;
    while (taken < 30 && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    EXPECT_EQ(taken, 30u);
}
}
TYPED_TEST(PudRuntimeRestartIntegrationTest, RestartedGraphQueriesRunOutOfUnfoldsLikeTheirTwin) {
    std::mt19937 rng(42);
    std::deque<pud_module> twin_modules;
    std::deque<pud_module> restarted_modules;
    for (std::deque<pud_module>* modules : {&twin_modules, &restarted_modules}) {
        modules->emplace_back(4001, std::vector<pud_module_id>{}, std::vector<rule>{
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
        modules->emplace_back(4002, std::vector<pud_module_id>{4001}, std::vector<rule>{
            rule(this->functor("reach", {this->var(0)}), {this->functor("path", {this->atom("a"), this->var(0)})}, 1),
            rule(this->atom("unreachable"), {this->functor("path", {this->atom("d"), this->atom("a")})}, 0),
        });

    }
    TypeParam twin_runtime(this->exprs);
    std::optional<TypeParam> restarted_runtime;
    restarted_runtime.emplace(this->exprs);
    std::set<pud_global_node_id> twin_live;
    std::set<pud_global_node_id> restarted_live;
    for (pud_module& module : twin_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = twin_runtime.register_module(module);
        twin_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            twin_live.erase(refuted);
    }
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }

{    size_t taken = 0;
    while (taken < 8 && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
}
{
    restarted_runtime.reset();
    restarted_runtime.emplace(this->exprs);
    restarted_live.clear();
    std::vector<pud_global_node_id> refuted_on_restart;
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        refuted_on_restart.insert(refuted_on_restart.end(), change.refuted.begin(), change.refuted.end());
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }
    EXPECT_EQ(restarted_live, twin_live);
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    EXPECT_THAT(refuted_on_restart, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
}

{    size_t taken = 0;
    while (taken < 2000 && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (leaf.module != 4002)
                continue;
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(0);
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    EXPECT_LT(taken, 2000u);
}
    std::vector<pud_global_node_id> unfoldable;
    for (pud_global_node_id leaf : restarted_live) {
        if (leaf.module != 4002)
            continue;
        if (restarted_runtime->get_rule_information(leaf).body.empty())
            continue;
        unfoldable.push_back(leaf);
    }
    EXPECT_TRUE(unfoldable.empty());
}

TYPED_TEST(PudRuntimeRestartIntegrationTest, RestartedTiersRunOutOfUnfoldsLikeTheirTwin) {
    std::mt19937 rng(43);
    std::deque<pud_module> twin_modules;
    std::deque<pud_module> restarted_modules;
    for (std::deque<pud_module>* modules : {&twin_modules, &restarted_modules}) {
        modules->emplace_back(5001, std::vector<pud_module_id>{}, std::vector<rule>{
            rule(this->functor("p", {this->atom("a")}), {}, 0),
            rule(this->functor("p", {this->atom("b")}), {}, 0),
        });
        modules->emplace_back(5002, std::vector<pud_module_id>{5001}, std::vector<rule>{
            rule(this->functor("r", {this->var(0)}), {this->functor("p", {this->var(0)})}, 1),
            rule(this->functor("r", {this->atom("c")}), {}, 0),
        });
        modules->emplace_back(5003, std::vector<pud_module_id>{5002}, std::vector<rule>{
            rule(this->functor("q", {this->var(0)}), {this->functor("r", {this->var(0)}), this->functor("p", {this->var(0)})}, 1),
            rule(this->functor("only_c", {this->var(0)}), {this->functor("r", {this->var(0)}), this->functor("s", {this->var(0)})}, 1),
        });

    }
    TypeParam twin_runtime(this->exprs);
    std::optional<TypeParam> restarted_runtime;
    restarted_runtime.emplace(this->exprs);
    std::set<pud_global_node_id> twin_live;
    std::set<pud_global_node_id> restarted_live;
    for (pud_module& module : twin_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = twin_runtime.register_module(module);
        twin_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            twin_live.erase(refuted);
    }
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }

{    size_t taken = 0;
    while (taken < 2 && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(0);
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
}{
    restarted_runtime.reset();
    restarted_runtime.emplace(this->exprs);
    restarted_live.clear();
    std::vector<pud_global_node_id> refuted_on_restart;
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        refuted_on_restart.insert(refuted_on_restart.end(), change.refuted.begin(), change.refuted.end());
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }
    EXPECT_EQ(restarted_live, twin_live);
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    EXPECT_THAT(refuted_on_restart, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
}
{    size_t taken = 0;
    while (taken < 2000 && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(0);
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    EXPECT_LT(taken, 2000u);
}
}
TYPED_TEST(PudRuntimeRestartIntegrationTest, RestartedSiblingModulesContinueLikeTheirTwin) {
    std::mt19937 rng(44);
    std::deque<pud_module> twin_modules;
    std::deque<pud_module> restarted_modules;
    for (std::deque<pud_module>* modules : {&twin_modules, &restarted_modules}) {
        modules->emplace_back(6001, std::vector<pud_module_id>{}, std::vector<rule>{
            rule(this->functor("nat", {this->atom("z")}), {}, 0),
            rule(this->functor("nat", {this->functor("s", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        });
        modules->emplace_back(6002, std::vector<pud_module_id>{6001}, std::vector<rule>{
            rule(this->functor("even", {this->atom("z")}), {}, 0),
            rule(this->functor("even", {this->functor("s", {this->functor("s", {this->var(0)})})}), {this->functor("even", {this->var(0)})}, 1),
        });
        modules->emplace_back(6003, std::vector<pud_module_id>{6001}, std::vector<rule>{
            rule(this->functor("odd", {this->functor("s", {this->atom("z")})}), {}, 0),
            rule(this->functor("odd", {this->functor("s", {this->functor("s", {this->var(0)})})}), {this->functor("odd", {this->var(0)})}, 1),
        });
        modules->emplace_back(6004, std::vector<pud_module_id>{6002, 6003}, std::vector<rule>{
            rule(this->functor("even_nat", {this->var(0)}), {this->functor("even", {this->var(0)}), this->functor("nat", {this->var(0)})}, 1),
            rule(this->functor("odd_nat", {this->var(0)}), {this->functor("odd", {this->var(0)}), this->functor("nat", {this->var(0)})}, 1),
        });

    }
    TypeParam twin_runtime(this->exprs);
    std::optional<TypeParam> restarted_runtime;
    restarted_runtime.emplace(this->exprs);
    std::set<pud_global_node_id> twin_live;
    std::set<pud_global_node_id> restarted_live;
    for (pud_module& module : twin_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = twin_runtime.register_module(module);
        twin_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            twin_live.erase(refuted);
    }
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }

{    size_t taken = 0;
    while (taken < 20 && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    ASSERT_EQ(taken, 20u);
}{
    restarted_runtime.reset();
    restarted_runtime.emplace(this->exprs);
    restarted_live.clear();
    std::vector<pud_global_node_id> refuted_on_restart;
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        refuted_on_restart.insert(refuted_on_restart.end(), change.refuted.begin(), change.refuted.end());
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }
    EXPECT_EQ(restarted_live, twin_live);
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    EXPECT_THAT(refuted_on_restart, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
}
{    size_t taken = 0;
    while (taken < 30 && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    EXPECT_EQ(taken, 30u);
}
}
TYPED_TEST(PudRuntimeRestartIntegrationTest, RestartedRefutationsCascadeLikeTheirTwin) {
    std::mt19937 rng(45);
    std::deque<pud_module> twin_modules;
    std::deque<pud_module> restarted_modules;
    for (std::deque<pud_module>* modules : {&twin_modules, &restarted_modules}) {
        modules->emplace_back(7001, std::vector<pud_module_id>{}, std::vector<rule>{
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

    }
    TypeParam twin_runtime(this->exprs);
    std::optional<TypeParam> restarted_runtime;
    restarted_runtime.emplace(this->exprs);
    std::set<pud_global_node_id> twin_live;
    std::set<pud_global_node_id> restarted_live;
    for (pud_module& module : twin_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = twin_runtime.register_module(module);
        twin_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            twin_live.erase(refuted);
    }
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }

{    size_t taken = 0;
    while (taken < 2 && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(0);
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
}{
    restarted_runtime.reset();
    restarted_runtime.emplace(this->exprs);
    restarted_live.clear();
    std::vector<pud_global_node_id> refuted_on_restart;
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        refuted_on_restart.insert(refuted_on_restart.end(), change.refuted.begin(), change.refuted.end());
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }
    EXPECT_EQ(restarted_live, twin_live);
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    EXPECT_THAT(refuted_on_restart, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
}
{    size_t taken = 0;
    while (taken < 2000 && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(0);
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    EXPECT_LT(taken, 2000u);
}
}
TYPED_TEST(PudRuntimeRestartIntegrationTest, AfterARestartAnUnfoldInADependencyRefutesTheDependentsLeavesLikeTheTwin) {
    std::deque<pud_module> twin_modules;
    std::deque<pud_module> restarted_modules;
    for (std::deque<pud_module>* modules : {&twin_modules, &restarted_modules}) {
        modules->emplace_back(8001, std::vector<pud_module_id>{}, std::vector<rule>{
            rule(this->functor("base", {this->atom("c")}), {}, 0),
            rule(this->functor("check", {this->atom("a")}), {}, 0),
            rule(this->functor("mid", {this->var(0)}), {this->functor("base", {this->var(0)}), this->functor("check", {this->var(0)})}, 1),
        });
        modules->emplace_back(8002, std::vector<pud_module_id>{8001}, std::vector<rule>{
            rule(this->atom("top"), {this->functor("mid", {this->atom("c")})}, 0),
            rule(this->atom("other"), {this->functor("check", {this->atom("a")})}, 0),
        });

    }
    TypeParam twin_runtime(this->exprs);
    std::optional<TypeParam> restarted_runtime;
    restarted_runtime.emplace(this->exprs);
    std::set<pud_global_node_id> twin_live;
    std::set<pud_global_node_id> restarted_live;
    for (pud_module& module : twin_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = twin_runtime.register_module(module);
        twin_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            twin_live.erase(refuted);
    }
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }

{
    restarted_runtime.reset();
    restarted_runtime.emplace(this->exprs);
    restarted_live.clear();
    std::vector<pud_global_node_id> refuted_on_restart;
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        refuted_on_restart.insert(refuted_on_restart.end(), change.refuted.begin(), change.refuted.end());
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }
    EXPECT_EQ(restarted_live, twin_live);
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    EXPECT_THAT(refuted_on_restart, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
}

    const pud_global_node_id mid{.module = 8001, .node = 2};
    const pud_frontier_change expected = twin_runtime.unfold(mid, 0);
    twin_live.erase(mid);
    twin_live.insert(expected.offspring.begin(), expected.offspring.end());
    for (pud_global_node_id refuted : expected.refuted)
        twin_live.erase(refuted);
    const pud_frontier_change actual = restarted_runtime->unfold(mid, 0);
    restarted_live.erase(mid);
    restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
    for (pud_global_node_id refuted : actual.refuted)
        restarted_live.erase(refuted);
    EXPECT_EQ(actual.offspring, expected.offspring);
    EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
    const pud_module* dep_module = nullptr;
    for (const pud_module& module : restarted_modules) {
        if (module.get_module_id() == 8002)
            dep_module = &module;
    }
    EXPECT_TRUE(dep_module->check_refuted(0));
    EXPECT_FALSE(dep_module->check_refuted(1));
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
}

TYPED_TEST(PudRuntimeRestartIntegrationTest, ARestartedRuntimeStoresNewInferencesAfterTheOnesAlreadyThere) {
    std::mt19937 rng(46);
    std::deque<pud_module> twin_modules;
    std::deque<pud_module> restarted_modules;
    for (std::deque<pud_module>* modules : {&twin_modules, &restarted_modules}) {
        modules->emplace_back(2001, std::vector<pud_module_id>{}, std::vector<rule>{
            rule(this->functor("nat", {this->atom("z")}), {}, 0),
            rule(this->functor("nat", {this->functor("s", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
            rule(this->functor("add", {this->atom("z"), this->var(0), this->var(0)}), {}, 1),
            rule(
                this->functor("add", {this->functor("s", {this->var(0)}), this->var(1), this->functor("s", {this->var(2)})}),
                {this->functor("add", {this->var(0), this->var(1), this->var(2)})},
                3),
        });
        modules->emplace_back(2002, std::vector<pud_module_id>{2001}, std::vector<rule>{
            rule(
                this->functor("half", {this->var(0)}),
                {this->functor("add", {this->var(0), this->var(0),
                    this->functor("s", {this->functor("s", {this->functor("s", {this->functor("s", {this->atom("z")})})})})})},
                1),
            rule(this->functor("number", {this->var(0)}), {this->functor("nat", {this->var(0)})}, 1),
        });

    }
    TypeParam twin_runtime(this->exprs);
    std::optional<TypeParam> restarted_runtime;
    restarted_runtime.emplace(this->exprs);
    std::set<pud_global_node_id> twin_live;
    std::set<pud_global_node_id> restarted_live;
    for (pud_module& module : twin_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = twin_runtime.register_module(module);
        twin_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            twin_live.erase(refuted);
    }
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }

{    size_t taken = 0;
    while (taken < 15 && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
}
{
    restarted_runtime.reset();
    restarted_runtime.emplace(this->exprs);
    restarted_live.clear();
    std::vector<pud_global_node_id> refuted_on_restart;
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        refuted_on_restart.insert(refuted_on_restart.end(), change.refuted.begin(), change.refuted.end());
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }
    EXPECT_EQ(restarted_live, twin_live);
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    EXPECT_THAT(refuted_on_restart, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
}

    std::vector<pud_global_node_id> unfoldable;
    for (pud_global_node_id leaf : twin_live) {
        if (twin_runtime.get_rule_information(leaf).body.empty())
            continue;
        unfoldable.push_back(leaf);
    }
    ASSERT_FALSE(unfoldable.empty());
    const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
    const pud_module* leaf_module = nullptr;
    for (const pud_module& module : twin_modules) {
        if (module.get_module_id() == leaf.module)
            leaf_module = &module;
    }
    std::vector<pud_body_goal_id> goal_ids;
    for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
        goal_ids.push_back(goal_id);
    std::sort(goal_ids.begin(), goal_ids.end());
    const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
    const pud_module* restarted_leaf_module = nullptr;
    for (const pud_module& module : restarted_modules) {
        if (module.get_module_id() == leaf.module)
            restarted_leaf_module = &module;
    }
    const size_t stored_before = reference_pud_count_nodes(*restarted_leaf_module);
    const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
    twin_live.erase(leaf);
    twin_live.insert(expected.offspring.begin(), expected.offspring.end());
    for (pud_global_node_id refuted : expected.refuted)
        twin_live.erase(refuted);
    const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
    restarted_live.erase(leaf);
    restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
    for (pud_global_node_id refuted : actual.refuted)
        restarted_live.erase(refuted);
    EXPECT_EQ(actual.offspring, expected.offspring);
    ASSERT_THAT(restarted_leaf_module->get_children(leaf.node), Not(IsEmpty()));
    for (pud_node_id child : restarted_leaf_module->get_children(leaf.node))
        EXPECT_GE(child, stored_before);
    EXPECT_EQ(reference_pud_count_nodes(*restarted_leaf_module), stored_before + restarted_leaf_module->get_children(leaf.node).size());
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
}

TYPED_TEST(PudRuntimeRestartIntegrationTest, RestartingNeverStoresAnInferenceOrMarksARefutation) {
    std::mt19937 rng(47);
    std::deque<pud_module> twin_modules;
    std::deque<pud_module> restarted_modules;
    for (std::deque<pud_module>* modules : {&twin_modules, &restarted_modules}) {
        modules->emplace_back(4001, std::vector<pud_module_id>{}, std::vector<rule>{
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
        modules->emplace_back(4002, std::vector<pud_module_id>{4001}, std::vector<rule>{
            rule(this->functor("reach", {this->var(0)}), {this->functor("path", {this->atom("a"), this->var(0)})}, 1),
            rule(this->atom("unreachable"), {this->functor("path", {this->atom("d"), this->atom("a")})}, 0),
        });

    }
    TypeParam twin_runtime(this->exprs);
    std::optional<TypeParam> restarted_runtime;
    restarted_runtime.emplace(this->exprs);
    std::set<pud_global_node_id> twin_live;
    std::set<pud_global_node_id> restarted_live;
    for (pud_module& module : twin_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = twin_runtime.register_module(module);
        twin_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            twin_live.erase(refuted);
    }
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }

{    size_t taken = 0;
    while (taken < 25 && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
}
    size_t stored_before = 0;
    for (const pud_module& module : restarted_modules)
        stored_before += reference_pud_count_nodes(module);
{
    restarted_runtime.reset();
    restarted_runtime.emplace(this->exprs);
    restarted_live.clear();
    std::vector<pud_global_node_id> refuted_on_restart;
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        refuted_on_restart.insert(refuted_on_restart.end(), change.refuted.begin(), change.refuted.end());
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }
    EXPECT_EQ(restarted_live, twin_live);
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    EXPECT_THAT(refuted_on_restart, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
}

    size_t stored_after = 0;
    for (const pud_module& module : restarted_modules)
        stored_after += reference_pud_count_nodes(module);
    EXPECT_EQ(stored_after, stored_before);
}

TYPED_TEST(PudRuntimeRestartIntegrationTest, RestartingTwiceWithoutUnfoldingChangesNothing) {
    std::mt19937 rng(48);
    std::deque<pud_module> twin_modules;
    std::deque<pud_module> restarted_modules;
    for (std::deque<pud_module>* modules : {&twin_modules, &restarted_modules}) {
        modules->emplace_back(3001, std::vector<pud_module_id>{}, std::vector<rule>{
            rule(this->functor("app", {this->atom("nil"), this->var(0), this->var(0)}), {}, 1),
            rule(
                this->functor("app", {this->functor("cons", {this->var(0), this->var(1)}), this->var(2), this->functor("cons", {this->var(0), this->var(3)})}),
                {this->functor("app", {this->var(1), this->var(2), this->var(3)})},
                4),
        });
        modules->emplace_back(3002, std::vector<pud_module_id>{3001}, std::vector<rule>{
            rule(
                this->functor("split", {this->var(0), this->var(1)}),
                {this->functor("app", {this->var(0), this->var(1), this->functor("cons", {this->atom("a"), this->functor("cons", {this->atom("b"), this->functor("cons", {this->atom("c"), this->atom("nil")})})})})},
                2),
            rule(
                this->functor("suffix", {this->var(0)}),
                {this->functor("app", {this->functor("cons", {this->atom("a"), this->atom("nil")}), this->var(0), this->functor("cons", {this->atom("a"), this->functor("cons", {this->atom("b"), this->atom("nil")})})})},
                1),
        });

    }
    TypeParam twin_runtime(this->exprs);
    std::optional<TypeParam> restarted_runtime;
    restarted_runtime.emplace(this->exprs);
    std::set<pud_global_node_id> twin_live;
    std::set<pud_global_node_id> restarted_live;
    for (pud_module& module : twin_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = twin_runtime.register_module(module);
        twin_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            twin_live.erase(refuted);
    }
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }

{    size_t taken = 0;
    while (taken < 20 && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
}
{
    restarted_runtime.reset();
    restarted_runtime.emplace(this->exprs);
    restarted_live.clear();
    std::vector<pud_global_node_id> refuted_on_restart;
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        refuted_on_restart.insert(refuted_on_restart.end(), change.refuted.begin(), change.refuted.end());
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }
    EXPECT_EQ(restarted_live, twin_live);
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    EXPECT_THAT(refuted_on_restart, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
}

{
    restarted_runtime.reset();
    restarted_runtime.emplace(this->exprs);
    restarted_live.clear();
    std::vector<pud_global_node_id> refuted_on_restart;
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        refuted_on_restart.insert(refuted_on_restart.end(), change.refuted.begin(), change.refuted.end());
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }
    EXPECT_EQ(restarted_live, twin_live);
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    EXPECT_THAT(refuted_on_restart, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
}

{    size_t taken = 0;
    while (taken < 10 && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    EXPECT_EQ(taken, 10u);
}
}

TYPED_TEST(PudRuntimeRestartIntegrationTest, RestartingAfterEveryUnfoldMatchesTheTwin) {
    std::mt19937 rng(49);
    std::deque<pud_module> twin_modules;
    std::deque<pud_module> restarted_modules;
    for (std::deque<pud_module>* modules : {&twin_modules, &restarted_modules}) {
        modules->emplace_back(2001, std::vector<pud_module_id>{}, std::vector<rule>{
            rule(this->functor("nat", {this->atom("z")}), {}, 0),
            rule(this->functor("nat", {this->functor("s", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
            rule(this->functor("add", {this->atom("z"), this->var(0), this->var(0)}), {}, 1),
            rule(
                this->functor("add", {this->functor("s", {this->var(0)}), this->var(1), this->functor("s", {this->var(2)})}),
                {this->functor("add", {this->var(0), this->var(1), this->var(2)})},
                3),
        });
        modules->emplace_back(2002, std::vector<pud_module_id>{2001}, std::vector<rule>{
            rule(
                this->functor("half", {this->var(0)}),
                {this->functor("add", {this->var(0), this->var(0),
                    this->functor("s", {this->functor("s", {this->functor("s", {this->functor("s", {this->atom("z")})})})})})},
                1),
            rule(this->functor("number", {this->var(0)}), {this->functor("nat", {this->var(0)})}, 1),
        });

    }
    TypeParam twin_runtime(this->exprs);
    std::optional<TypeParam> restarted_runtime;
    restarted_runtime.emplace(this->exprs);
    std::set<pud_global_node_id> twin_live;
    std::set<pud_global_node_id> restarted_live;
    for (pud_module& module : twin_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = twin_runtime.register_module(module);
        twin_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            twin_live.erase(refuted);
    }
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }

    for (int unfold_index = 0; unfold_index < 20 && !this->HasFailure(); ++unfold_index) {
    EXPECT_EQ(restarted_live, twin_live);
    std::vector<pud_global_node_id> unfoldable;
    for (pud_global_node_id leaf : twin_live) {
        if (twin_runtime.get_rule_information(leaf).body.empty())
            continue;
        unfoldable.push_back(leaf);
    }
    const bool had_step = !unfoldable.empty();
    if (had_step) {
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
    }

        ASSERT_TRUE(had_step) << "unfold " << unfold_index;
{
    restarted_runtime.reset();
    restarted_runtime.emplace(this->exprs);
    restarted_live.clear();
    std::vector<pud_global_node_id> refuted_on_restart;
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        refuted_on_restart.insert(refuted_on_restart.end(), change.refuted.begin(), change.refuted.end());
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }
    EXPECT_EQ(restarted_live, twin_live);
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    EXPECT_THAT(refuted_on_restart, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
}

    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
}

TYPED_TEST(PudRuntimeRestartIntegrationTest, RestartingEveryFewUnfoldsMatchesTheTwin) {
    std::mt19937 rng(50);
    std::deque<pud_module> twin_modules;
    std::deque<pud_module> restarted_modules;
    for (std::deque<pud_module>* modules : {&twin_modules, &restarted_modules}) {
        modules->emplace_back(6001, std::vector<pud_module_id>{}, std::vector<rule>{
            rule(this->functor("nat", {this->atom("z")}), {}, 0),
            rule(this->functor("nat", {this->functor("s", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        });
        modules->emplace_back(6002, std::vector<pud_module_id>{6001}, std::vector<rule>{
            rule(this->functor("even", {this->atom("z")}), {}, 0),
            rule(this->functor("even", {this->functor("s", {this->functor("s", {this->var(0)})})}), {this->functor("even", {this->var(0)})}, 1),
        });
        modules->emplace_back(6003, std::vector<pud_module_id>{6001}, std::vector<rule>{
            rule(this->functor("odd", {this->functor("s", {this->atom("z")})}), {}, 0),
            rule(this->functor("odd", {this->functor("s", {this->functor("s", {this->var(0)})})}), {this->functor("odd", {this->var(0)})}, 1),
        });
        modules->emplace_back(6004, std::vector<pud_module_id>{6002, 6003}, std::vector<rule>{
            rule(this->functor("even_nat", {this->var(0)}), {this->functor("even", {this->var(0)}), this->functor("nat", {this->var(0)})}, 1),
            rule(this->functor("odd_nat", {this->var(0)}), {this->functor("odd", {this->var(0)}), this->functor("nat", {this->var(0)})}, 1),
        });

    }
    TypeParam twin_runtime(this->exprs);
    std::optional<TypeParam> restarted_runtime;
    restarted_runtime.emplace(this->exprs);
    std::set<pud_global_node_id> twin_live;
    std::set<pud_global_node_id> restarted_live;
    for (pud_module& module : twin_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = twin_runtime.register_module(module);
        twin_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            twin_live.erase(refuted);
    }
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }

    for (int generation = 0; generation < 6 && !this->HasFailure(); ++generation) {
{    size_t taken = 0;
    while (taken < 6 && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
        EXPECT_EQ(taken, 6u);
}
{
    restarted_runtime.reset();
    restarted_runtime.emplace(this->exprs);
    restarted_live.clear();
    std::vector<pud_global_node_id> refuted_on_restart;
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        refuted_on_restart.insert(refuted_on_restart.end(), change.refuted.begin(), change.refuted.end());
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }
    EXPECT_EQ(restarted_live, twin_live);
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    EXPECT_THAT(refuted_on_restart, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
}

    }
}

TYPED_TEST(PudRuntimeRestartIntegrationTest, ARestartAndACopyReloadAgreeAndContinueAlike) {
    std::mt19937 rng(51);
    std::deque<pud_module> twin_modules;
    std::deque<pud_module> restarted_modules;
    for (std::deque<pud_module>* modules : {&twin_modules, &restarted_modules}) {
        modules->emplace_back(3001, std::vector<pud_module_id>{}, std::vector<rule>{
            rule(this->functor("app", {this->atom("nil"), this->var(0), this->var(0)}), {}, 1),
            rule(
                this->functor("app", {this->functor("cons", {this->var(0), this->var(1)}), this->var(2), this->functor("cons", {this->var(0), this->var(3)})}),
                {this->functor("app", {this->var(1), this->var(2), this->var(3)})},
                4),
        });
        modules->emplace_back(3002, std::vector<pud_module_id>{3001}, std::vector<rule>{
            rule(
                this->functor("split", {this->var(0), this->var(1)}),
                {this->functor("app", {this->var(0), this->var(1), this->functor("cons", {this->atom("a"), this->functor("cons", {this->atom("b"), this->functor("cons", {this->atom("c"), this->atom("nil")})})})})},
                2),
            rule(
                this->functor("suffix", {this->var(0)}),
                {this->functor("app", {this->functor("cons", {this->atom("a"), this->atom("nil")}), this->var(0), this->functor("cons", {this->atom("a"), this->functor("cons", {this->atom("b"), this->atom("nil")})})})},
                1),
        });

    }
    TypeParam twin_runtime(this->exprs);
    std::optional<TypeParam> restarted_runtime;
    restarted_runtime.emplace(this->exprs);
    std::set<pud_global_node_id> twin_live;
    std::set<pud_global_node_id> restarted_live;
    for (pud_module& module : twin_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = twin_runtime.register_module(module);
        twin_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            twin_live.erase(refuted);
    }
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }

{    size_t taken = 0;
    while (taken < 12 && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
}
    std::deque<pud_module> copy_modules(restarted_modules.begin(), restarted_modules.end());
    TypeParam copy_runtime(this->exprs);
    std::set<pud_global_node_id> copy_live;
    std::vector<pud_global_node_id> refuted_on_reload;
    for (pud_module& copy : copy_modules) {
        install_shared_functor_names(copy, this->functors);
        const pud_frontier_change change = copy_runtime.register_module(copy);
        refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        copy_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            copy_live.erase(refuted);
    }
    EXPECT_EQ(copy_live, restarted_live);
    EXPECT_EQ(copy_live, reference_pud_frontier(copy_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(copy_modules), IsEmpty());
    EXPECT_THAT(refuted_on_reload, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(restarted_modules, copy_modules), IsEmpty());
{
    restarted_runtime.reset();
    restarted_runtime.emplace(this->exprs);
    restarted_live.clear();
    std::vector<pud_global_node_id> refuted_on_restart;
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        refuted_on_restart.insert(refuted_on_restart.end(), change.refuted.begin(), change.refuted.end());
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }
    EXPECT_EQ(restarted_live, twin_live);
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    EXPECT_THAT(refuted_on_restart, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
}

    EXPECT_EQ(copy_live, restarted_live);
    for (int unfold_index = 0; unfold_index < 15 && !this->HasFailure(); ++unfold_index) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        ASSERT_FALSE(unfoldable.empty());
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change by_restart = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(by_restart.offspring.begin(), by_restart.offspring.end());
        for (pud_global_node_id refuted : by_restart.refuted)
            restarted_live.erase(refuted);
        const pud_frontier_change by_copy = copy_runtime.unfold(leaf, goal);
        copy_live.erase(leaf);
        copy_live.insert(by_copy.offspring.begin(), by_copy.offspring.end());
        for (pud_global_node_id refuted : by_copy.refuted)
            copy_live.erase(refuted);
        EXPECT_EQ(by_restart.offspring, expected.offspring);
        EXPECT_EQ(by_copy.offspring, expected.offspring);
        EXPECT_THAT(by_restart.refuted, UnorderedElementsAreArray(expected.refuted));
        EXPECT_THAT(by_copy.refuted, UnorderedElementsAreArray(expected.refuted));
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(twin_modules, copy_modules), IsEmpty());
}


TYPED_TEST(PudRuntimeRestartIntegrationTest, RestartedPeanoMulMatchesTwinUnderHeavyLoad) {
    std::mt19937 rng(80);
    std::deque<pud_module> twin_modules;
    std::deque<pud_module> restarted_modules;
    for (std::deque<pud_module>* modules : {&twin_modules, &restarted_modules}) {
        modules->emplace_back(9001, std::vector<pud_module_id>{}, std::vector<rule>{
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
        modules->emplace_back(9002, std::vector<pud_module_id>{9001}, std::vector<rule>{
            rule(
                this->functor("factor", {this->var(0), this->var(1)}),
                {this->functor("mul", {this->var(0), this->var(1), this->functor("s", {this->functor("s", {this->functor("s", {this->functor("s", {this->functor("s", {this->functor("s", {this->atom("z")})})})})})})})},
                2),
            rule(
                this->functor("triple", {this->var(0)}),
                {this->functor("mul", {this->functor("s", {this->functor("s", {this->functor("s", {this->atom("z")})})}), this->var(0), this->functor("s", {this->functor("s", {this->functor("s", {this->functor("s", {this->functor("s", {this->functor("s", {this->functor("s", {this->functor("s", {this->functor("s", {this->atom("z")})})})})})})})})})})},
                1),
            rule(
                this->functor("add_mul", {this->var(0), this->var(1), this->var(2)}),
                {this->functor("add", {this->var(0), this->var(1), this->var(2)}), this->functor("mul", {this->var(0), this->functor("s", {this->functor("s", {this->atom("z")})}), this->var(3)})},
                4),
        });

    }
    TypeParam twin_runtime(this->exprs);
    std::optional<TypeParam> restarted_runtime;
    restarted_runtime.emplace(this->exprs);
    std::set<pud_global_node_id> twin_live;
    std::set<pud_global_node_id> restarted_live;
    for (pud_module& module : twin_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = twin_runtime.register_module(module);
        twin_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            twin_live.erase(refuted);
    }
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }

{    size_t taken = 0;
    while (taken < 50 && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    ASSERT_EQ(taken, 50u);
}{
    restarted_runtime.reset();
    restarted_runtime.emplace(this->exprs);
    restarted_live.clear();
    std::vector<pud_global_node_id> refuted_on_restart;
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        refuted_on_restart.insert(refuted_on_restart.end(), change.refuted.begin(), change.refuted.end());
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }
    EXPECT_EQ(restarted_live, twin_live);
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    EXPECT_THAT(refuted_on_restart, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
}
{    size_t taken = 0;
    while (taken < 40 && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    EXPECT_EQ(taken, 40u);
}
}
TYPED_TEST(PudRuntimeRestartIntegrationTest, RestartedListMembersMatchTwinToExhaustion) {
    std::mt19937 rng(81);
    std::deque<pud_module> twin_modules;
    std::deque<pud_module> restarted_modules;
    for (std::deque<pud_module>* modules : {&twin_modules, &restarted_modules}) {
        modules->emplace_back(9101, std::vector<pud_module_id>{}, std::vector<rule>{
            rule(this->functor("mem", {this->var(0), this->functor("cons", {this->var(0), this->var(1)})}), {}, 2),
            rule(
                this->functor("mem", {this->var(0), this->functor("cons", {this->var(1), this->var(2)})}),
                {this->functor("mem", {this->var(0), this->var(2)})},
                3),
        });
        modules->emplace_back(9102, std::vector<pud_module_id>{9101}, std::vector<rule>{
            rule(
                this->functor("every", {this->var(0)}),
                {this->functor("mem", {this->var(0), this->functor("cons", {this->atom("x"), this->functor("cons", {this->atom("y"), this->functor("cons", {this->atom("z"), this->atom("nil")})})})})},
                1),
            rule(
                this->functor("pair_mem", {this->var(0), this->var(1)}),
                {
                    this->functor("mem", {this->var(0), this->functor("cons", {this->atom("a"), this->functor("cons", {this->atom("b"), this->functor("cons", {this->atom("c"), this->atom("nil")})})})}),
                    this->functor("mem", {this->var(1), this->functor("cons", {this->atom("b"), this->functor("cons", {this->atom("c"), this->functor("cons", {this->atom("d"), this->atom("nil")})})})}),
                },
                2),
        });

    }
    TypeParam twin_runtime(this->exprs);
    std::optional<TypeParam> restarted_runtime;
    restarted_runtime.emplace(this->exprs);
    std::set<pud_global_node_id> twin_live;
    std::set<pud_global_node_id> restarted_live;
    for (pud_module& module : twin_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = twin_runtime.register_module(module);
        twin_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            twin_live.erase(refuted);
    }
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }

{    size_t taken = 0;
    while (taken < 10 && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
}{
    restarted_runtime.reset();
    restarted_runtime.emplace(this->exprs);
    restarted_live.clear();
    std::vector<pud_global_node_id> refuted_on_restart;
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        refuted_on_restart.insert(refuted_on_restart.end(), change.refuted.begin(), change.refuted.end());
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }
    EXPECT_EQ(restarted_live, twin_live);
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    EXPECT_THAT(refuted_on_restart, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
}
{    size_t taken = 0;
    while (taken < 2000 && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (leaf.module != 9102)
                continue;
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(0);
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    EXPECT_LT(taken, 2000u);
}
}
TYPED_TEST(PudRuntimeRestartIntegrationTest, RestartedMirrorTreesMatchTwin) {
    std::mt19937 rng(82);
    std::deque<pud_module> twin_modules;
    std::deque<pud_module> restarted_modules;
    for (std::deque<pud_module>* modules : {&twin_modules, &restarted_modules}) {
        {
            const expr* sample = this->functor(
                "branch",
                {this->functor("leaf", {this->atom("l")}),
                 this->functor("branch", {this->functor("leaf", {this->atom("m")}), this->functor("leaf", {this->atom("r")})})});
            modules->emplace_back(9201, std::vector<pud_module_id>{}, std::vector<rule>{
                rule(this->functor("mirror", {this->functor("leaf", {this->var(0)}), this->functor("leaf", {this->var(0)})}), {}, 1),
                rule(
                    this->functor("mirror", {this->functor("branch", {this->var(0), this->var(1)}), this->functor("branch", {this->var(2), this->var(3)})}),
                    {this->functor("mirror", {this->var(0), this->var(3)}), this->functor("mirror", {this->var(1), this->var(2)})},
                    4),
            });
            modules->emplace_back(9202, std::vector<pud_module_id>{9201}, std::vector<rule>{
                rule(this->functor("flip", {this->var(0)}), {this->functor("mirror", {sample, this->var(0)})}, 1),
            });
        }

    }
    TypeParam twin_runtime(this->exprs);
    std::optional<TypeParam> restarted_runtime;
    restarted_runtime.emplace(this->exprs);
    std::set<pud_global_node_id> twin_live;
    std::set<pud_global_node_id> restarted_live;
    for (pud_module& module : twin_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = twin_runtime.register_module(module);
        twin_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            twin_live.erase(refuted);
    }
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }

{    size_t taken = 0;
    while (taken < 15 && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    ASSERT_EQ(taken, 15u);
}{
    restarted_runtime.reset();
    restarted_runtime.emplace(this->exprs);
    restarted_live.clear();
    std::vector<pud_global_node_id> refuted_on_restart;
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        refuted_on_restart.insert(refuted_on_restart.end(), change.refuted.begin(), change.refuted.end());
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }
    EXPECT_EQ(restarted_live, twin_live);
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    EXPECT_THAT(refuted_on_restart, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
}
{    size_t taken = 0;
    while (taken < 2000 && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (leaf.module != 9202)
                continue;
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(0);
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    EXPECT_LT(taken, 2000u);
}
}
TYPED_TEST(PudRuntimeRestartIntegrationTest, RestartedNestedUnaryChainMatchesTwin) {
    std::mt19937 rng(83);
    std::deque<pud_module> twin_modules;
    std::deque<pud_module> restarted_modules;
    for (std::deque<pud_module>* modules : {&twin_modules, &restarted_modules}) {
        {
            const expr* deep = this->atom("z");
            for (int level = 0; level < 8; ++level)
                deep = this->functor("s", {deep});
            const expr* inner = this->atom("z");
            for (int level = 0; level < 3; ++level)
                inner = this->functor("s", {inner});
            const expr* outer = inner;
            for (int level = 0; level < 2; ++level)
                outer = this->functor("s", {outer});
            const expr* reach_nat = this->atom("z");
            for (int level = 0; level < 5; ++level)
                reach_nat = this->functor("s", {reach_nat});
            const expr* peano_two = this->functor("s", {this->functor("s", {this->atom("z")})});
            const expr* peano_one = this->functor("s", {this->atom("z")});
            modules->emplace_back(9301, std::vector<pud_module_id>{}, std::vector<rule>{
                rule(this->functor("nat", {this->atom("z")}), {}, 0),
                rule(this->functor("nat", {this->functor("s", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
                rule(this->functor("deep_nat", {deep}), {this->functor("nat", {deep})}, 0),
                rule(
                    this->functor("shallow", {this->var(0)}),
                    {this->functor("nat", {this->var(0)}), this->functor("add", {this->var(0), peano_two, outer})},
                    1),
                rule(this->functor("add", {this->atom("z"), this->var(0), this->var(0)}), {}, 1),
                rule(
                    this->functor("add", {this->functor("s", {this->var(0)}), this->var(1), this->functor("s", {this->var(2)})}),
                    {this->functor("add", {this->var(0), this->var(1), this->var(2)})},
                    3),
            });
            modules->emplace_back(9302, std::vector<pud_module_id>{9301}, std::vector<rule>{
                rule(
                    this->functor("reach", {this->var(0)}),
                    {this->functor("nat", {reach_nat}), this->functor("add", {this->var(0), this->functor("s", {this->atom("z")}), reach_nat})},
                    1),
            });
        }

    }
    TypeParam twin_runtime(this->exprs);
    std::optional<TypeParam> restarted_runtime;
    restarted_runtime.emplace(this->exprs);
    std::set<pud_global_node_id> twin_live;
    std::set<pud_global_node_id> restarted_live;
    for (pud_module& module : twin_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = twin_runtime.register_module(module);
        twin_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            twin_live.erase(refuted);
    }
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }

{    size_t taken = 0;
    while (taken < 30 && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    ASSERT_EQ(taken, 30u);
}{
    restarted_runtime.reset();
    restarted_runtime.emplace(this->exprs);
    restarted_live.clear();
    std::vector<pud_global_node_id> refuted_on_restart;
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        refuted_on_restart.insert(refuted_on_restart.end(), change.refuted.begin(), change.refuted.end());
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }
    EXPECT_EQ(restarted_live, twin_live);
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    EXPECT_THAT(refuted_on_restart, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
}
{    size_t taken = 0;
    while (taken < 25 && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    EXPECT_EQ(taken, 25u);
}
}
TYPED_TEST(PudRuntimeRestartIntegrationTest, RestartedTripleAppendMatchesTwinUnderStress) {
    std::mt19937 rng(84);
    std::deque<pud_module> twin_modules;
    std::deque<pud_module> restarted_modules;
    for (std::deque<pud_module>* modules : {&twin_modules, &restarted_modules}) {
        modules->emplace_back(9401, std::vector<pud_module_id>{}, std::vector<rule>{
            rule(this->functor("app", {this->atom("nil"), this->var(0), this->var(0)}), {}, 1),
            rule(
                this->functor("app", {this->functor("cons", {this->var(0), this->var(1)}), this->var(2), this->functor("cons", {this->var(0), this->var(3)})}),
                {this->functor("app", {this->var(1), this->var(2), this->var(3)})},
                4),
        });
        modules->emplace_back(9402, std::vector<pud_module_id>{9401}, std::vector<rule>{
            rule(
                this->atom("triple_split"),
                {
                    this->functor("app", {this->var(0), this->var(1), this->functor("cons", {this->atom("a"), this->functor("cons", {this->atom("b"), this->functor("cons", {this->atom("c"), this->atom("nil")})})})}),
                    this->functor("app", {this->var(1), this->var(2), this->functor("cons", {this->atom("d"), this->functor("cons", {this->atom("e"), this->atom("nil")})})}),
                    this->functor("app", {this->var(0), this->var(2), this->var(3)}),
                },
                4),
            rule(
                this->functor("nested_app", {this->var(0)}),
                {
                    this->functor("app", {this->var(0), this->var(1), this->functor("cons", {this->atom("p"), this->functor("cons", {this->atom("q"), this->atom("nil")})})}),
                    this->functor("app", {this->var(1), this->functor("cons", {this->atom("r"), this->atom("nil")}), this->functor("cons", {this->atom("p"), this->functor("cons", {this->atom("q"), this->functor("cons", {this->atom("r"), this->atom("nil")})})})}),
                },
                2),
        });

    }
    TypeParam twin_runtime(this->exprs);
    std::optional<TypeParam> restarted_runtime;
    restarted_runtime.emplace(this->exprs);
    std::set<pud_global_node_id> twin_live;
    std::set<pud_global_node_id> restarted_live;
    for (pud_module& module : twin_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = twin_runtime.register_module(module);
        twin_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            twin_live.erase(refuted);
    }
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }

{    size_t taken = 0;
    while (taken < 45 && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    ASSERT_EQ(taken, 45u);
}{
    restarted_runtime.reset();
    restarted_runtime.emplace(this->exprs);
    restarted_live.clear();
    std::vector<pud_global_node_id> refuted_on_restart;
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        refuted_on_restart.insert(refuted_on_restart.end(), change.refuted.begin(), change.refuted.end());
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }
    EXPECT_EQ(restarted_live, twin_live);
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    EXPECT_THAT(refuted_on_restart, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
}
    for (int generation = 0; generation < 4 && !this->HasFailure(); ++generation) {
{    size_t taken = 0;
    while (taken < 12 && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
        EXPECT_EQ(taken, 12u);
}{
    restarted_runtime.reset();
    restarted_runtime.emplace(this->exprs);
    restarted_live.clear();
    std::vector<pud_global_node_id> refuted_on_restart;
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        refuted_on_restart.insert(refuted_on_restart.end(), change.refuted.begin(), change.refuted.end());
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }
    EXPECT_EQ(restarted_live, twin_live);
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    EXPECT_THAT(refuted_on_restart, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
}
    }

}
TYPED_TEST(PudRuntimeRestartIntegrationTest, RestartedSharedBinaryMatchesTwin) {
    std::mt19937 rng(85);
    std::deque<pud_module> twin_modules;
    std::deque<pud_module> restarted_modules;
    for (std::deque<pud_module>* modules : {&twin_modules, &restarted_modules}) {
        {
            const expr* shared = this->atom("v");
            for (int level = 0; level < 4; ++level)
                shared = this->functor("pair", {shared, shared});
            const expr* peano_two = this->functor("s", {this->functor("s", {this->atom("z")})});
            modules->emplace_back(9501, std::vector<pud_module_id>{}, std::vector<rule>{
                rule(this->functor("nat", {this->atom("z")}), {}, 0),
                rule(this->functor("nat", {this->functor("s", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
                rule(this->functor("unpack", {shared, this->var(0)}), {this->functor("same", {this->var(0), shared})}, 1),
                rule(this->functor("same", {this->var(0), this->var(0)}), {}, 1),
                rule(
                    this->functor("both", {this->var(0), this->var(1)}),
                    {this->functor("same", {this->var(0), this->var(1)}), this->functor("nat", {this->var(0)})},
                    2),
            });
            modules->emplace_back(9502, std::vector<pud_module_id>{9501}, std::vector<rule>{
                rule(this->functor("grounded", {this->var(0)}), {this->functor("unpack", {shared, this->var(0)})}, 1),
                rule(this->functor("nat_query", {this->var(0)}), {this->functor("both", {peano_two, this->var(0)})}, 1),
            });
        }

    }
    TypeParam twin_runtime(this->exprs);
    std::optional<TypeParam> restarted_runtime;
    restarted_runtime.emplace(this->exprs);
    std::set<pud_global_node_id> twin_live;
    std::set<pud_global_node_id> restarted_live;
    for (pud_module& module : twin_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = twin_runtime.register_module(module);
        twin_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            twin_live.erase(refuted);
    }
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }

{    size_t taken = 0;
    while (taken < 25 && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    ASSERT_EQ(taken, 25u);
}{
    restarted_runtime.reset();
    restarted_runtime.emplace(this->exprs);
    restarted_live.clear();
    std::vector<pud_global_node_id> refuted_on_restart;
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        refuted_on_restart.insert(refuted_on_restart.end(), change.refuted.begin(), change.refuted.end());
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }
    EXPECT_EQ(restarted_live, twin_live);
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    EXPECT_THAT(refuted_on_restart, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
}
{    size_t taken = 0;
    while (taken < 20 && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    EXPECT_EQ(taken, 20u);
}
}
TYPED_TEST(PudRuntimeRestartIntegrationTest, ColorsRestartsAndContinuesLikeItsTwin) {
    for (uint32_t seed = 0; seed < 5 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(200 + seed);
    std::deque<pud_module> twin_modules;
    std::deque<pud_module> restarted_modules;
    for (std::deque<pud_module>* modules : {&twin_modules, &restarted_modules}) {
        modules->emplace_back(1001, std::vector<pud_module_id>{}, std::vector<rule>{
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

    }
    TypeParam twin_runtime(this->exprs);
    std::optional<TypeParam> restarted_runtime;
    restarted_runtime.emplace(this->exprs);
    std::set<pud_global_node_id> twin_live;
    std::set<pud_global_node_id> restarted_live;
    for (pud_module& module : twin_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = twin_runtime.register_module(module);
        twin_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            twin_live.erase(refuted);
    }
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }

{    const size_t unfold_budget = rng() % 25;
    size_t taken = 0;
    while (taken < unfold_budget && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
}
{
    restarted_runtime.reset();
    restarted_runtime.emplace(this->exprs);
    restarted_live.clear();
    std::vector<pud_global_node_id> refuted_on_restart;
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        refuted_on_restart.insert(refuted_on_restart.end(), change.refuted.begin(), change.refuted.end());
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }
    EXPECT_EQ(restarted_live, twin_live);
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    EXPECT_THAT(refuted_on_restart, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
}

{    size_t taken = 0;
    while (taken < 20 && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
}
        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 1001;
            return;
        }
    }
}

TYPED_TEST(PudRuntimeRestartIntegrationTest, ArithmeticRestartsAndContinuesLikeItsTwin) {
    for (uint32_t seed = 0; seed < 5 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(200 + seed);
    std::deque<pud_module> twin_modules;
    std::deque<pud_module> restarted_modules;
    for (std::deque<pud_module>* modules : {&twin_modules, &restarted_modules}) {
        modules->emplace_back(2001, std::vector<pud_module_id>{}, std::vector<rule>{
            rule(this->functor("nat", {this->atom("z")}), {}, 0),
            rule(this->functor("nat", {this->functor("s", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
            rule(this->functor("add", {this->atom("z"), this->var(0), this->var(0)}), {}, 1),
            rule(
                this->functor("add", {this->functor("s", {this->var(0)}), this->var(1), this->functor("s", {this->var(2)})}),
                {this->functor("add", {this->var(0), this->var(1), this->var(2)})},
                3),
        });
        modules->emplace_back(2002, std::vector<pud_module_id>{2001}, std::vector<rule>{
            rule(
                this->functor("half", {this->var(0)}),
                {this->functor("add", {this->var(0), this->var(0),
                    this->functor("s", {this->functor("s", {this->functor("s", {this->functor("s", {this->atom("z")})})})})})},
                1),
            rule(this->functor("number", {this->var(0)}), {this->functor("nat", {this->var(0)})}, 1),
        });

    }
    TypeParam twin_runtime(this->exprs);
    std::optional<TypeParam> restarted_runtime;
    restarted_runtime.emplace(this->exprs);
    std::set<pud_global_node_id> twin_live;
    std::set<pud_global_node_id> restarted_live;
    for (pud_module& module : twin_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = twin_runtime.register_module(module);
        twin_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            twin_live.erase(refuted);
    }
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }

{    const size_t unfold_budget = rng() % 25;
    size_t taken = 0;
    while (taken < unfold_budget && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
}
{
    restarted_runtime.reset();
    restarted_runtime.emplace(this->exprs);
    restarted_live.clear();
    std::vector<pud_global_node_id> refuted_on_restart;
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        refuted_on_restart.insert(refuted_on_restart.end(), change.refuted.begin(), change.refuted.end());
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }
    EXPECT_EQ(restarted_live, twin_live);
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    EXPECT_THAT(refuted_on_restart, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
}

{    size_t taken = 0;
    while (taken < 20 && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
}
        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 2001;
            return;
        }
    }
}

TYPED_TEST(PudRuntimeRestartIntegrationTest, ListsRestartsAndContinuesLikeItsTwin) {
    for (uint32_t seed = 0; seed < 5 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(200 + seed);
    std::deque<pud_module> twin_modules;
    std::deque<pud_module> restarted_modules;
    for (std::deque<pud_module>* modules : {&twin_modules, &restarted_modules}) {
        modules->emplace_back(3001, std::vector<pud_module_id>{}, std::vector<rule>{
            rule(this->functor("app", {this->atom("nil"), this->var(0), this->var(0)}), {}, 1),
            rule(
                this->functor("app", {this->functor("cons", {this->var(0), this->var(1)}), this->var(2), this->functor("cons", {this->var(0), this->var(3)})}),
                {this->functor("app", {this->var(1), this->var(2), this->var(3)})},
                4),
        });
        modules->emplace_back(3002, std::vector<pud_module_id>{3001}, std::vector<rule>{
            rule(
                this->functor("split", {this->var(0), this->var(1)}),
                {this->functor("app", {this->var(0), this->var(1), this->functor("cons", {this->atom("a"), this->functor("cons", {this->atom("b"), this->functor("cons", {this->atom("c"), this->atom("nil")})})})})},
                2),
            rule(
                this->functor("suffix", {this->var(0)}),
                {this->functor("app", {this->functor("cons", {this->atom("a"), this->atom("nil")}), this->var(0), this->functor("cons", {this->atom("a"), this->functor("cons", {this->atom("b"), this->atom("nil")})})})},
                1),
        });

    }
    TypeParam twin_runtime(this->exprs);
    std::optional<TypeParam> restarted_runtime;
    restarted_runtime.emplace(this->exprs);
    std::set<pud_global_node_id> twin_live;
    std::set<pud_global_node_id> restarted_live;
    for (pud_module& module : twin_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = twin_runtime.register_module(module);
        twin_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            twin_live.erase(refuted);
    }
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }

{    const size_t unfold_budget = rng() % 25;
    size_t taken = 0;
    while (taken < unfold_budget && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
}
{
    restarted_runtime.reset();
    restarted_runtime.emplace(this->exprs);
    restarted_live.clear();
    std::vector<pud_global_node_id> refuted_on_restart;
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        refuted_on_restart.insert(refuted_on_restart.end(), change.refuted.begin(), change.refuted.end());
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }
    EXPECT_EQ(restarted_live, twin_live);
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    EXPECT_THAT(refuted_on_restart, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
}

{    size_t taken = 0;
    while (taken < 20 && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
}
        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 3001;
            return;
        }
    }
}

TYPED_TEST(PudRuntimeRestartIntegrationTest, GraphRestartsAndContinuesLikeItsTwin) {
    for (uint32_t seed = 0; seed < 5 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(200 + seed);
    std::deque<pud_module> twin_modules;
    std::deque<pud_module> restarted_modules;
    for (std::deque<pud_module>* modules : {&twin_modules, &restarted_modules}) {
        modules->emplace_back(4001, std::vector<pud_module_id>{}, std::vector<rule>{
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
        modules->emplace_back(4002, std::vector<pud_module_id>{4001}, std::vector<rule>{
            rule(this->functor("reach", {this->var(0)}), {this->functor("path", {this->atom("a"), this->var(0)})}, 1),
            rule(this->atom("unreachable"), {this->functor("path", {this->atom("d"), this->atom("a")})}, 0),
        });

    }
    TypeParam twin_runtime(this->exprs);
    std::optional<TypeParam> restarted_runtime;
    restarted_runtime.emplace(this->exprs);
    std::set<pud_global_node_id> twin_live;
    std::set<pud_global_node_id> restarted_live;
    for (pud_module& module : twin_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = twin_runtime.register_module(module);
        twin_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            twin_live.erase(refuted);
    }
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }

{    const size_t unfold_budget = rng() % 25;
    size_t taken = 0;
    while (taken < unfold_budget && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
}
{
    restarted_runtime.reset();
    restarted_runtime.emplace(this->exprs);
    restarted_live.clear();
    std::vector<pud_global_node_id> refuted_on_restart;
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        refuted_on_restart.insert(refuted_on_restart.end(), change.refuted.begin(), change.refuted.end());
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }
    EXPECT_EQ(restarted_live, twin_live);
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    EXPECT_THAT(refuted_on_restart, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
}

{    size_t taken = 0;
    while (taken < 20 && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
}
        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 4001;
            return;
        }
    }
}

TYPED_TEST(PudRuntimeRestartIntegrationTest, TiersRestartsAndContinuesLikeItsTwin) {
    for (uint32_t seed = 0; seed < 5 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(200 + seed);
    std::deque<pud_module> twin_modules;
    std::deque<pud_module> restarted_modules;
    for (std::deque<pud_module>* modules : {&twin_modules, &restarted_modules}) {
        modules->emplace_back(5001, std::vector<pud_module_id>{}, std::vector<rule>{
            rule(this->functor("p", {this->atom("a")}), {}, 0),
            rule(this->functor("p", {this->atom("b")}), {}, 0),
        });
        modules->emplace_back(5002, std::vector<pud_module_id>{5001}, std::vector<rule>{
            rule(this->functor("r", {this->var(0)}), {this->functor("p", {this->var(0)})}, 1),
            rule(this->functor("r", {this->atom("c")}), {}, 0),
        });
        modules->emplace_back(5003, std::vector<pud_module_id>{5002}, std::vector<rule>{
            rule(this->functor("q", {this->var(0)}), {this->functor("r", {this->var(0)}), this->functor("p", {this->var(0)})}, 1),
            rule(this->functor("only_c", {this->var(0)}), {this->functor("r", {this->var(0)}), this->functor("s", {this->var(0)})}, 1),
        });

    }
    TypeParam twin_runtime(this->exprs);
    std::optional<TypeParam> restarted_runtime;
    restarted_runtime.emplace(this->exprs);
    std::set<pud_global_node_id> twin_live;
    std::set<pud_global_node_id> restarted_live;
    for (pud_module& module : twin_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = twin_runtime.register_module(module);
        twin_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            twin_live.erase(refuted);
    }
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }

{    const size_t unfold_budget = rng() % 25;
    size_t taken = 0;
    while (taken < unfold_budget && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
}
{
    restarted_runtime.reset();
    restarted_runtime.emplace(this->exprs);
    restarted_live.clear();
    std::vector<pud_global_node_id> refuted_on_restart;
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        refuted_on_restart.insert(refuted_on_restart.end(), change.refuted.begin(), change.refuted.end());
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }
    EXPECT_EQ(restarted_live, twin_live);
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    EXPECT_THAT(refuted_on_restart, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
}

{    size_t taken = 0;
    while (taken < 20 && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
}
        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 5001;
            return;
        }
    }
}

TYPED_TEST(PudRuntimeRestartIntegrationTest, SiblingsRestartsAndContinuesLikeItsTwin) {
    for (uint32_t seed = 0; seed < 5 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(200 + seed);
    std::deque<pud_module> twin_modules;
    std::deque<pud_module> restarted_modules;
    for (std::deque<pud_module>* modules : {&twin_modules, &restarted_modules}) {
        modules->emplace_back(6001, std::vector<pud_module_id>{}, std::vector<rule>{
            rule(this->functor("nat", {this->atom("z")}), {}, 0),
            rule(this->functor("nat", {this->functor("s", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        });
        modules->emplace_back(6002, std::vector<pud_module_id>{6001}, std::vector<rule>{
            rule(this->functor("even", {this->atom("z")}), {}, 0),
            rule(this->functor("even", {this->functor("s", {this->functor("s", {this->var(0)})})}), {this->functor("even", {this->var(0)})}, 1),
        });
        modules->emplace_back(6003, std::vector<pud_module_id>{6001}, std::vector<rule>{
            rule(this->functor("odd", {this->functor("s", {this->atom("z")})}), {}, 0),
            rule(this->functor("odd", {this->functor("s", {this->functor("s", {this->var(0)})})}), {this->functor("odd", {this->var(0)})}, 1),
        });
        modules->emplace_back(6004, std::vector<pud_module_id>{6002, 6003}, std::vector<rule>{
            rule(this->functor("even_nat", {this->var(0)}), {this->functor("even", {this->var(0)}), this->functor("nat", {this->var(0)})}, 1),
            rule(this->functor("odd_nat", {this->var(0)}), {this->functor("odd", {this->var(0)}), this->functor("nat", {this->var(0)})}, 1),
        });

    }
    TypeParam twin_runtime(this->exprs);
    std::optional<TypeParam> restarted_runtime;
    restarted_runtime.emplace(this->exprs);
    std::set<pud_global_node_id> twin_live;
    std::set<pud_global_node_id> restarted_live;
    for (pud_module& module : twin_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = twin_runtime.register_module(module);
        twin_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            twin_live.erase(refuted);
    }
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }

{    const size_t unfold_budget = rng() % 25;
    size_t taken = 0;
    while (taken < unfold_budget && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
}
{
    restarted_runtime.reset();
    restarted_runtime.emplace(this->exprs);
    restarted_live.clear();
    std::vector<pud_global_node_id> refuted_on_restart;
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        refuted_on_restart.insert(refuted_on_restart.end(), change.refuted.begin(), change.refuted.end());
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }
    EXPECT_EQ(restarted_live, twin_live);
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    EXPECT_THAT(refuted_on_restart, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
}

{    size_t taken = 0;
    while (taken < 20 && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
}
        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 6001;
            return;
        }
    }
}

TYPED_TEST(PudRuntimeRestartIntegrationTest, RefutationsRestartsAndContinuesLikeItsTwin) {
    for (uint32_t seed = 0; seed < 5 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(200 + seed);
    std::deque<pud_module> twin_modules;
    std::deque<pud_module> restarted_modules;
    for (std::deque<pud_module>* modules : {&twin_modules, &restarted_modules}) {
        modules->emplace_back(7001, std::vector<pud_module_id>{}, std::vector<rule>{
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

    }
    TypeParam twin_runtime(this->exprs);
    std::optional<TypeParam> restarted_runtime;
    restarted_runtime.emplace(this->exprs);
    std::set<pud_global_node_id> twin_live;
    std::set<pud_global_node_id> restarted_live;
    for (pud_module& module : twin_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = twin_runtime.register_module(module);
        twin_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            twin_live.erase(refuted);
    }
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }

{    const size_t unfold_budget = rng() % 25;
    size_t taken = 0;
    while (taken < unfold_budget && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
}
{
    restarted_runtime.reset();
    restarted_runtime.emplace(this->exprs);
    restarted_live.clear();
    std::vector<pud_global_node_id> refuted_on_restart;
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        refuted_on_restart.insert(refuted_on_restart.end(), change.refuted.begin(), change.refuted.end());
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }
    EXPECT_EQ(restarted_live, twin_live);
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    EXPECT_THAT(refuted_on_restart, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
}

{    size_t taken = 0;
    while (taken < 20 && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
}
        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 7001;
            return;
        }
    }
}

TYPED_TEST(PudRuntimeRestartIntegrationTest, CascadeRestartsAndContinuesLikeItsTwin) {
    for (uint32_t seed = 0; seed < 5 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(200 + seed);
    std::deque<pud_module> twin_modules;
    std::deque<pud_module> restarted_modules;
    for (std::deque<pud_module>* modules : {&twin_modules, &restarted_modules}) {
        modules->emplace_back(8001, std::vector<pud_module_id>{}, std::vector<rule>{
            rule(this->functor("base", {this->atom("c")}), {}, 0),
            rule(this->functor("check", {this->atom("a")}), {}, 0),
            rule(this->functor("mid", {this->var(0)}), {this->functor("base", {this->var(0)}), this->functor("check", {this->var(0)})}, 1),
        });
        modules->emplace_back(8002, std::vector<pud_module_id>{8001}, std::vector<rule>{
            rule(this->atom("top"), {this->functor("mid", {this->atom("c")})}, 0),
            rule(this->atom("other"), {this->functor("check", {this->atom("a")})}, 0),
        });

    }
    TypeParam twin_runtime(this->exprs);
    std::optional<TypeParam> restarted_runtime;
    restarted_runtime.emplace(this->exprs);
    std::set<pud_global_node_id> twin_live;
    std::set<pud_global_node_id> restarted_live;
    for (pud_module& module : twin_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = twin_runtime.register_module(module);
        twin_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            twin_live.erase(refuted);
    }
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }

{    const size_t unfold_budget = rng() % 25;
    size_t taken = 0;
    while (taken < unfold_budget && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
}
{
    restarted_runtime.reset();
    restarted_runtime.emplace(this->exprs);
    restarted_live.clear();
    std::vector<pud_global_node_id> refuted_on_restart;
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        refuted_on_restart.insert(refuted_on_restart.end(), change.refuted.begin(), change.refuted.end());
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }
    EXPECT_EQ(restarted_live, twin_live);
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    EXPECT_THAT(refuted_on_restart, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
}

{    size_t taken = 0;
    while (taken < 20 && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
}
        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 8001;
            return;
        }
    }
}

TYPED_TEST(PudRuntimeRestartIntegrationTest, PeanoMulRestartsAndContinuesLikeItsTwin) {
    for (uint32_t seed = 0; seed < 5 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(200 + seed);
    std::deque<pud_module> twin_modules;
    std::deque<pud_module> restarted_modules;
    for (std::deque<pud_module>* modules : {&twin_modules, &restarted_modules}) {
        modules->emplace_back(9001, std::vector<pud_module_id>{}, std::vector<rule>{
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
        modules->emplace_back(9002, std::vector<pud_module_id>{9001}, std::vector<rule>{
            rule(
                this->functor("factor", {this->var(0), this->var(1)}),
                {this->functor("mul", {this->var(0), this->var(1), this->functor("s", {this->functor("s", {this->functor("s", {this->functor("s", {this->functor("s", {this->functor("s", {this->atom("z")})})})})})})})},
                2),
            rule(
                this->functor("triple", {this->var(0)}),
                {this->functor("mul", {this->functor("s", {this->functor("s", {this->functor("s", {this->atom("z")})})}), this->var(0), this->functor("s", {this->functor("s", {this->functor("s", {this->functor("s", {this->functor("s", {this->functor("s", {this->functor("s", {this->functor("s", {this->functor("s", {this->atom("z")})})})})})})})})})})},
                1),
            rule(
                this->functor("add_mul", {this->var(0), this->var(1), this->var(2)}),
                {this->functor("add", {this->var(0), this->var(1), this->var(2)}), this->functor("mul", {this->var(0), this->functor("s", {this->functor("s", {this->atom("z")})}), this->var(3)})},
                4),
        });

    }
    TypeParam twin_runtime(this->exprs);
    std::optional<TypeParam> restarted_runtime;
    restarted_runtime.emplace(this->exprs);
    std::set<pud_global_node_id> twin_live;
    std::set<pud_global_node_id> restarted_live;
    for (pud_module& module : twin_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = twin_runtime.register_module(module);
        twin_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            twin_live.erase(refuted);
    }
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }

{    const size_t unfold_budget = rng() % 25;
    size_t taken = 0;
    while (taken < unfold_budget && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
}
{
    restarted_runtime.reset();
    restarted_runtime.emplace(this->exprs);
    restarted_live.clear();
    std::vector<pud_global_node_id> refuted_on_restart;
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        refuted_on_restart.insert(refuted_on_restart.end(), change.refuted.begin(), change.refuted.end());
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }
    EXPECT_EQ(restarted_live, twin_live);
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    EXPECT_THAT(refuted_on_restart, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
}

{    size_t taken = 0;
    while (taken < 20 && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
}
        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 9001;
            return;
        }
    }
}

TYPED_TEST(PudRuntimeRestartIntegrationTest, ListMembersRestartsAndContinuesLikeItsTwin) {
    for (uint32_t seed = 0; seed < 5 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(200 + seed);
    std::deque<pud_module> twin_modules;
    std::deque<pud_module> restarted_modules;
    for (std::deque<pud_module>* modules : {&twin_modules, &restarted_modules}) {
        modules->emplace_back(9101, std::vector<pud_module_id>{}, std::vector<rule>{
            rule(this->functor("mem", {this->var(0), this->functor("cons", {this->var(0), this->var(1)})}), {}, 2),
            rule(
                this->functor("mem", {this->var(0), this->functor("cons", {this->var(1), this->var(2)})}),
                {this->functor("mem", {this->var(0), this->var(2)})},
                3),
        });
        modules->emplace_back(9102, std::vector<pud_module_id>{9101}, std::vector<rule>{
            rule(
                this->functor("every", {this->var(0)}),
                {this->functor("mem", {this->var(0), this->functor("cons", {this->atom("x"), this->functor("cons", {this->atom("y"), this->functor("cons", {this->atom("z"), this->atom("nil")})})})})},
                1),
            rule(
                this->functor("pair_mem", {this->var(0), this->var(1)}),
                {
                    this->functor("mem", {this->var(0), this->functor("cons", {this->atom("a"), this->functor("cons", {this->atom("b"), this->functor("cons", {this->atom("c"), this->atom("nil")})})})}),
                    this->functor("mem", {this->var(1), this->functor("cons", {this->atom("b"), this->functor("cons", {this->atom("c"), this->functor("cons", {this->atom("d"), this->atom("nil")})})})}),
                },
                2),
        });

    }
    TypeParam twin_runtime(this->exprs);
    std::optional<TypeParam> restarted_runtime;
    restarted_runtime.emplace(this->exprs);
    std::set<pud_global_node_id> twin_live;
    std::set<pud_global_node_id> restarted_live;
    for (pud_module& module : twin_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = twin_runtime.register_module(module);
        twin_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            twin_live.erase(refuted);
    }
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }

{    const size_t unfold_budget = rng() % 25;
    size_t taken = 0;
    while (taken < unfold_budget && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
}
{
    restarted_runtime.reset();
    restarted_runtime.emplace(this->exprs);
    restarted_live.clear();
    std::vector<pud_global_node_id> refuted_on_restart;
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        refuted_on_restart.insert(refuted_on_restart.end(), change.refuted.begin(), change.refuted.end());
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }
    EXPECT_EQ(restarted_live, twin_live);
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    EXPECT_THAT(refuted_on_restart, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
}

{    size_t taken = 0;
    while (taken < 20 && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
}
        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 9101;
            return;
        }
    }
}

TYPED_TEST(PudRuntimeRestartIntegrationTest, MirrorTreesRestartsAndContinuesLikeItsTwin) {
    for (uint32_t seed = 0; seed < 5 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(200 + seed);
    std::deque<pud_module> twin_modules;
    std::deque<pud_module> restarted_modules;
    for (std::deque<pud_module>* modules : {&twin_modules, &restarted_modules}) {
        {
            const expr* sample = this->functor(
                "branch",
                {this->functor("leaf", {this->atom("l")}),
                 this->functor("branch", {this->functor("leaf", {this->atom("m")}), this->functor("leaf", {this->atom("r")})})});
            modules->emplace_back(9201, std::vector<pud_module_id>{}, std::vector<rule>{
                rule(this->functor("mirror", {this->functor("leaf", {this->var(0)}), this->functor("leaf", {this->var(0)})}), {}, 1),
                rule(
                    this->functor("mirror", {this->functor("branch", {this->var(0), this->var(1)}), this->functor("branch", {this->var(2), this->var(3)})}),
                    {this->functor("mirror", {this->var(0), this->var(3)}), this->functor("mirror", {this->var(1), this->var(2)})},
                    4),
            });
            modules->emplace_back(9202, std::vector<pud_module_id>{9201}, std::vector<rule>{
                rule(this->functor("flip", {this->var(0)}), {this->functor("mirror", {sample, this->var(0)})}, 1),
            });
        }

    }
    TypeParam twin_runtime(this->exprs);
    std::optional<TypeParam> restarted_runtime;
    restarted_runtime.emplace(this->exprs);
    std::set<pud_global_node_id> twin_live;
    std::set<pud_global_node_id> restarted_live;
    for (pud_module& module : twin_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = twin_runtime.register_module(module);
        twin_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            twin_live.erase(refuted);
    }
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }

{    const size_t unfold_budget = rng() % 25;
    size_t taken = 0;
    while (taken < unfold_budget && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
}
{
    restarted_runtime.reset();
    restarted_runtime.emplace(this->exprs);
    restarted_live.clear();
    std::vector<pud_global_node_id> refuted_on_restart;
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        refuted_on_restart.insert(refuted_on_restart.end(), change.refuted.begin(), change.refuted.end());
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }
    EXPECT_EQ(restarted_live, twin_live);
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    EXPECT_THAT(refuted_on_restart, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
}

{    size_t taken = 0;
    while (taken < 20 && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
}
        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 9201;
            return;
        }
    }
}

TYPED_TEST(PudRuntimeRestartIntegrationTest, NestedUnaryChainRestartsAndContinuesLikeItsTwin) {
    for (uint32_t seed = 0; seed < 5 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(200 + seed);
    std::deque<pud_module> twin_modules;
    std::deque<pud_module> restarted_modules;
    for (std::deque<pud_module>* modules : {&twin_modules, &restarted_modules}) {
        {
            const expr* deep = this->atom("z");
            for (int level = 0; level < 8; ++level)
                deep = this->functor("s", {deep});
            const expr* inner = this->atom("z");
            for (int level = 0; level < 3; ++level)
                inner = this->functor("s", {inner});
            const expr* outer = inner;
            for (int level = 0; level < 2; ++level)
                outer = this->functor("s", {outer});
            const expr* reach_nat = this->atom("z");
            for (int level = 0; level < 5; ++level)
                reach_nat = this->functor("s", {reach_nat});
            const expr* peano_two = this->functor("s", {this->functor("s", {this->atom("z")})});
            const expr* peano_one = this->functor("s", {this->atom("z")});
            modules->emplace_back(9301, std::vector<pud_module_id>{}, std::vector<rule>{
                rule(this->functor("nat", {this->atom("z")}), {}, 0),
                rule(this->functor("nat", {this->functor("s", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
                rule(this->functor("deep_nat", {deep}), {this->functor("nat", {deep})}, 0),
                rule(
                    this->functor("shallow", {this->var(0)}),
                    {this->functor("nat", {this->var(0)}), this->functor("add", {this->var(0), peano_two, outer})},
                    1),
                rule(this->functor("add", {this->atom("z"), this->var(0), this->var(0)}), {}, 1),
                rule(
                    this->functor("add", {this->functor("s", {this->var(0)}), this->var(1), this->functor("s", {this->var(2)})}),
                    {this->functor("add", {this->var(0), this->var(1), this->var(2)})},
                    3),
            });
            modules->emplace_back(9302, std::vector<pud_module_id>{9301}, std::vector<rule>{
                rule(
                    this->functor("reach", {this->var(0)}),
                    {this->functor("nat", {reach_nat}), this->functor("add", {this->var(0), this->functor("s", {this->atom("z")}), reach_nat})},
                    1),
            });
        }

    }
    TypeParam twin_runtime(this->exprs);
    std::optional<TypeParam> restarted_runtime;
    restarted_runtime.emplace(this->exprs);
    std::set<pud_global_node_id> twin_live;
    std::set<pud_global_node_id> restarted_live;
    for (pud_module& module : twin_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = twin_runtime.register_module(module);
        twin_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            twin_live.erase(refuted);
    }
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }

{    const size_t unfold_budget = rng() % 25;
    size_t taken = 0;
    while (taken < unfold_budget && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
}
{
    restarted_runtime.reset();
    restarted_runtime.emplace(this->exprs);
    restarted_live.clear();
    std::vector<pud_global_node_id> refuted_on_restart;
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        refuted_on_restart.insert(refuted_on_restart.end(), change.refuted.begin(), change.refuted.end());
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }
    EXPECT_EQ(restarted_live, twin_live);
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    EXPECT_THAT(refuted_on_restart, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
}

{    size_t taken = 0;
    while (taken < 20 && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
}
        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 9301;
            return;
        }
    }
}

TYPED_TEST(PudRuntimeRestartIntegrationTest, TripleAppendRestartsAndContinuesLikeItsTwin) {
    for (uint32_t seed = 0; seed < 5 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(200 + seed);
    std::deque<pud_module> twin_modules;
    std::deque<pud_module> restarted_modules;
    for (std::deque<pud_module>* modules : {&twin_modules, &restarted_modules}) {
        modules->emplace_back(9401, std::vector<pud_module_id>{}, std::vector<rule>{
            rule(this->functor("app", {this->atom("nil"), this->var(0), this->var(0)}), {}, 1),
            rule(
                this->functor("app", {this->functor("cons", {this->var(0), this->var(1)}), this->var(2), this->functor("cons", {this->var(0), this->var(3)})}),
                {this->functor("app", {this->var(1), this->var(2), this->var(3)})},
                4),
        });
        modules->emplace_back(9402, std::vector<pud_module_id>{9401}, std::vector<rule>{
            rule(
                this->atom("triple_split"),
                {
                    this->functor("app", {this->var(0), this->var(1), this->functor("cons", {this->atom("a"), this->functor("cons", {this->atom("b"), this->functor("cons", {this->atom("c"), this->atom("nil")})})})}),
                    this->functor("app", {this->var(1), this->var(2), this->functor("cons", {this->atom("d"), this->functor("cons", {this->atom("e"), this->atom("nil")})})}),
                    this->functor("app", {this->var(0), this->var(2), this->var(3)}),
                },
                4),
            rule(
                this->functor("nested_app", {this->var(0)}),
                {
                    this->functor("app", {this->var(0), this->var(1), this->functor("cons", {this->atom("p"), this->functor("cons", {this->atom("q"), this->atom("nil")})})}),
                    this->functor("app", {this->var(1), this->functor("cons", {this->atom("r"), this->atom("nil")}), this->functor("cons", {this->atom("p"), this->functor("cons", {this->atom("q"), this->functor("cons", {this->atom("r"), this->atom("nil")})})})}),
                },
                2),
        });

    }
    TypeParam twin_runtime(this->exprs);
    std::optional<TypeParam> restarted_runtime;
    restarted_runtime.emplace(this->exprs);
    std::set<pud_global_node_id> twin_live;
    std::set<pud_global_node_id> restarted_live;
    for (pud_module& module : twin_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = twin_runtime.register_module(module);
        twin_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            twin_live.erase(refuted);
    }
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }

{    const size_t unfold_budget = rng() % 25;
    size_t taken = 0;
    while (taken < unfold_budget && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
}
{
    restarted_runtime.reset();
    restarted_runtime.emplace(this->exprs);
    restarted_live.clear();
    std::vector<pud_global_node_id> refuted_on_restart;
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        refuted_on_restart.insert(refuted_on_restart.end(), change.refuted.begin(), change.refuted.end());
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }
    EXPECT_EQ(restarted_live, twin_live);
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    EXPECT_THAT(refuted_on_restart, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
}

{    size_t taken = 0;
    while (taken < 20 && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
}
        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 9401;
            return;
        }
    }
}

TYPED_TEST(PudRuntimeRestartIntegrationTest, SharedBinaryRestartsAndContinuesLikeItsTwin) {
    for (uint32_t seed = 0; seed < 5 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(200 + seed);
    std::deque<pud_module> twin_modules;
    std::deque<pud_module> restarted_modules;
    for (std::deque<pud_module>* modules : {&twin_modules, &restarted_modules}) {
        {
            const expr* shared = this->atom("v");
            for (int level = 0; level < 4; ++level)
                shared = this->functor("pair", {shared, shared});
            const expr* peano_two = this->functor("s", {this->functor("s", {this->atom("z")})});
            modules->emplace_back(9501, std::vector<pud_module_id>{}, std::vector<rule>{
                rule(this->functor("nat", {this->atom("z")}), {}, 0),
                rule(this->functor("nat", {this->functor("s", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
                rule(this->functor("unpack", {shared, this->var(0)}), {this->functor("same", {this->var(0), shared})}, 1),
                rule(this->functor("same", {this->var(0), this->var(0)}), {}, 1),
                rule(
                    this->functor("both", {this->var(0), this->var(1)}),
                    {this->functor("same", {this->var(0), this->var(1)}), this->functor("nat", {this->var(0)})},
                    2),
            });
            modules->emplace_back(9502, std::vector<pud_module_id>{9501}, std::vector<rule>{
                rule(this->functor("grounded", {this->var(0)}), {this->functor("unpack", {shared, this->var(0)})}, 1),
                rule(this->functor("nat_query", {this->var(0)}), {this->functor("both", {peano_two, this->var(0)})}, 1),
            });
        }

    }
    TypeParam twin_runtime(this->exprs);
    std::optional<TypeParam> restarted_runtime;
    restarted_runtime.emplace(this->exprs);
    std::set<pud_global_node_id> twin_live;
    std::set<pud_global_node_id> restarted_live;
    for (pud_module& module : twin_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = twin_runtime.register_module(module);
        twin_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            twin_live.erase(refuted);
    }
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }

{    const size_t unfold_budget = rng() % 25;
    size_t taken = 0;
    while (taken < unfold_budget && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
}
{
    restarted_runtime.reset();
    restarted_runtime.emplace(this->exprs);
    restarted_live.clear();
    std::vector<pud_global_node_id> refuted_on_restart;
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        refuted_on_restart.insert(refuted_on_restart.end(), change.refuted.begin(), change.refuted.end());
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }
    EXPECT_EQ(restarted_live, twin_live);
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    EXPECT_THAT(refuted_on_restart, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
}

{    size_t taken = 0;
    while (taken < 20 && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
}
        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 9501;
            return;
        }
    }
}

TYPED_TEST(PudRuntimeRestartIntegrationTest, PeanoMulUnderStressRestartsAndContinuesLikeItsTwin) {
    for (uint32_t seed = 0; seed < 3 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(500 + seed);
    std::deque<pud_module> twin_modules;
    std::deque<pud_module> restarted_modules;
    for (std::deque<pud_module>* modules : {&twin_modules, &restarted_modules}) {
        modules->emplace_back(9001, std::vector<pud_module_id>{}, std::vector<rule>{
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
        modules->emplace_back(9002, std::vector<pud_module_id>{9001}, std::vector<rule>{
            rule(
                this->functor("factor", {this->var(0), this->var(1)}),
                {this->functor("mul", {this->var(0), this->var(1), this->functor("s", {this->functor("s", {this->functor("s", {this->functor("s", {this->functor("s", {this->functor("s", {this->atom("z")})})})})})})})},
                2),
            rule(
                this->functor("triple", {this->var(0)}),
                {this->functor("mul", {this->functor("s", {this->functor("s", {this->functor("s", {this->atom("z")})})}), this->var(0), this->functor("s", {this->functor("s", {this->functor("s", {this->functor("s", {this->functor("s", {this->functor("s", {this->functor("s", {this->functor("s", {this->functor("s", {this->atom("z")})})})})})})})})})})},
                1),
            rule(
                this->functor("add_mul", {this->var(0), this->var(1), this->var(2)}),
                {this->functor("add", {this->var(0), this->var(1), this->var(2)}), this->functor("mul", {this->var(0), this->functor("s", {this->functor("s", {this->atom("z")})}), this->var(3)})},
                4),
        });

    }
    TypeParam twin_runtime(this->exprs);
    std::optional<TypeParam> restarted_runtime;
    restarted_runtime.emplace(this->exprs);
    std::set<pud_global_node_id> twin_live;
    std::set<pud_global_node_id> restarted_live;
    for (pud_module& module : twin_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = twin_runtime.register_module(module);
        twin_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            twin_live.erase(refuted);
    }
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }

{    const size_t unfold_budget = 25 + rng() % 25;
    size_t taken = 0;
    while (taken < unfold_budget && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
}
{
    restarted_runtime.reset();
    restarted_runtime.emplace(this->exprs);
    restarted_live.clear();
    std::vector<pud_global_node_id> refuted_on_restart;
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        refuted_on_restart.insert(refuted_on_restart.end(), change.refuted.begin(), change.refuted.end());
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }
    EXPECT_EQ(restarted_live, twin_live);
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    EXPECT_THAT(refuted_on_restart, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
}

{    size_t taken = 0;
    while (taken < 30 && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
}
        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 9001;
            return;
        }
    }
}

TYPED_TEST(PudRuntimeRestartIntegrationTest, ListMembersUnderStressRestartsAndContinuesLikeItsTwin) {
    for (uint32_t seed = 0; seed < 3 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(500 + seed);
    std::deque<pud_module> twin_modules;
    std::deque<pud_module> restarted_modules;
    for (std::deque<pud_module>* modules : {&twin_modules, &restarted_modules}) {
        modules->emplace_back(9101, std::vector<pud_module_id>{}, std::vector<rule>{
            rule(this->functor("mem", {this->var(0), this->functor("cons", {this->var(0), this->var(1)})}), {}, 2),
            rule(
                this->functor("mem", {this->var(0), this->functor("cons", {this->var(1), this->var(2)})}),
                {this->functor("mem", {this->var(0), this->var(2)})},
                3),
        });
        modules->emplace_back(9102, std::vector<pud_module_id>{9101}, std::vector<rule>{
            rule(
                this->functor("every", {this->var(0)}),
                {this->functor("mem", {this->var(0), this->functor("cons", {this->atom("x"), this->functor("cons", {this->atom("y"), this->functor("cons", {this->atom("z"), this->atom("nil")})})})})},
                1),
            rule(
                this->functor("pair_mem", {this->var(0), this->var(1)}),
                {
                    this->functor("mem", {this->var(0), this->functor("cons", {this->atom("a"), this->functor("cons", {this->atom("b"), this->functor("cons", {this->atom("c"), this->atom("nil")})})})}),
                    this->functor("mem", {this->var(1), this->functor("cons", {this->atom("b"), this->functor("cons", {this->atom("c"), this->functor("cons", {this->atom("d"), this->atom("nil")})})})}),
                },
                2),
        });

    }
    TypeParam twin_runtime(this->exprs);
    std::optional<TypeParam> restarted_runtime;
    restarted_runtime.emplace(this->exprs);
    std::set<pud_global_node_id> twin_live;
    std::set<pud_global_node_id> restarted_live;
    for (pud_module& module : twin_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = twin_runtime.register_module(module);
        twin_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            twin_live.erase(refuted);
    }
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }

{    const size_t unfold_budget = 25 + rng() % 25;
    size_t taken = 0;
    while (taken < unfold_budget && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
}
{
    restarted_runtime.reset();
    restarted_runtime.emplace(this->exprs);
    restarted_live.clear();
    std::vector<pud_global_node_id> refuted_on_restart;
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        refuted_on_restart.insert(refuted_on_restart.end(), change.refuted.begin(), change.refuted.end());
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }
    EXPECT_EQ(restarted_live, twin_live);
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    EXPECT_THAT(refuted_on_restart, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
}

{    size_t taken = 0;
    while (taken < 30 && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
}
        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 9101;
            return;
        }
    }
}

TYPED_TEST(PudRuntimeRestartIntegrationTest, MirrorTreesUnderStressRestartsAndContinuesLikeItsTwin) {
    for (uint32_t seed = 0; seed < 3 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(500 + seed);
    std::deque<pud_module> twin_modules;
    std::deque<pud_module> restarted_modules;
    for (std::deque<pud_module>* modules : {&twin_modules, &restarted_modules}) {
        {
            const expr* sample = this->functor(
                "branch",
                {this->functor("leaf", {this->atom("l")}),
                 this->functor("branch", {this->functor("leaf", {this->atom("m")}), this->functor("leaf", {this->atom("r")})})});
            modules->emplace_back(9201, std::vector<pud_module_id>{}, std::vector<rule>{
                rule(this->functor("mirror", {this->functor("leaf", {this->var(0)}), this->functor("leaf", {this->var(0)})}), {}, 1),
                rule(
                    this->functor("mirror", {this->functor("branch", {this->var(0), this->var(1)}), this->functor("branch", {this->var(2), this->var(3)})}),
                    {this->functor("mirror", {this->var(0), this->var(3)}), this->functor("mirror", {this->var(1), this->var(2)})},
                    4),
            });
            modules->emplace_back(9202, std::vector<pud_module_id>{9201}, std::vector<rule>{
                rule(this->functor("flip", {this->var(0)}), {this->functor("mirror", {sample, this->var(0)})}, 1),
            });
        }

    }
    TypeParam twin_runtime(this->exprs);
    std::optional<TypeParam> restarted_runtime;
    restarted_runtime.emplace(this->exprs);
    std::set<pud_global_node_id> twin_live;
    std::set<pud_global_node_id> restarted_live;
    for (pud_module& module : twin_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = twin_runtime.register_module(module);
        twin_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            twin_live.erase(refuted);
    }
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }

{    const size_t unfold_budget = 25 + rng() % 25;
    size_t taken = 0;
    while (taken < unfold_budget && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
}
{
    restarted_runtime.reset();
    restarted_runtime.emplace(this->exprs);
    restarted_live.clear();
    std::vector<pud_global_node_id> refuted_on_restart;
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        refuted_on_restart.insert(refuted_on_restart.end(), change.refuted.begin(), change.refuted.end());
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }
    EXPECT_EQ(restarted_live, twin_live);
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    EXPECT_THAT(refuted_on_restart, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
}

{    size_t taken = 0;
    while (taken < 30 && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
}
        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 9201;
            return;
        }
    }
}

TYPED_TEST(PudRuntimeRestartIntegrationTest, NestedUnaryChainUnderStressRestartsAndContinuesLikeItsTwin) {
    for (uint32_t seed = 0; seed < 3 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(500 + seed);
    std::deque<pud_module> twin_modules;
    std::deque<pud_module> restarted_modules;
    for (std::deque<pud_module>* modules : {&twin_modules, &restarted_modules}) {
        {
            const expr* deep = this->atom("z");
            for (int level = 0; level < 8; ++level)
                deep = this->functor("s", {deep});
            const expr* inner = this->atom("z");
            for (int level = 0; level < 3; ++level)
                inner = this->functor("s", {inner});
            const expr* outer = inner;
            for (int level = 0; level < 2; ++level)
                outer = this->functor("s", {outer});
            const expr* reach_nat = this->atom("z");
            for (int level = 0; level < 5; ++level)
                reach_nat = this->functor("s", {reach_nat});
            const expr* peano_two = this->functor("s", {this->functor("s", {this->atom("z")})});
            const expr* peano_one = this->functor("s", {this->atom("z")});
            modules->emplace_back(9301, std::vector<pud_module_id>{}, std::vector<rule>{
                rule(this->functor("nat", {this->atom("z")}), {}, 0),
                rule(this->functor("nat", {this->functor("s", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
                rule(this->functor("deep_nat", {deep}), {this->functor("nat", {deep})}, 0),
                rule(
                    this->functor("shallow", {this->var(0)}),
                    {this->functor("nat", {this->var(0)}), this->functor("add", {this->var(0), peano_two, outer})},
                    1),
                rule(this->functor("add", {this->atom("z"), this->var(0), this->var(0)}), {}, 1),
                rule(
                    this->functor("add", {this->functor("s", {this->var(0)}), this->var(1), this->functor("s", {this->var(2)})}),
                    {this->functor("add", {this->var(0), this->var(1), this->var(2)})},
                    3),
            });
            modules->emplace_back(9302, std::vector<pud_module_id>{9301}, std::vector<rule>{
                rule(
                    this->functor("reach", {this->var(0)}),
                    {this->functor("nat", {reach_nat}), this->functor("add", {this->var(0), this->functor("s", {this->atom("z")}), reach_nat})},
                    1),
            });
        }

    }
    TypeParam twin_runtime(this->exprs);
    std::optional<TypeParam> restarted_runtime;
    restarted_runtime.emplace(this->exprs);
    std::set<pud_global_node_id> twin_live;
    std::set<pud_global_node_id> restarted_live;
    for (pud_module& module : twin_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = twin_runtime.register_module(module);
        twin_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            twin_live.erase(refuted);
    }
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }

{    const size_t unfold_budget = 25 + rng() % 25;
    size_t taken = 0;
    while (taken < unfold_budget && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
}
{
    restarted_runtime.reset();
    restarted_runtime.emplace(this->exprs);
    restarted_live.clear();
    std::vector<pud_global_node_id> refuted_on_restart;
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        refuted_on_restart.insert(refuted_on_restart.end(), change.refuted.begin(), change.refuted.end());
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }
    EXPECT_EQ(restarted_live, twin_live);
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    EXPECT_THAT(refuted_on_restart, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
}

{    size_t taken = 0;
    while (taken < 30 && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
}
        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 9301;
            return;
        }
    }
}

TYPED_TEST(PudRuntimeRestartIntegrationTest, TripleAppendUnderStressRestartsAndContinuesLikeItsTwin) {
    for (uint32_t seed = 0; seed < 3 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(500 + seed);
    std::deque<pud_module> twin_modules;
    std::deque<pud_module> restarted_modules;
    for (std::deque<pud_module>* modules : {&twin_modules, &restarted_modules}) {
        modules->emplace_back(9401, std::vector<pud_module_id>{}, std::vector<rule>{
            rule(this->functor("app", {this->atom("nil"), this->var(0), this->var(0)}), {}, 1),
            rule(
                this->functor("app", {this->functor("cons", {this->var(0), this->var(1)}), this->var(2), this->functor("cons", {this->var(0), this->var(3)})}),
                {this->functor("app", {this->var(1), this->var(2), this->var(3)})},
                4),
        });
        modules->emplace_back(9402, std::vector<pud_module_id>{9401}, std::vector<rule>{
            rule(
                this->atom("triple_split"),
                {
                    this->functor("app", {this->var(0), this->var(1), this->functor("cons", {this->atom("a"), this->functor("cons", {this->atom("b"), this->functor("cons", {this->atom("c"), this->atom("nil")})})})}),
                    this->functor("app", {this->var(1), this->var(2), this->functor("cons", {this->atom("d"), this->functor("cons", {this->atom("e"), this->atom("nil")})})}),
                    this->functor("app", {this->var(0), this->var(2), this->var(3)}),
                },
                4),
            rule(
                this->functor("nested_app", {this->var(0)}),
                {
                    this->functor("app", {this->var(0), this->var(1), this->functor("cons", {this->atom("p"), this->functor("cons", {this->atom("q"), this->atom("nil")})})}),
                    this->functor("app", {this->var(1), this->functor("cons", {this->atom("r"), this->atom("nil")}), this->functor("cons", {this->atom("p"), this->functor("cons", {this->atom("q"), this->functor("cons", {this->atom("r"), this->atom("nil")})})})}),
                },
                2),
        });

    }
    TypeParam twin_runtime(this->exprs);
    std::optional<TypeParam> restarted_runtime;
    restarted_runtime.emplace(this->exprs);
    std::set<pud_global_node_id> twin_live;
    std::set<pud_global_node_id> restarted_live;
    for (pud_module& module : twin_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = twin_runtime.register_module(module);
        twin_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            twin_live.erase(refuted);
    }
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }

{    const size_t unfold_budget = 25 + rng() % 25;
    size_t taken = 0;
    while (taken < unfold_budget && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
}
{
    restarted_runtime.reset();
    restarted_runtime.emplace(this->exprs);
    restarted_live.clear();
    std::vector<pud_global_node_id> refuted_on_restart;
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        refuted_on_restart.insert(refuted_on_restart.end(), change.refuted.begin(), change.refuted.end());
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }
    EXPECT_EQ(restarted_live, twin_live);
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    EXPECT_THAT(refuted_on_restart, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
}

{    size_t taken = 0;
    while (taken < 30 && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
}
        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 9401;
            return;
        }
    }
}

TYPED_TEST(PudRuntimeRestartIntegrationTest, SharedBinaryUnderStressRestartsAndContinuesLikeItsTwin) {
    for (uint32_t seed = 0; seed < 3 && !this->HasFailure(); ++seed) {
        std::mt19937 rng(500 + seed);
    std::deque<pud_module> twin_modules;
    std::deque<pud_module> restarted_modules;
    for (std::deque<pud_module>* modules : {&twin_modules, &restarted_modules}) {
        {
            const expr* shared = this->atom("v");
            for (int level = 0; level < 4; ++level)
                shared = this->functor("pair", {shared, shared});
            const expr* peano_two = this->functor("s", {this->functor("s", {this->atom("z")})});
            modules->emplace_back(9501, std::vector<pud_module_id>{}, std::vector<rule>{
                rule(this->functor("nat", {this->atom("z")}), {}, 0),
                rule(this->functor("nat", {this->functor("s", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
                rule(this->functor("unpack", {shared, this->var(0)}), {this->functor("same", {this->var(0), shared})}, 1),
                rule(this->functor("same", {this->var(0), this->var(0)}), {}, 1),
                rule(
                    this->functor("both", {this->var(0), this->var(1)}),
                    {this->functor("same", {this->var(0), this->var(1)}), this->functor("nat", {this->var(0)})},
                    2),
            });
            modules->emplace_back(9502, std::vector<pud_module_id>{9501}, std::vector<rule>{
                rule(this->functor("grounded", {this->var(0)}), {this->functor("unpack", {shared, this->var(0)})}, 1),
                rule(this->functor("nat_query", {this->var(0)}), {this->functor("both", {peano_two, this->var(0)})}, 1),
            });
        }

    }
    TypeParam twin_runtime(this->exprs);
    std::optional<TypeParam> restarted_runtime;
    restarted_runtime.emplace(this->exprs);
    std::set<pud_global_node_id> twin_live;
    std::set<pud_global_node_id> restarted_live;
    for (pud_module& module : twin_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = twin_runtime.register_module(module);
        twin_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            twin_live.erase(refuted);
    }
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }

{    const size_t unfold_budget = 25 + rng() % 25;
    size_t taken = 0;
    while (taken < unfold_budget && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
}
{
    restarted_runtime.reset();
    restarted_runtime.emplace(this->exprs);
    restarted_live.clear();
    std::vector<pud_global_node_id> refuted_on_restart;
    for (pud_module& module : restarted_modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = restarted_runtime->register_module(module);
        refuted_on_restart.insert(refuted_on_restart.end(), change.refuted.begin(), change.refuted.end());
        restarted_live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            restarted_live.erase(refuted);
    }
    EXPECT_EQ(restarted_live, twin_live);
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
    EXPECT_THAT(refuted_on_restart, IsEmpty());
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
}

{    size_t taken = 0;
    while (taken < 30 && !this->HasFailure()) {
        EXPECT_EQ(restarted_live, twin_live);
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : twin_live) {
            if (twin_runtime.get_rule_information(leaf).body.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        std::vector<pud_body_goal_id> goal_ids;
        for (const auto& [goal_id, goal] : twin_runtime.get_rule_information(leaf).body)
            goal_ids.push_back(goal_id);
        std::sort(goal_ids.begin(), goal_ids.end());
        const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
        const pud_frontier_change expected = twin_runtime.unfold(leaf, goal);
        twin_live.erase(leaf);
        twin_live.insert(expected.offspring.begin(), expected.offspring.end());
        for (pud_global_node_id refuted : expected.refuted)
            twin_live.erase(refuted);
        const pud_frontier_change actual = restarted_runtime->unfold(leaf, goal);
        restarted_live.erase(leaf);
        restarted_live.insert(actual.offspring.begin(), actual.offspring.end());
        for (pud_global_node_id refuted : actual.refuted)
            restarted_live.erase(refuted);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << leaf.module << ", " << leaf.node << ") on " << goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        ++taken;
    }
    EXPECT_THAT(reference_pud_module_differences(twin_modules, restarted_modules), IsEmpty());
    EXPECT_EQ(twin_live, reference_pud_frontier(twin_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(twin_modules), IsEmpty());
    EXPECT_EQ(restarted_live, reference_pud_frontier(restarted_modules));
    EXPECT_THAT(reference_pud_stranded_nodes(restarted_modules), IsEmpty());
}
        if (this->HasFailure()) {
            ADD_FAILURE() << "seed " << seed << ", program of module " << 9501;
            return;
        }
    }
}

