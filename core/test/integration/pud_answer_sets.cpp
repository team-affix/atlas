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
#include "reference_pud_normalize_with_bindings.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/pud_module.hpp"
#include "infrastructure/pud_runtime.hpp"

using ::testing::IsEmpty;

template<typename IRuntime>
struct PudAnswerSetsIntegrationTest : public ::testing::Test {
    test_functors functors;
    expr_pool exprs;
    const expr* var(uint32_t index) { return exprs.make_var(index); }
    const expr* atom(const char* name) { return exprs.make_functor(functors.id(name), {}); }
    const expr* functor(const char* name, std::vector<const expr*> args) {
        return exprs.make_functor(functors.id(name), std::move(args));
    }
};

using answer_set_runtimes_t = ::testing::Types<pud_runtime<pud_module, expr_pool>>;
TYPED_TEST_SUITE(PudAnswerSetsIntegrationTest, answer_set_runtimes_t);

TYPED_TEST(PudAnswerSetsIntegrationTest, ColorsQueryHasExactlyTheWarmColors) {
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("color", {this->atom("red")}), {}, 0),
        rule(this->functor("color", {this->atom("blue")}), {}, 0),
        rule(this->functor("color", {this->atom("green")}), {}, 0),
        rule(this->functor("warm", {this->atom("red")}), {}, 0),
        rule(this->functor("warm", {this->atom("orange")}), {}, 0),
    });
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(
            this->functor("q", {this->var(0)}),
            {this->functor("color", {this->var(0)}), this->functor("warm", {this->var(0)})},
            1),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id leaf : change.offspring) {
            live.insert(leaf);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
        }
    }
    size_t steps = 0;
    for (; steps < 2000; ++steps) {
        std::optional<pud_global_node_id> chosen_leaf;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            if (!chosen_leaf.has_value() || leaf < chosen_leaf.value())
                chosen_leaf = leaf;
        }
        if (!chosen_leaf.has_value())
            break;
        std::optional<pud_body_goal_id> chosen_goal;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(*chosen_leaf).body) {
            if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                chosen_goal = goal_id;
        }
        const pud_frontier_change change = runtime.unfold(*chosen_leaf, *chosen_goal);
        live.erase(*chosen_leaf);
        for (pud_global_node_id child : change.offspring) {
            live.insert(child);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
        }
    }
    EXPECT_LT(steps, 2000u);
    std::set<const expr*> answers;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        const pud_unnormalized_rule_information leaf_info = runtime.get_rule_information(leaf);
        if (!leaf_info.body.empty())
            continue;
        answers.insert(reference_pud_normalize_with_bindings(leaf_info.bindings, this->exprs, leaf_info.head));
    }
    EXPECT_EQ(answers, (std::set<const expr*>{this->functor("q", {this->atom("red")})}));
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsIntegrationTest, ColorsQueryHasTheSameAnswersInEveryUnfoldOrder) {
    const std::set<const expr*> expected{this->functor("q", {this->atom("red")})};
    for (uint32_t seed = 0; seed < 50; ++seed) {
        std::mt19937 rng(seed);
        std::deque<pud_module> modules;
        modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
            rule(this->functor("color", {this->atom("red")}), {}, 0),
            rule(this->functor("color", {this->atom("blue")}), {}, 0),
            rule(this->functor("color", {this->atom("green")}), {}, 0),
            rule(this->functor("warm", {this->atom("red")}), {}, 0),
            rule(this->functor("warm", {this->atom("orange")}), {}, 0),
        });
        modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
            rule(
                this->functor("q", {this->var(0)}),
                {this->functor("color", {this->var(0)}), this->functor("warm", {this->var(0)})},
                1),
        });
        TypeParam runtime(this->exprs);
        std::set<pud_global_node_id> live;
            for (pud_module& module : modules) {
            install_shared_functor_names(module, this->functors);
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id leaf : change.offspring) {
                live.insert(leaf);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
            }
        }
        size_t steps = 0;
        for (; steps < 2000; ++steps) {
            std::vector<pud_global_node_id> unfoldable;
            for (pud_global_node_id leaf : live) {
                if (leaf.module != 2)
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
            const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
            const pud_frontier_change change = runtime.unfold(leaf, goal);
            live.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
                live.insert(child);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
            }
        }
        EXPECT_LT(steps, 2000u);
        std::set<const expr*> answers;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            const pud_unnormalized_rule_information leaf_info = runtime.get_rule_information(leaf);
            if (!leaf_info.body.empty())
                continue;
            answers.insert(reference_pud_normalize_with_bindings(leaf_info.bindings, this->exprs, leaf_info.head));
        }
        EXPECT_EQ(answers, expected);
    }
}

TYPED_TEST(PudAnswerSetsIntegrationTest, PeanoAdditionIsInvertedExactly) {
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("add", {this->atom("z"), this->var(0), this->var(0)}), {}, 1),
        rule(
            this->functor("add", {this->functor("s", {this->var(0)}), this->var(1), this->functor("s", {this->var(2)})}),
            {this->functor("add", {this->var(0), this->var(1), this->var(2)})},
            3),
    });
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(
            this->functor("q", {this->var(0), this->var(1)}),
            {this->functor("add", {this->var(0), this->var(1),
                this->functor("s", {this->functor("s", {this->atom("z")})})})},
            2),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id leaf : change.offspring) {
            live.insert(leaf);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
        }
    }
    size_t steps = 0;
    for (; steps < 2000; ++steps) {
        std::optional<pud_global_node_id> chosen_leaf;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            if (!chosen_leaf.has_value() || leaf < chosen_leaf.value())
                chosen_leaf = leaf;
        }
        if (!chosen_leaf.has_value())
            break;
        std::optional<pud_body_goal_id> chosen_goal;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(*chosen_leaf).body) {
            if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                chosen_goal = goal_id;
        }
        const pud_frontier_change change = runtime.unfold(*chosen_leaf, *chosen_goal);
        live.erase(*chosen_leaf);
        for (pud_global_node_id child : change.offspring) {
            live.insert(child);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
        }
    }
    EXPECT_LT(steps, 2000u);
    std::set<const expr*> answers;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        const pud_unnormalized_rule_information leaf_info = runtime.get_rule_information(leaf);
        if (!leaf_info.body.empty())
            continue;
        answers.insert(reference_pud_normalize_with_bindings(leaf_info.bindings, this->exprs, leaf_info.head));
    }
    EXPECT_EQ(
        answers,
        (std::set<const expr*>{
            this->functor("q", {this->atom("z"), this->functor("s", {this->functor("s", {this->atom("z")})})}),
            this->functor("q", {this->functor("s", {this->atom("z")}), this->functor("s", {this->atom("z")})}),
            this->functor("q", {this->functor("s", {this->functor("s", {this->atom("z")})}), this->atom("z")})}));
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsIntegrationTest, PeanoAdditionHasTheSameAnswersInEveryUnfoldOrder) {
    const std::set<const expr*> expected{
        this->functor("q", {this->atom("z"), this->functor("s", {this->functor("s", {this->atom("z")})})}),
        this->functor("q", {this->functor("s", {this->atom("z")}), this->functor("s", {this->atom("z")})}),
        this->functor("q", {this->functor("s", {this->functor("s", {this->atom("z")})}), this->atom("z")})};
    for (uint32_t seed = 0; seed < 50; ++seed) {
        std::mt19937 rng(seed);
        std::deque<pud_module> modules;
        modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
            rule(this->functor("add", {this->atom("z"), this->var(0), this->var(0)}), {}, 1),
            rule(
                this->functor("add", {this->functor("s", {this->var(0)}), this->var(1), this->functor("s", {this->var(2)})}),
                {this->functor("add", {this->var(0), this->var(1), this->var(2)})},
                3),
        });
        modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
            rule(
                this->functor("q", {this->var(0), this->var(1)}),
                {this->functor("add", {this->var(0), this->var(1),
                    this->functor("s", {this->functor("s", {this->atom("z")})})})},
                2),
        });
        TypeParam runtime(this->exprs);
        std::set<pud_global_node_id> live;
            for (pud_module& module : modules) {
            install_shared_functor_names(module, this->functors);
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id leaf : change.offspring) {
                live.insert(leaf);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
            }
        }
        size_t steps = 0;
        for (; steps < 2000; ++steps) {
            std::vector<pud_global_node_id> unfoldable;
            for (pud_global_node_id leaf : live) {
                if (leaf.module != 2)
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
            const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
            const pud_frontier_change change = runtime.unfold(leaf, goal);
            live.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
                live.insert(child);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
            }
        }
        EXPECT_LT(steps, 2000u);
        std::set<const expr*> answers;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            const pud_unnormalized_rule_information leaf_info = runtime.get_rule_information(leaf);
            if (!leaf_info.body.empty())
                continue;
            answers.insert(reference_pud_normalize_with_bindings(leaf_info.bindings, this->exprs, leaf_info.head));
        }
        EXPECT_EQ(answers, expected);
    }
}

TYPED_TEST(PudAnswerSetsIntegrationTest, AppendSplitsAListEveryWay) {
    const expr* target = this->functor(
        "cons",
        {this->atom("a"), this->functor("cons", {this->atom("b"), this->atom("nil")})});
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("app", {this->atom("nil"), this->var(0), this->var(0)}), {}, 1),
        rule(
            this->functor("app", {this->functor("cons", {this->var(0), this->var(1)}), this->var(2), this->functor("cons", {this->var(0), this->var(3)})}),
            {this->functor("app", {this->var(1), this->var(2), this->var(3)})},
            4),
    });
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {this->var(0), this->var(1)}), {this->functor("app", {this->var(0), this->var(1), target})}, 2),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id leaf : change.offspring) {
            live.insert(leaf);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
        }
    }
    size_t steps = 0;
    for (; steps < 2000; ++steps) {
        std::optional<pud_global_node_id> chosen_leaf;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            if (!chosen_leaf.has_value() || leaf < chosen_leaf.value())
                chosen_leaf = leaf;
        }
        if (!chosen_leaf.has_value())
            break;
        std::optional<pud_body_goal_id> chosen_goal;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(*chosen_leaf).body) {
            if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                chosen_goal = goal_id;
        }
        const pud_frontier_change change = runtime.unfold(*chosen_leaf, *chosen_goal);
        live.erase(*chosen_leaf);
        for (pud_global_node_id child : change.offspring) {
            live.insert(child);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
        }
    }
    EXPECT_LT(steps, 2000u);
    std::set<const expr*> answers;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        const pud_unnormalized_rule_information leaf_info = runtime.get_rule_information(leaf);
        if (!leaf_info.body.empty())
            continue;
        answers.insert(reference_pud_normalize_with_bindings(leaf_info.bindings, this->exprs, leaf_info.head));
    }
    EXPECT_EQ(
        answers,
        (std::set<const expr*>{
            this->functor("q", {this->atom("nil"), target}),
            this->functor("q", {this->functor("cons", {this->atom("a"), this->atom("nil")}), this->functor("cons", {this->atom("b"), this->atom("nil")})}),
            this->functor("q", {target, this->atom("nil")})}));
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsIntegrationTest, AppendSplitsHaveTheSameAnswersInEveryUnfoldOrder) {
    const expr* target = this->functor(
        "cons",
        {this->atom("a"), this->functor("cons", {this->atom("b"), this->atom("nil")})});
    const std::set<const expr*> expected{
        this->functor("q", {this->atom("nil"), target}),
        this->functor("q", {this->functor("cons", {this->atom("a"), this->atom("nil")}), this->functor("cons", {this->atom("b"), this->atom("nil")})}),
        this->functor("q", {target, this->atom("nil")})};
    for (uint32_t seed = 0; seed < 50; ++seed) {
        std::mt19937 rng(seed);
        std::deque<pud_module> modules;
        modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
            rule(this->functor("app", {this->atom("nil"), this->var(0), this->var(0)}), {}, 1),
            rule(
                this->functor("app", {this->functor("cons", {this->var(0), this->var(1)}), this->var(2), this->functor("cons", {this->var(0), this->var(3)})}),
                {this->functor("app", {this->var(1), this->var(2), this->var(3)})},
                4),
        });
        modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
            rule(this->functor("q", {this->var(0), this->var(1)}), {this->functor("app", {this->var(0), this->var(1), target})}, 2),
        });
        TypeParam runtime(this->exprs);
        std::set<pud_global_node_id> live;
            for (pud_module& module : modules) {
            install_shared_functor_names(module, this->functors);
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id leaf : change.offspring) {
                live.insert(leaf);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
            }
        }
        size_t steps = 0;
        for (; steps < 2000; ++steps) {
            std::vector<pud_global_node_id> unfoldable;
            for (pud_global_node_id leaf : live) {
                if (leaf.module != 2)
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
            const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
            const pud_frontier_change change = runtime.unfold(leaf, goal);
            live.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
                live.insert(child);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
            }
        }
        EXPECT_LT(steps, 2000u);
        std::set<const expr*> answers;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            const pud_unnormalized_rule_information leaf_info = runtime.get_rule_information(leaf);
            if (!leaf_info.body.empty())
                continue;
            answers.insert(reference_pud_normalize_with_bindings(leaf_info.bindings, this->exprs, leaf_info.head));
        }
        EXPECT_EQ(answers, expected);
    }
}

TYPED_TEST(PudAnswerSetsIntegrationTest, ARefutedNodeNeverHeldAnAnswer) {
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("add", {this->atom("z"), this->var(0), this->var(0)}), {}, 1),
        rule(
            this->functor("add", {this->functor("s", {this->var(0)}), this->var(1), this->functor("s", {this->var(2)})}),
            {this->functor("add", {this->var(0), this->var(1), this->var(2)})},
            3),
    });
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(
            this->functor("q", {this->var(0), this->var(1)}),
            {this->functor("add", {this->var(0), this->var(1),
                this->functor("s", {this->functor("s", {this->atom("z")})})})},
            2),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id leaf : change.offspring) {
            live.insert(leaf);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
        }
    }
    size_t steps = 0;
    for (; steps < 2000; ++steps) {
        std::optional<pud_global_node_id> chosen_leaf;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            if (runtime.get_rule_information(leaf).body.empty())
                continue;
            if (!chosen_leaf.has_value() || leaf < chosen_leaf.value())
                chosen_leaf = leaf;
        }
        if (!chosen_leaf.has_value())
            break;
        std::optional<pud_body_goal_id> chosen_goal;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(*chosen_leaf).body) {
            if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                chosen_goal = goal_id;
        }
        const pud_frontier_change change = runtime.unfold(*chosen_leaf, *chosen_goal);
        live.erase(*chosen_leaf);
        for (pud_global_node_id child : change.offspring) {
            live.insert(child);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
        }
    }
    EXPECT_LT(steps, 2000u);
    const pud_module& query_module = modules.back();
    std::set<const expr*> answers;
    size_t solved_live_leaves = 0;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        const pud_unnormalized_rule_information leaf_info = runtime.get_rule_information(leaf);
        if (!leaf_info.body.empty())
            continue;
        ++solved_live_leaves;
        answers.insert(reference_pud_normalize_with_bindings(leaf_info.bindings, this->exprs, leaf_info.head));
    }
    EXPECT_EQ(solved_live_leaves, answers.size());
    std::vector<pud_node_id> pending;
    for (pud_node_id axiom = 0; axiom < query_module.count_axioms(); ++axiom)
        pending.push_back(axiom);
    while (!pending.empty()) {
        const pud_node_id node = pending.back();
        pending.pop_back();
        if (query_module.check_refuted(node)) {
            std::vector<pud_node_id> descent;
            descent.push_back(node);
            while (!descent.empty()) {
                const pud_node_id current = descent.back();
                descent.pop_back();
                if (query_module.get_children(current).empty()) {
                    const pud_global_node_id global{.module = 2, .node = current};
                    const bool solved_leaf =
                        live.count(global) && runtime.get_rule_information(global).body.empty();
                    EXPECT_FALSE(solved_leaf);
                }
                for (pud_node_id child : query_module.get_children(current))
                    descent.push_back(child);
            }
        }
        for (pud_node_id child : query_module.get_children(node))
            pending.push_back(child);
    }
}

TYPED_TEST(PudAnswerSetsIntegrationTest, PeanoAdditionAnswersSurviveUnfoldingTheDependencyFirst) {
    const std::set<const expr*> expected{
        this->functor("q", {this->atom("z"), this->functor("s", {this->functor("s", {this->atom("z")})})}),
        this->functor("q", {this->functor("s", {this->atom("z")}), this->functor("s", {this->atom("z")})}),
        this->functor("q", {this->functor("s", {this->functor("s", {this->atom("z")})}), this->atom("z")})};
    for (uint32_t seed = 0; seed < 20; ++seed) {
        std::mt19937 rng(seed);
        std::deque<pud_module> modules;
        modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
            rule(this->functor("add", {this->atom("z"), this->var(0), this->var(0)}), {}, 1),
            rule(
                this->functor("add", {this->functor("s", {this->var(0)}), this->var(1), this->functor("s", {this->var(2)})}),
                {this->functor("add", {this->var(0), this->var(1), this->var(2)})},
                3),
        });
        modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
            rule(
                this->functor("q", {this->var(0), this->var(1)}),
                {this->functor("add", {this->var(0), this->var(1),
                    this->functor("s", {this->functor("s", {this->atom("z")})})})},
                2),
        });
        TypeParam runtime(this->exprs);
        std::set<pud_global_node_id> live;
            for (pud_module& module : modules) {
            install_shared_functor_names(module, this->functors);
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id leaf : change.offspring) {
                live.insert(leaf);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
            }
        }
        for (size_t dependency_steps = 0; dependency_steps < 15; ++dependency_steps) {
            std::vector<pud_global_node_id> unfoldable;
            for (pud_global_node_id leaf : live) {
                if (leaf.module != 1)
                    continue;
                if (runtime.get_rule_information(leaf).body.empty())
                    continue;
                unfoldable.push_back(leaf);
            }
            ASSERT_FALSE(unfoldable.empty());
            const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
            std::vector<pud_body_goal_id> goal_ids;
            for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
                goal_ids.push_back(goal_id);
            const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
            const pud_frontier_change change = runtime.unfold(leaf, goal);
            live.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
                live.insert(child);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
            }
        }
        size_t query_steps = 0;
        for (; query_steps < 2000; ++query_steps) {
            std::vector<pud_global_node_id> unfoldable;
            for (pud_global_node_id leaf : live) {
                if (leaf.module != 2)
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
            const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
            const pud_frontier_change change = runtime.unfold(leaf, goal);
            live.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
                live.insert(child);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
            }
        }
        EXPECT_LT(query_steps, 2000u);
        std::set<const expr*> answers;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            const pud_unnormalized_rule_information leaf_info = runtime.get_rule_information(leaf);
            if (!leaf_info.body.empty())
                continue;
            answers.insert(reference_pud_normalize_with_bindings(leaf_info.bindings, this->exprs, leaf_info.head));
        }
        EXPECT_EQ(answers, expected);
    }
}

TYPED_TEST(PudAnswerSetsIntegrationTest, AppendAnswersSurviveUnfoldingTheDependencyFirst) {
    const expr* target = this->functor(
        "cons",
        {this->atom("a"), this->functor("cons", {this->atom("b"), this->atom("nil")})});
    const std::set<const expr*> expected{
        this->functor("q", {this->atom("nil"), target}),
        this->functor("q", {this->functor("cons", {this->atom("a"), this->atom("nil")}), this->functor("cons", {this->atom("b"), this->atom("nil")})}),
        this->functor("q", {target, this->atom("nil")})};
    for (uint32_t seed = 0; seed < 20; ++seed) {
        std::mt19937 rng(seed);
        std::deque<pud_module> modules;
        modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
            rule(this->functor("app", {this->atom("nil"), this->var(0), this->var(0)}), {}, 1),
            rule(
                this->functor("app", {this->functor("cons", {this->var(0), this->var(1)}), this->var(2), this->functor("cons", {this->var(0), this->var(3)})}),
                {this->functor("app", {this->var(1), this->var(2), this->var(3)})},
                4),
        });
        modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
            rule(this->functor("q", {this->var(0), this->var(1)}), {this->functor("app", {this->var(0), this->var(1), target})}, 2),
        });
        TypeParam runtime(this->exprs);
        std::set<pud_global_node_id> live;
            for (pud_module& module : modules) {
            install_shared_functor_names(module, this->functors);
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id leaf : change.offspring) {
                live.insert(leaf);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
            }
        }
        for (size_t dependency_steps = 0; dependency_steps < 15; ++dependency_steps) {
            std::vector<pud_global_node_id> unfoldable;
            for (pud_global_node_id leaf : live) {
                if (leaf.module != 1)
                    continue;
                if (runtime.get_rule_information(leaf).body.empty())
                    continue;
                unfoldable.push_back(leaf);
            }
            ASSERT_FALSE(unfoldable.empty());
            const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
            std::vector<pud_body_goal_id> goal_ids;
            for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
                goal_ids.push_back(goal_id);
            const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
            const pud_frontier_change change = runtime.unfold(leaf, goal);
            live.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
                live.insert(child);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
            }
        }
        size_t query_steps = 0;
        for (; query_steps < 2000; ++query_steps) {
            std::vector<pud_global_node_id> unfoldable;
            for (pud_global_node_id leaf : live) {
                if (leaf.module != 2)
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
            const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
            const pud_frontier_change change = runtime.unfold(leaf, goal);
            live.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
                live.insert(child);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
            }
        }
        EXPECT_LT(query_steps, 2000u);
        std::set<const expr*> answers;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            const pud_unnormalized_rule_information leaf_info = runtime.get_rule_information(leaf);
            if (!leaf_info.body.empty())
                continue;
            answers.insert(reference_pud_normalize_with_bindings(leaf_info.bindings, this->exprs, leaf_info.head));
        }
        EXPECT_EQ(answers, expected);
    }
}
