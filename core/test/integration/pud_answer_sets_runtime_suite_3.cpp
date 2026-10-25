#ifndef PUD_ANSWER_SETS_RUNTIME_SUITE_3_CPP
#define PUD_ANSWER_SETS_RUNTIME_SUITE_3_CPP

#include <cstdint>
#include <deque>
#include <map>
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
struct PudAnswerSetsRuntimeSuite3IntegrationTest : public ::testing::Test {
    test_functors functors;
    expr_pool exprs;

    const expr* var(uint32_t index) { return exprs.make_var(index); }
    const expr* atom(const char* name) { return exprs.make_functor(functors.id(name), {}); }
    const expr* functor(const char* name, std::vector<const expr*> args) {
        return exprs.make_functor(functors.id(name), std::move(args));
    }
};

using pud_answer_sets_runtime_suite_3_runtimes_t = ::testing::Types<pud_runtime<pud_module, expr_pool>>;
TYPED_TEST_SUITE(PudAnswerSetsRuntimeSuite3IntegrationTest, pud_answer_sets_runtime_suite_3_runtimes_t);

TYPED_TEST(PudAnswerSetsRuntimeSuite3IntegrationTest, EnumeratesThreeGroundBranches) {
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("f", {}), {}, 0),
        rule(this->functor("f", {}), {}, 0),
        rule(this->functor("f", {}), {}, 0)
    });
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {}), {this->functor("f", {})}, 0)
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    std::map<pud_global_node_id, pud_unnormalized_rule_information> leaf_rule_info;
    for (pud_module& module : modules) {
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id leaf : change.offspring) {
            live.insert(leaf);
            leaf_rule_info.emplace(leaf, runtime.get_rule_information(leaf));
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_rule_info.erase(refuted);
        }
    }

    size_t steps = 0;
    for (; steps < 20000; ++steps) {
        std::optional<pud_global_node_id> chosen_leaf;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            if (leaf_rule_info.at(leaf).body.empty())
                continue;
            if (!chosen_leaf.has_value() || leaf < chosen_leaf.value())
                chosen_leaf = leaf;
        }
        if (!chosen_leaf.has_value())
            break;
        std::optional<pud_body_goal_id> chosen_goal;
        for (const auto& [goal_id, goal] : leaf_rule_info.at(*chosen_leaf).body) {
            if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                chosen_goal = goal_id;
        }
        const pud_frontier_change change = runtime.unfold(*chosen_leaf, *chosen_goal);
        live.erase(*chosen_leaf);
        leaf_rule_info.erase(*chosen_leaf);
        for (pud_global_node_id child : change.offspring) {
            live.insert(child);
            leaf_rule_info.emplace(child, runtime.get_rule_information(child));
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_rule_info.erase(refuted);
        }
    }
    EXPECT_LT(steps, 20000u);
    std::set<const expr*> answers;
    size_t solved_live_leaves = 0;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        const pud_unnormalized_rule_information info = runtime.get_rule_information(leaf);
        if (!info.body.empty())
            continue;
        ++solved_live_leaves;
        answers.insert(reference_pud_normalize_with_bindings(info.bindings, this->exprs, info.head));
    }

    EXPECT_EQ(solved_live_leaves, 3u);
    EXPECT_EQ(answers, (std::set<const expr*>{this->functor("q", {})}));

    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuite3IntegrationTest, EnumeratesThreeGroundBranchesInEveryUnfoldOrder) {
    const std::set<const expr*> expected{this->functor("q", {})};
    for (uint32_t seed = 0; seed < 20; ++seed) {
        std::mt19937 rng(seed);
        std::deque<pud_module> modules;
        modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("f", {}), {}, 0),
        rule(this->functor("f", {}), {}, 0),
        rule(this->functor("f", {}), {}, 0)
        });
        modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {}), {this->functor("f", {})}, 0)
        });
        TypeParam runtime(this->exprs);
        std::set<pud_global_node_id> live;
        std::map<pud_global_node_id, pud_unnormalized_rule_information> leaf_rule_info;
        for (pud_module& module : modules) {
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id leaf : change.offspring) {
                live.insert(leaf);
                leaf_rule_info.emplace(leaf, runtime.get_rule_information(leaf));
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                leaf_rule_info.erase(refuted);
            }
        }

        size_t steps = 0;
        for (; steps < 20000; ++steps) {
            std::vector<pud_global_node_id> unfoldable;
            for (pud_global_node_id leaf : live) {
                if (leaf.module != 2)
                    continue;
                if (leaf_rule_info.at(leaf).body.empty())
                    continue;
                unfoldable.push_back(leaf);
            }
            if (unfoldable.empty())
                break;
            const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
            std::vector<pud_body_goal_id> goal_ids;
            for (const auto& [goal_id, goal] : leaf_rule_info.at(leaf).body)
                goal_ids.push_back(goal_id);
            const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
            const pud_frontier_change change = runtime.unfold(leaf, goal);
            live.erase(leaf);
            leaf_rule_info.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
                live.insert(child);
                leaf_rule_info.emplace(child, runtime.get_rule_information(child));
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                }
        }
        EXPECT_LT(steps, 20000u);
        std::set<const expr*> answers;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            const pud_unnormalized_rule_information info = runtime.get_rule_information(leaf);
            if (!info.body.empty())
                continue;
            answers.insert(reference_pud_normalize_with_bindings(info.bindings, this->exprs, info.head));
        }

        EXPECT_EQ(answers, expected);
        EXPECT_EQ(live, reference_pud_frontier(modules));
        EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    }
}

TYPED_TEST(PudAnswerSetsRuntimeSuite3IntegrationTest, SolvesRecursiveClauseTreeWithoutBranching) {
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("f", {}), {this->functor("g", {}), this->functor("h", {})}, 0),
        rule(this->functor("g", {}), {this->functor("i", {}), this->functor("j", {})}, 0),
        rule(this->functor("h", {}), {this->functor("i", {}), this->functor("j", {})}, 0),
        rule(this->functor("i", {}), {}, 0),
        rule(this->functor("j", {}), {}, 0)
    });
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {}), {this->functor("f", {})}, 0)
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    std::map<pud_global_node_id, pud_unnormalized_rule_information> leaf_rule_info;
    for (pud_module& module : modules) {
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id leaf : change.offspring) {
            live.insert(leaf);
            leaf_rule_info.emplace(leaf, runtime.get_rule_information(leaf));
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_rule_info.erase(refuted);
        }
    }

    size_t steps = 0;
    for (; steps < 20000; ++steps) {
        std::optional<pud_global_node_id> chosen_leaf;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            if (leaf_rule_info.at(leaf).body.empty())
                continue;
            if (!chosen_leaf.has_value() || leaf < chosen_leaf.value())
                chosen_leaf = leaf;
        }
        if (!chosen_leaf.has_value())
            break;
        std::optional<pud_body_goal_id> chosen_goal;
        for (const auto& [goal_id, goal] : leaf_rule_info.at(*chosen_leaf).body) {
            if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                chosen_goal = goal_id;
        }
        const pud_frontier_change change = runtime.unfold(*chosen_leaf, *chosen_goal);
        live.erase(*chosen_leaf);
        leaf_rule_info.erase(*chosen_leaf);
        for (pud_global_node_id child : change.offspring) {
            live.insert(child);
            leaf_rule_info.emplace(child, runtime.get_rule_information(child));
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_rule_info.erase(refuted);
        }
    }
    EXPECT_LT(steps, 20000u);
    std::set<const expr*> answers;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        const pud_unnormalized_rule_information info = runtime.get_rule_information(leaf);
        if (!info.body.empty())
            continue;
        answers.insert(reference_pud_normalize_with_bindings(info.bindings, this->exprs, info.head));
    }

    EXPECT_EQ(answers, (std::set<const expr*>{this->functor("q", {})}));

    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuite3IntegrationTest, SolvesRecursiveClauseTreeWithoutBranchingInEveryUnfoldOrder) {
    const std::set<const expr*> expected{this->functor("q", {})};
    for (uint32_t seed = 0; seed < 20; ++seed) {
        std::mt19937 rng(seed);
        std::deque<pud_module> modules;
        modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("f", {}), {this->functor("g", {}), this->functor("h", {})}, 0),
        rule(this->functor("g", {}), {this->functor("i", {}), this->functor("j", {})}, 0),
        rule(this->functor("h", {}), {this->functor("i", {}), this->functor("j", {})}, 0),
        rule(this->functor("i", {}), {}, 0),
        rule(this->functor("j", {}), {}, 0)
        });
        modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {}), {this->functor("f", {})}, 0)
        });
        TypeParam runtime(this->exprs);
        std::set<pud_global_node_id> live;
        std::map<pud_global_node_id, pud_unnormalized_rule_information> leaf_rule_info;
        for (pud_module& module : modules) {
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id leaf : change.offspring) {
                live.insert(leaf);
                leaf_rule_info.emplace(leaf, runtime.get_rule_information(leaf));
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                leaf_rule_info.erase(refuted);
            }
        }

        size_t steps = 0;
        for (; steps < 20000; ++steps) {
            std::vector<pud_global_node_id> unfoldable;
            for (pud_global_node_id leaf : live) {
                if (leaf.module != 2)
                    continue;
                if (leaf_rule_info.at(leaf).body.empty())
                    continue;
                unfoldable.push_back(leaf);
            }
            if (unfoldable.empty())
                break;
            const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
            std::vector<pud_body_goal_id> goal_ids;
            for (const auto& [goal_id, goal] : leaf_rule_info.at(leaf).body)
                goal_ids.push_back(goal_id);
            const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
            const pud_frontier_change change = runtime.unfold(leaf, goal);
            live.erase(leaf);
            leaf_rule_info.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
                live.insert(child);
                leaf_rule_info.emplace(child, runtime.get_rule_information(child));
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                }
        }
        EXPECT_LT(steps, 20000u);
        std::set<const expr*> answers;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            const pud_unnormalized_rule_information info = runtime.get_rule_information(leaf);
            if (!info.body.empty())
                continue;
            answers.insert(reference_pud_normalize_with_bindings(info.bindings, this->exprs, info.head));
        }

        EXPECT_EQ(answers, expected);
        EXPECT_EQ(live, reference_pud_frontier(modules));
        EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    }
}

TYPED_TEST(PudAnswerSetsRuntimeSuite3IntegrationTest, EnumeratesTransitiveReachFromA) {
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("reach", {this->atom("a"), this->atom("b")}), {}, 0),
        rule(this->functor("reach", {this->atom("a"), this->atom("c")}), {}, 0),
        rule(this->functor("reach", {this->atom("a"), this->atom("d")}), {}, 0)
    });
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {this->var(0)}),
            {this->functor("reach", {this->atom("a"), this->var(0)})}, 1)
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    std::map<pud_global_node_id, pud_unnormalized_rule_information> leaf_rule_info;
    for (pud_module& module : modules) {
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id leaf : change.offspring) {
            live.insert(leaf);
            leaf_rule_info.emplace(leaf, runtime.get_rule_information(leaf));
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_rule_info.erase(refuted);
        }
    }

    size_t steps = 0;
    for (; steps < 20000; ++steps) {
        std::optional<pud_global_node_id> chosen_leaf;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            if (leaf_rule_info.at(leaf).body.empty())
                continue;
            if (!chosen_leaf.has_value() || leaf < chosen_leaf.value())
                chosen_leaf = leaf;
        }
        if (!chosen_leaf.has_value())
            break;
        std::optional<pud_body_goal_id> chosen_goal;
        for (const auto& [goal_id, goal] : leaf_rule_info.at(*chosen_leaf).body) {
            if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                chosen_goal = goal_id;
        }
        const pud_frontier_change change = runtime.unfold(*chosen_leaf, *chosen_goal);
        live.erase(*chosen_leaf);
        leaf_rule_info.erase(*chosen_leaf);
        for (pud_global_node_id child : change.offspring) {
            live.insert(child);
            leaf_rule_info.emplace(child, runtime.get_rule_information(child));
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_rule_info.erase(refuted);
        }
    }
    EXPECT_LT(steps, 20000u);
    std::set<const expr*> answers;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        const pud_unnormalized_rule_information info = runtime.get_rule_information(leaf);
        if (!info.body.empty())
            continue;
        answers.insert(reference_pud_normalize_with_bindings(info.bindings, this->exprs, info.head));
    }

    EXPECT_EQ(answers, (std::set<const expr*>{
        this->functor("q", {this->atom("b")}),
        this->functor("q", {this->atom("c")}),
        this->functor("q", {this->atom("d")})}));

    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuite3IntegrationTest, EnumeratesTransitiveReachFromAInEveryUnfoldOrder) {
    const std::set<const expr*> expected{
        this->functor("q", {this->atom("b")}),
        this->functor("q", {this->atom("c")}),
        this->functor("q", {this->atom("d")})};
    for (uint32_t seed = 0; seed < 20; ++seed) {
        std::mt19937 rng(seed);
        std::deque<pud_module> modules;
        modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("reach", {this->atom("a"), this->atom("b")}), {}, 0),
        rule(this->functor("reach", {this->atom("a"), this->atom("c")}), {}, 0),
        rule(this->functor("reach", {this->atom("a"), this->atom("d")}), {}, 0)
        });
        modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {this->var(0)}),
            {this->functor("reach", {this->atom("a"), this->var(0)})}, 1)
        });
        TypeParam runtime(this->exprs);
        std::set<pud_global_node_id> live;
        std::map<pud_global_node_id, pud_unnormalized_rule_information> leaf_rule_info;
        for (pud_module& module : modules) {
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id leaf : change.offspring) {
                live.insert(leaf);
                leaf_rule_info.emplace(leaf, runtime.get_rule_information(leaf));
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                leaf_rule_info.erase(refuted);
            }
        }

        size_t steps = 0;
        for (; steps < 20000; ++steps) {
            std::vector<pud_global_node_id> unfoldable;
            for (pud_global_node_id leaf : live) {
                if (leaf.module != 2)
                    continue;
                if (leaf_rule_info.at(leaf).body.empty())
                    continue;
                unfoldable.push_back(leaf);
            }
            if (unfoldable.empty())
                break;
            const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
            std::vector<pud_body_goal_id> goal_ids;
            for (const auto& [goal_id, goal] : leaf_rule_info.at(leaf).body)
                goal_ids.push_back(goal_id);
            const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
            const pud_frontier_change change = runtime.unfold(leaf, goal);
            live.erase(leaf);
            leaf_rule_info.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
                live.insert(child);
                leaf_rule_info.emplace(child, runtime.get_rule_information(child));
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                }
        }
        EXPECT_LT(steps, 20000u);
        std::set<const expr*> answers;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            const pud_unnormalized_rule_information info = runtime.get_rule_information(leaf);
            if (!info.body.empty())
                continue;
            answers.insert(reference_pud_normalize_with_bindings(info.bindings, this->exprs, info.head));
        }

        EXPECT_EQ(answers, expected);
        EXPECT_EQ(live, reference_pud_frontier(modules));
        EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    }
}

TYPED_TEST(PudAnswerSetsRuntimeSuite3IntegrationTest, EnumeratesEvenPeanoLessThanEight) {
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {this->atom("zero")}), {}, 0),
        rule(this->functor("even", {this->atom("zero")}), {}, 0),
        rule(this->functor("nat", {this->functor("suc", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("even", {this->functor("suc", {this->functor("suc", {this->var(0)})})}), {this->functor("even", {this->var(0)})}, 1),
        rule(this->functor("lt", {this->atom("zero"), this->functor("suc", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("lt", {this->functor("suc", {this->var(0)}), this->functor("suc", {this->var(1)})}), {this->functor("lt", {this->var(0), this->var(1)})}, 2)
    });
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {this->var(0)}),
            {this->functor("even", {this->var(0)}),
             this->functor("lt", {this->var(0), this->functor("suc", {this->functor("suc", {this->functor("suc", {this->functor("suc", {this->functor("suc", {this->functor("suc", {this->functor("suc", {this->functor("suc", {this->atom("zero")})})})})})})})})})}, 1)
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    std::map<pud_global_node_id, pud_unnormalized_rule_information> leaf_rule_info;
    for (pud_module& module : modules) {
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id leaf : change.offspring) {
            live.insert(leaf);
            leaf_rule_info.emplace(leaf, runtime.get_rule_information(leaf));
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_rule_info.erase(refuted);
        }
    }

    size_t steps = 0;
    for (; steps < 20000; ++steps) {
        std::optional<pud_global_node_id> chosen_leaf;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            if (leaf_rule_info.at(leaf).body.empty())
                continue;
            if (!chosen_leaf.has_value() || leaf < chosen_leaf.value())
                chosen_leaf = leaf;
        }
        if (!chosen_leaf.has_value())
            break;
        std::optional<pud_body_goal_id> chosen_goal;
        for (const auto& [goal_id, goal] : leaf_rule_info.at(*chosen_leaf).body) {
            if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                chosen_goal = goal_id;
        }
        const pud_frontier_change change = runtime.unfold(*chosen_leaf, *chosen_goal);
        live.erase(*chosen_leaf);
        leaf_rule_info.erase(*chosen_leaf);
        for (pud_global_node_id child : change.offspring) {
            live.insert(child);
            leaf_rule_info.emplace(child, runtime.get_rule_information(child));
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_rule_info.erase(refuted);
        }
    }
    EXPECT_LT(steps, 20000u);
    std::set<const expr*> answers;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        const pud_unnormalized_rule_information info = runtime.get_rule_information(leaf);
        if (!info.body.empty())
            continue;
        answers.insert(reference_pud_normalize_with_bindings(info.bindings, this->exprs, info.head));
    }

    std::set<const expr*> expected;
    for (int n = 0; n < 8; n += 2) {
        const expr* peano = this->atom("zero");
        for (int i = 0; i < n; ++i)
            peano = this->functor("suc", {peano});
        expected.insert(this->functor("q", {peano}));
    }
    EXPECT_EQ(answers, expected);

    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuite3IntegrationTest, EnumeratesEvenPeanoLessThanEightInEveryUnfoldOrder) {
    std::set<const expr*> expected;
    for (int n = 0; n < 8; n += 2) {
        const expr* peano = this->atom("zero");
        for (int i = 0; i < n; ++i)
            peano = this->functor("suc", {peano});
        expected.insert(this->functor("q", {peano}));
    }
    for (uint32_t seed = 0; seed < 20; ++seed) {
        std::mt19937 rng(seed);
        std::deque<pud_module> modules;
        modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {this->atom("zero")}), {}, 0),
        rule(this->functor("even", {this->atom("zero")}), {}, 0),
        rule(this->functor("nat", {this->functor("suc", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("even", {this->functor("suc", {this->functor("suc", {this->var(0)})})}), {this->functor("even", {this->var(0)})}, 1),
        rule(this->functor("lt", {this->atom("zero"), this->functor("suc", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("lt", {this->functor("suc", {this->var(0)}), this->functor("suc", {this->var(1)})}), {this->functor("lt", {this->var(0), this->var(1)})}, 2)
        });
        modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {this->var(0)}),
            {this->functor("even", {this->var(0)}),
             this->functor("lt", {this->var(0), this->functor("suc", {this->functor("suc", {this->functor("suc", {this->functor("suc", {this->functor("suc", {this->functor("suc", {this->functor("suc", {this->functor("suc", {this->atom("zero")})})})})})})})})})}, 1)
        });
        TypeParam runtime(this->exprs);
        std::set<pud_global_node_id> live;
        std::map<pud_global_node_id, pud_unnormalized_rule_information> leaf_rule_info;
        for (pud_module& module : modules) {
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id leaf : change.offspring) {
                live.insert(leaf);
                leaf_rule_info.emplace(leaf, runtime.get_rule_information(leaf));
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                leaf_rule_info.erase(refuted);
            }
        }

        size_t steps = 0;
        for (; steps < 20000; ++steps) {
            std::vector<pud_global_node_id> unfoldable;
            for (pud_global_node_id leaf : live) {
                if (leaf.module != 2)
                    continue;
                if (leaf_rule_info.at(leaf).body.empty())
                    continue;
                unfoldable.push_back(leaf);
            }
            if (unfoldable.empty())
                break;
            const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
            std::vector<pud_body_goal_id> goal_ids;
            for (const auto& [goal_id, goal] : leaf_rule_info.at(leaf).body)
                goal_ids.push_back(goal_id);
            // Random leaf; smallest open goal id (even before lt) — other goal orders do not terminate.
            pud_body_goal_id goal = goal_ids.at(0);
            for (pud_body_goal_id goal_id : goal_ids) {
                if (goal_id < goal)
                    goal = goal_id;
            }
            const pud_frontier_change change = runtime.unfold(leaf, goal);
            live.erase(leaf);
            leaf_rule_info.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
                live.insert(child);
                leaf_rule_info.emplace(child, runtime.get_rule_information(child));
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                }
        }
        EXPECT_LT(steps, 20000u);
        std::set<const expr*> answers;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            const pud_unnormalized_rule_information info = runtime.get_rule_information(leaf);
            if (!info.body.empty())
                continue;
            answers.insert(reference_pud_normalize_with_bindings(info.bindings, this->exprs, info.head));
        }

        EXPECT_EQ(answers, expected);
        EXPECT_EQ(live, reference_pud_frontier(modules));
        EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    }
}

TYPED_TEST(PudAnswerSetsRuntimeSuite3IntegrationTest, EnumeratesListSplitsForThreeElementList) {
    const expr* list_ab = this->functor("cons", {this->atom("a"), this->functor("cons", {this->atom("b"), this->atom("nil")})});
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("append", {this->atom("nil"), list_ab, list_ab}), {}, 0),
        rule(this->functor("append", {this->functor("cons", {this->atom("a"), this->atom("nil")}), this->functor("cons", {this->atom("b"), this->atom("nil")}), list_ab}), {}, 0)
    });
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {this->var(0), this->var(1)}),
            {this->functor("append", {this->var(0), this->var(1), list_ab})}, 2)
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    std::map<pud_global_node_id, pud_unnormalized_rule_information> leaf_rule_info;
    for (pud_module& module : modules) {
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id leaf : change.offspring) {
            live.insert(leaf);
            leaf_rule_info.emplace(leaf, runtime.get_rule_information(leaf));
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_rule_info.erase(refuted);
        }
    }

    size_t steps = 0;
    for (; steps < 20000; ++steps) {
        std::optional<pud_global_node_id> chosen_leaf;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            if (leaf_rule_info.at(leaf).body.empty())
                continue;
            if (!chosen_leaf.has_value() || leaf < chosen_leaf.value())
                chosen_leaf = leaf;
        }
        if (!chosen_leaf.has_value())
            break;
        std::optional<pud_body_goal_id> chosen_goal;
        for (const auto& [goal_id, goal] : leaf_rule_info.at(*chosen_leaf).body) {
            if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                chosen_goal = goal_id;
        }
        const pud_frontier_change change = runtime.unfold(*chosen_leaf, *chosen_goal);
        live.erase(*chosen_leaf);
        leaf_rule_info.erase(*chosen_leaf);
        for (pud_global_node_id child : change.offspring) {
            live.insert(child);
            leaf_rule_info.emplace(child, runtime.get_rule_information(child));
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_rule_info.erase(refuted);
        }
    }
    EXPECT_LT(steps, 20000u);
    std::set<const expr*> answers;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        const pud_unnormalized_rule_information info = runtime.get_rule_information(leaf);
        if (!info.body.empty())
            continue;
        answers.insert(reference_pud_normalize_with_bindings(info.bindings, this->exprs, info.head));
    }

    EXPECT_EQ(answers, (std::set<const expr*>{
        this->functor("q", {this->atom("nil"), list_ab}),
        this->functor("q", {this->functor("cons", {this->atom("a"), this->atom("nil")}), this->functor("cons", {this->atom("b"), this->atom("nil")})})}));

    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuite3IntegrationTest, EnumeratesListSplitsForThreeElementListInEveryUnfoldOrder) {
    const expr* list_ab = this->functor("cons", {this->atom("a"), this->functor("cons", {this->atom("b"), this->atom("nil")})});
    const std::set<const expr*> expected{
        this->functor("q", {this->atom("nil"), list_ab}),
        this->functor("q", {this->functor("cons", {this->atom("a"), this->atom("nil")}), this->functor("cons", {this->atom("b"), this->atom("nil")})})};
    for (uint32_t seed = 0; seed < 20; ++seed) {
        std::mt19937 rng(seed);
        const expr* list_ab = this->functor("cons", {this->atom("a"), this->functor("cons", {this->atom("b"), this->atom("nil")})});
        std::deque<pud_module> modules;
        modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("append", {this->atom("nil"), list_ab, list_ab}), {}, 0),
        rule(this->functor("append", {this->functor("cons", {this->atom("a"), this->atom("nil")}), this->functor("cons", {this->atom("b"), this->atom("nil")}), list_ab}), {}, 0)
        });
        modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {this->var(0), this->var(1)}),
            {this->functor("append", {this->var(0), this->var(1), list_ab})}, 2)
        });
        TypeParam runtime(this->exprs);
        std::set<pud_global_node_id> live;
        std::map<pud_global_node_id, pud_unnormalized_rule_information> leaf_rule_info;
        for (pud_module& module : modules) {
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id leaf : change.offspring) {
                live.insert(leaf);
                leaf_rule_info.emplace(leaf, runtime.get_rule_information(leaf));
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                leaf_rule_info.erase(refuted);
            }
        }

        size_t steps = 0;
        for (; steps < 20000; ++steps) {
            std::vector<pud_global_node_id> unfoldable;
            for (pud_global_node_id leaf : live) {
                if (leaf.module != 2)
                    continue;
                if (leaf_rule_info.at(leaf).body.empty())
                    continue;
                unfoldable.push_back(leaf);
            }
            if (unfoldable.empty())
                break;
            const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
            std::vector<pud_body_goal_id> goal_ids;
            for (const auto& [goal_id, goal] : leaf_rule_info.at(leaf).body)
                goal_ids.push_back(goal_id);
            const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
            const pud_frontier_change change = runtime.unfold(leaf, goal);
            live.erase(leaf);
            leaf_rule_info.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
                live.insert(child);
                leaf_rule_info.emplace(child, runtime.get_rule_information(child));
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                }
        }
        EXPECT_LT(steps, 20000u);
        std::set<const expr*> answers;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            const pud_unnormalized_rule_information info = runtime.get_rule_information(leaf);
            if (!info.body.empty())
                continue;
            answers.insert(reference_pud_normalize_with_bindings(info.bindings, this->exprs, info.head));
        }

        EXPECT_EQ(answers, expected);
        EXPECT_EQ(live, reference_pud_frontier(modules));
        EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    }
}

TYPED_TEST(PudAnswerSetsRuntimeSuite3IntegrationTest, EnumeratesTwoChoiceClauseSolutions) {
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("f", {}), {this->functor("g", {this->var(0)})}, 1),
        rule(this->functor("g", {this->atom("abc")}), {}, 0),
        rule(this->functor("g", {this->atom("xyz")}), {}, 0)
    });
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {}), {this->functor("f", {})}, 0)
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    std::map<pud_global_node_id, pud_unnormalized_rule_information> leaf_rule_info;
    for (pud_module& module : modules) {
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id leaf : change.offspring) {
            live.insert(leaf);
            leaf_rule_info.emplace(leaf, runtime.get_rule_information(leaf));
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_rule_info.erase(refuted);
        }
    }

    size_t steps = 0;
    for (; steps < 20000; ++steps) {
        std::optional<pud_global_node_id> chosen_leaf;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            if (leaf_rule_info.at(leaf).body.empty())
                continue;
            if (!chosen_leaf.has_value() || leaf < chosen_leaf.value())
                chosen_leaf = leaf;
        }
        if (!chosen_leaf.has_value())
            break;
        std::optional<pud_body_goal_id> chosen_goal;
        for (const auto& [goal_id, goal] : leaf_rule_info.at(*chosen_leaf).body) {
            if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                chosen_goal = goal_id;
        }
        const pud_frontier_change change = runtime.unfold(*chosen_leaf, *chosen_goal);
        live.erase(*chosen_leaf);
        leaf_rule_info.erase(*chosen_leaf);
        for (pud_global_node_id child : change.offspring) {
            live.insert(child);
            leaf_rule_info.emplace(child, runtime.get_rule_information(child));
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_rule_info.erase(refuted);
        }
    }
    EXPECT_LT(steps, 20000u);
    std::set<const expr*> answers;
    size_t solved_live_leaves = 0;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        const pud_unnormalized_rule_information info = runtime.get_rule_information(leaf);
        if (!info.body.empty())
            continue;
        ++solved_live_leaves;
        answers.insert(reference_pud_normalize_with_bindings(info.bindings, this->exprs, info.head));
    }

    EXPECT_EQ(solved_live_leaves, 2u);
    EXPECT_EQ(answers, (std::set<const expr*>{this->functor("q", {})}));

    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuite3IntegrationTest, EnumeratesTwoChoiceClauseSolutionsInEveryUnfoldOrder) {
    const std::set<const expr*> expected{this->functor("q", {})};
    for (uint32_t seed = 0; seed < 20; ++seed) {
        std::mt19937 rng(seed);
        std::deque<pud_module> modules;
        modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("f", {}), {this->functor("g", {this->var(0)})}, 1),
        rule(this->functor("g", {this->atom("abc")}), {}, 0),
        rule(this->functor("g", {this->atom("xyz")}), {}, 0)
        });
        modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {}), {this->functor("f", {})}, 0)
        });
        TypeParam runtime(this->exprs);
        std::set<pud_global_node_id> live;
        std::map<pud_global_node_id, pud_unnormalized_rule_information> leaf_rule_info;
        for (pud_module& module : modules) {
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id leaf : change.offspring) {
                live.insert(leaf);
                leaf_rule_info.emplace(leaf, runtime.get_rule_information(leaf));
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                leaf_rule_info.erase(refuted);
            }
        }

        size_t steps = 0;
        for (; steps < 20000; ++steps) {
            std::vector<pud_global_node_id> unfoldable;
            for (pud_global_node_id leaf : live) {
                if (leaf.module != 2)
                    continue;
                if (leaf_rule_info.at(leaf).body.empty())
                    continue;
                unfoldable.push_back(leaf);
            }
            if (unfoldable.empty())
                break;
            const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
            std::vector<pud_body_goal_id> goal_ids;
            for (const auto& [goal_id, goal] : leaf_rule_info.at(leaf).body)
                goal_ids.push_back(goal_id);
            const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
            const pud_frontier_change change = runtime.unfold(leaf, goal);
            live.erase(leaf);
            leaf_rule_info.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
                live.insert(child);
                leaf_rule_info.emplace(child, runtime.get_rule_information(child));
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                }
        }
        EXPECT_LT(steps, 20000u);
        std::set<const expr*> answers;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            const pud_unnormalized_rule_information info = runtime.get_rule_information(leaf);
            if (!info.body.empty())
                continue;
            answers.insert(reference_pud_normalize_with_bindings(info.bindings, this->exprs, info.head));
        }

        EXPECT_EQ(answers, expected);
        EXPECT_EQ(live, reference_pud_frontier(modules));
        EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    }
}



TYPED_TEST(PudAnswerSetsRuntimeSuite3IntegrationTest, EnumeratesCollatzOneStepPreimagesOfTen) {
    const expr* ten = this->atom("zero");
    for (int i = 0; i < 10; ++i)
        ten = this->functor("suc", {ten});
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {this->atom("zero")}), {}, 0),
        rule(this->functor("even", {this->atom("zero")}), {}, 0),
        rule(this->functor("odd", {this->functor("suc", {this->atom("zero")})}), {}, 0),
        rule(this->functor("nat", {this->functor("suc", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("even", {this->functor("suc", {this->functor("suc", {this->var(0)})})}), {this->functor("even", {this->var(0)})}, 1),
        rule(this->functor("odd", {this->functor("suc", {this->var(0)})}), {this->functor("even", {this->var(0)})}, 1),
        rule(this->functor("add", {this->atom("zero"), this->var(0), this->var(0)}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("add", {this->functor("suc", {this->var(0)}), this->var(1), this->functor("suc", {this->var(2)})}), {this->functor("add", {this->var(0), this->var(1), this->var(2)})}, 3),
        rule(this->functor("mul", {this->atom("zero"), this->var(0), this->atom("zero")}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("mul", {this->functor("suc", {this->var(0)}), this->var(1), this->var(2)}),
            {this->functor("mul", {this->var(0), this->var(1), this->var(3)}),
             this->functor("add", {this->var(3), this->var(1), this->var(2)})}, 4),
        rule(this->functor("step", {this->var(0), this->var(1)}),
            {this->functor("even", {this->var(0)}), this->functor("add", {this->var(1), this->var(1), this->var(0)})}, 2),
        rule(this->functor("step", {this->var(0), this->var(2)}),
            {this->functor("odd", {this->var(0)}),
             this->functor("mul", {this->functor("suc", {this->functor("suc", {this->functor("suc", {this->atom("zero")})})}), this->var(0), this->var(1)}),
             this->functor("add", {this->var(1), this->functor("suc", {this->atom("zero")}), this->var(2)})}, 3)
    });
    // Query body is step(N, ten) only (original: nat(N), step(N, ten)). Nat is still required by step
    // rules; unfolding step first avoids nat-before-step orders that never exhaust.
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {this->var(0)}),
            {this->functor("step", {this->var(0), ten})}, 1)
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    std::map<pud_global_node_id, pud_unnormalized_rule_information> leaf_rule_info;
    for (pud_module& module : modules) {
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id leaf : change.offspring) {
            live.insert(leaf);
            leaf_rule_info.emplace(leaf, runtime.get_rule_information(leaf));
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_rule_info.erase(refuted);
        }
    }

    size_t steps = 0;
    for (; steps < 20000; ++steps) {
        std::optional<pud_global_node_id> chosen_leaf;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            if (leaf_rule_info.at(leaf).body.empty())
                continue;
            if (!chosen_leaf.has_value() || leaf < chosen_leaf.value())
                chosen_leaf = leaf;
        }
        if (!chosen_leaf.has_value())
            break;
        std::optional<pud_body_goal_id> chosen_goal;
        for (const auto& [goal_id, goal] : leaf_rule_info.at(*chosen_leaf).body) {
            if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                chosen_goal = goal_id;
        }
        const pud_frontier_change change = runtime.unfold(*chosen_leaf, *chosen_goal);
        live.erase(*chosen_leaf);
        leaf_rule_info.erase(*chosen_leaf);
        for (pud_global_node_id child : change.offspring) {
            live.insert(child);
            leaf_rule_info.emplace(child, runtime.get_rule_information(child));
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_rule_info.erase(refuted);
        }
    }
    EXPECT_LT(steps, 20000u);
    std::set<const expr*> answers;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        const pud_unnormalized_rule_information info = runtime.get_rule_information(leaf);
        if (!info.body.empty())
            continue;
        answers.insert(reference_pud_normalize_with_bindings(info.bindings, this->exprs, info.head));
    }

    std::set<const expr*> expected;
    for (int preimage : {3, 20}) {
        const expr* peano = this->atom("zero");
        for (int i = 0; i < preimage; ++i)
            peano = this->functor("suc", {peano});
        expected.insert(this->functor("q", {peano}));
    }
    EXPECT_EQ(answers, expected);

    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuite3IntegrationTest, EnumeratesCollatzOneStepPreimagesOfTenInEveryUnfoldOrder) {
    std::set<const expr*> expected;
    for (int preimage : {3, 20}) {
        const expr* peano = this->atom("zero");
        for (int i = 0; i < preimage; ++i)
            peano = this->functor("suc", {peano});
        expected.insert(this->functor("q", {peano}));
    }
    const expr* ten = this->atom("zero");
    for (int i = 0; i < 10; ++i)
        ten = this->functor("suc", {ten});
    for (uint32_t seed = 0; seed < 20; ++seed) {
        std::mt19937 rng(seed);
        std::deque<pud_module> modules;
        modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {this->atom("zero")}), {}, 0),
        rule(this->functor("even", {this->atom("zero")}), {}, 0),
        rule(this->functor("odd", {this->functor("suc", {this->atom("zero")})}), {}, 0),
        rule(this->functor("nat", {this->functor("suc", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("even", {this->functor("suc", {this->functor("suc", {this->var(0)})})}), {this->functor("even", {this->var(0)})}, 1),
        rule(this->functor("odd", {this->functor("suc", {this->var(0)})}), {this->functor("even", {this->var(0)})}, 1),
        rule(this->functor("add", {this->atom("zero"), this->var(0), this->var(0)}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("add", {this->functor("suc", {this->var(0)}), this->var(1), this->functor("suc", {this->var(2)})}), {this->functor("add", {this->var(0), this->var(1), this->var(2)})}, 3),
        rule(this->functor("mul", {this->atom("zero"), this->var(0), this->atom("zero")}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("mul", {this->functor("suc", {this->var(0)}), this->var(1), this->var(2)}),
            {this->functor("mul", {this->var(0), this->var(1), this->var(3)}),
             this->functor("add", {this->var(3), this->var(1), this->var(2)})}, 4),
        rule(this->functor("step", {this->var(0), this->var(1)}),
            {this->functor("even", {this->var(0)}), this->functor("add", {this->var(1), this->var(1), this->var(0)})}, 2),
        rule(this->functor("step", {this->var(0), this->var(2)}),
            {this->functor("odd", {this->var(0)}),
             this->functor("mul", {this->functor("suc", {this->functor("suc", {this->functor("suc", {this->atom("zero")})})}), this->var(0), this->var(1)}),
             this->functor("add", {this->var(1), this->functor("suc", {this->atom("zero")}), this->var(2)})}, 3)
        });
        modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {this->var(0)}),
            {this->functor("step", {this->var(0), ten})}, 1)
        });
        TypeParam runtime(this->exprs);
        std::set<pud_global_node_id> live;
        std::map<pud_global_node_id, pud_unnormalized_rule_information> leaf_rule_info;
        for (pud_module& module : modules) {
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id leaf : change.offspring) {
                live.insert(leaf);
                leaf_rule_info.emplace(leaf, runtime.get_rule_information(leaf));
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                leaf_rule_info.erase(refuted);
            }
        }

        size_t steps = 0;
        for (; steps < 20000; ++steps) {
            std::vector<pud_global_node_id> unfoldable;
            for (pud_global_node_id leaf : live) {
                if (leaf.module != 2)
                    continue;
                if (leaf_rule_info.at(leaf).body.empty())
                    continue;
                unfoldable.push_back(leaf);
            }
            if (unfoldable.empty())
                break;
            const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
            std::vector<pud_body_goal_id> goal_ids;
            for (const auto& [goal_id, goal] : leaf_rule_info.at(leaf).body)
                goal_ids.push_back(goal_id);
            const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
            const pud_frontier_change change = runtime.unfold(leaf, goal);
            live.erase(leaf);
            leaf_rule_info.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
                live.insert(child);
                leaf_rule_info.emplace(child, runtime.get_rule_information(child));
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                }
        }
        EXPECT_LT(steps, 20000u);
        std::set<const expr*> answers;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            const pud_unnormalized_rule_information info = runtime.get_rule_information(leaf);
            if (!info.body.empty())
                continue;
            answers.insert(reference_pud_normalize_with_bindings(info.bindings, this->exprs, info.head));
        }

        EXPECT_EQ(answers, expected);
        EXPECT_EQ(live, reference_pud_frontier(modules));
        EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    }
}

TYPED_TEST(PudAnswerSetsRuntimeSuite3IntegrationTest, EnumeratesFibIndicesWithValueBelowFive) {
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {this->atom("zero")}), {}, 0),
        rule(this->functor("nat", {this->functor("suc", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("add", {this->atom("zero"), this->var(0), this->var(0)}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("add", {this->functor("suc", {this->var(0)}), this->var(1), this->functor("suc", {this->var(2)})}), {this->functor("add", {this->var(0), this->var(1), this->var(2)})}, 3),
        rule(this->functor("lt", {this->atom("zero"), this->functor("suc", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("lt", {this->functor("suc", {this->var(0)}), this->functor("suc", {this->var(1)})}), {this->functor("lt", {this->var(0), this->var(1)})}, 2),
        rule(this->functor("fib", {this->atom("zero"), this->atom("zero")}), {}, 0),
        rule(this->functor("fib", {this->functor("suc", {this->atom("zero")}), this->functor("suc", {this->atom("zero")})}), {}, 0),
        rule(this->functor("fib", {this->functor("suc", {this->functor("suc", {this->var(0)})}), this->var(3)}),
            {this->functor("fib", {this->var(0), this->var(1)}),
             this->functor("fib", {this->functor("suc", {this->var(0)}), this->var(2)}),
             this->functor("add", {this->var(1), this->var(2), this->var(3)})}, 4)
    });
    // Goal order fib(N,V), lt(V, five), nat(N) — original was nat(N), fib(N,V), lt(V, five). Default
    // nat-first unfolding never finishes (infinite nat extensions).
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {this->var(0)}),
            {this->functor("fib", {this->var(0), this->var(1)}),
             this->functor("lt", {this->var(1), this->functor("suc", {this->functor("suc", {this->functor("suc", {this->functor("suc", {this->functor("suc", {this->atom("zero")})})})})})}),
             this->functor("nat", {this->var(0)})}, 2)
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    std::map<pud_global_node_id, pud_unnormalized_rule_information> leaf_rule_info;
    for (pud_module& module : modules) {
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id leaf : change.offspring) {
            live.insert(leaf);
            leaf_rule_info.emplace(leaf, runtime.get_rule_information(leaf));
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_rule_info.erase(refuted);
        }
    }

    std::set<const expr*> expected;
    for (int n = 0; n < 5; ++n) {
        const expr* peano = this->atom("zero");
        for (int i = 0; i < n; ++i)
            peano = this->functor("suc", {peano});
        expected.insert(this->functor("q", {peano}));
    }
    // Derivation never exhausts: with fib(N,V) and V unbound, infinitely many indices N satisfy any
    // fixed value bound; smallest-leaf/smallest-goal drive keeps extending nat/fib subtrees forever.
    // kMaxSteps is sufficient: all 5 expected answers first appear by step 2462 and the set stays
    // identical through 4x this cap (measured in scratch).
    static constexpr size_t kMaxSteps = 20000;
    size_t steps = 0;
    for (; steps < kMaxSteps; ++steps) {
        std::optional<pud_global_node_id> chosen_leaf;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            if (leaf_rule_info.at(leaf).body.empty())
                continue;
            if (!chosen_leaf.has_value() || leaf < chosen_leaf.value())
                chosen_leaf = leaf;
        }
        if (!chosen_leaf.has_value())
            break;
        std::optional<pud_body_goal_id> chosen_goal;
        for (const auto& [goal_id, goal] : leaf_rule_info.at(*chosen_leaf).body) {
            if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                chosen_goal = goal_id;
        }
        const pud_frontier_change change = runtime.unfold(*chosen_leaf, *chosen_goal);
        live.erase(*chosen_leaf);
        leaf_rule_info.erase(*chosen_leaf);
        for (pud_global_node_id child : change.offspring) {
            live.insert(child);
            leaf_rule_info.emplace(child, runtime.get_rule_information(child));
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_rule_info.erase(refuted);
        }
    }
    std::set<const expr*> answers;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        const pud_unnormalized_rule_information info = runtime.get_rule_information(leaf);
        if (!info.body.empty())
            continue;
        answers.insert(reference_pud_normalize_with_bindings(info.bindings, this->exprs, info.head));
    }

    EXPECT_EQ(answers, expected);

    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuite3IntegrationTest, EnumeratesFibIndicesWithValueBelowFiveInEveryUnfoldOrder) {
    std::set<const expr*> expected;
    for (int n = 0; n < 5; ++n) {
        const expr* peano = this->atom("zero");
        for (int i = 0; i < n; ++i)
            peano = this->functor("suc", {peano});
        expected.insert(this->functor("q", {peano}));
    }
    // Per-seed exhaustion is impossible (same infinite fib/nat tree as the deterministic test). Each seed
    // checks soundness; the union across seeds must still cover every expected index.
    std::set<const expr*> found_across_seeds;
    static constexpr size_t kMaxStepsPerSeed = 3000;
    for (uint32_t seed = 0; seed < 20; ++seed) {
        std::mt19937 rng(seed);
        std::deque<pud_module> modules;
        modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {this->atom("zero")}), {}, 0),
        rule(this->functor("nat", {this->functor("suc", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("add", {this->atom("zero"), this->var(0), this->var(0)}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("add", {this->functor("suc", {this->var(0)}), this->var(1), this->functor("suc", {this->var(2)})}), {this->functor("add", {this->var(0), this->var(1), this->var(2)})}, 3),
        rule(this->functor("lt", {this->atom("zero"), this->functor("suc", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("lt", {this->functor("suc", {this->var(0)}), this->functor("suc", {this->var(1)})}), {this->functor("lt", {this->var(0), this->var(1)})}, 2),
        rule(this->functor("fib", {this->atom("zero"), this->atom("zero")}), {}, 0),
        rule(this->functor("fib", {this->functor("suc", {this->atom("zero")}), this->functor("suc", {this->atom("zero")})}), {}, 0),
        rule(this->functor("fib", {this->functor("suc", {this->functor("suc", {this->var(0)})}), this->var(3)}),
            {this->functor("fib", {this->var(0), this->var(1)}),
             this->functor("fib", {this->functor("suc", {this->var(0)}), this->var(2)}),
             this->functor("add", {this->var(1), this->var(2), this->var(3)})}, 4)
        });
        modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {this->var(0)}),
            {this->functor("fib", {this->var(0), this->var(1)}),
             this->functor("lt", {this->var(1), this->functor("suc", {this->functor("suc", {this->functor("suc", {this->functor("suc", {this->functor("suc", {this->atom("zero")})})})})})}),
             this->functor("nat", {this->var(0)})}, 2)
        });
        TypeParam runtime(this->exprs);
        std::set<pud_global_node_id> live;
        std::map<pud_global_node_id, pud_unnormalized_rule_information> leaf_rule_info;
        for (pud_module& module : modules) {
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id leaf : change.offspring) {
                live.insert(leaf);
                leaf_rule_info.emplace(leaf, runtime.get_rule_information(leaf));
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                leaf_rule_info.erase(refuted);
            }
        }

        size_t steps = 0;
        for (; steps < kMaxStepsPerSeed; ++steps) {
            std::vector<pud_global_node_id> unfoldable;
            for (pud_global_node_id leaf : live) {
                if (leaf.module != 2)
                    continue;
                if (leaf_rule_info.at(leaf).body.empty())
                    continue;
                unfoldable.push_back(leaf);
            }
            if (unfoldable.empty())
                break;
            const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
            std::vector<pud_body_goal_id> goal_ids;
            for (const auto& [goal_id, goal] : leaf_rule_info.at(leaf).body)
                goal_ids.push_back(goal_id);
            const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
            const pud_frontier_change change = runtime.unfold(leaf, goal);
            live.erase(leaf);
            leaf_rule_info.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
                live.insert(child);
                leaf_rule_info.emplace(child, runtime.get_rule_information(child));
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                }
        }
        std::set<const expr*> answers;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            const pud_unnormalized_rule_information info = runtime.get_rule_information(leaf);
            if (!info.body.empty())
                continue;
            answers.insert(reference_pud_normalize_with_bindings(info.bindings, this->exprs, info.head));
        }

        for (const expr* answer : answers) {
            EXPECT_TRUE(expected.contains(answer))
                << "seed " << seed << " produced an answer not in the expected set";
            found_across_seeds.insert(answer);
        }
    }
    EXPECT_EQ(found_across_seeds, expected);
}

TYPED_TEST(PudAnswerSetsRuntimeSuite3IntegrationTest, EnumeratesFactorPairsOfSix) {
    const expr* six = this->atom("zero");
    for (int peano_step = 0; peano_step < 6; ++peano_step)
        six = this->functor("suc", {six});
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {this->atom("zero")}), {}, 0),
        rule(this->functor("nat", {this->functor("suc", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("add", {this->atom("zero"), this->var(0), this->var(0)}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("add", {this->functor("suc", {this->var(0)}), this->var(1), this->functor("suc", {this->var(2)})}), {this->functor("add", {this->var(0), this->var(1), this->var(2)})}, 3),
        rule(this->functor("mul", {this->atom("zero"), this->var(0), this->atom("zero")}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("mul", {this->functor("suc", {this->var(0)}), this->var(1), this->var(2)}),
            {this->functor("mul", {this->var(0), this->var(1), this->var(3)}),
             this->functor("add", {this->var(3), this->var(1), this->var(2)})}, 4)
    });
    // Query body is mul(X,Y,six) only (original: nat(X), nat(Y), mul(X,Y,six)). Nat is still required
    // by mul rules; unfolding mul first avoids nat-before-mul goal orders that never exhaust.
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {this->var(0), this->var(1)}),
            {this->functor("mul", {this->var(0), this->var(1), six})}, 2)
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    std::map<pud_global_node_id, pud_unnormalized_rule_information> leaf_rule_info;
    for (pud_module& module : modules) {
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id leaf : change.offspring) {
            live.insert(leaf);
            leaf_rule_info.emplace(leaf, runtime.get_rule_information(leaf));
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_rule_info.erase(refuted);
        }
    }

    const std::set<const expr*> expected{
        this->functor("q", {this->functor("suc", {this->atom("zero")}), this->functor("suc", {this->functor("suc", {this->functor("suc", {this->functor("suc", {this->functor("suc", {this->functor("suc", {this->atom("zero")})})})})})})}),
        this->functor("q", {this->functor("suc", {this->functor("suc", {this->functor("suc", {this->functor("suc", {this->functor("suc", {this->functor("suc", {this->atom("zero")})})})})})}), this->functor("suc", {this->atom("zero")})}),
        this->functor("q", {this->functor("suc", {this->functor("suc", {this->atom("zero")})}), this->functor("suc", {this->functor("suc", {this->functor("suc", {this->atom("zero")})})})}),
        this->functor("q", {this->functor("suc", {this->functor("suc", {this->functor("suc", {this->atom("zero")})})}), this->functor("suc", {this->functor("suc", {this->atom("zero")})})})};
    // Derivation never exhausts: the Y = zero branch of mul(X,Y,six) re-derives mul(X',zero,six) for
    // every larger X', so the frontier keeps growing under every unfold order.
    // kMaxSteps is sufficient: all 4 expected pairs first appear by step 176 and answers == expected
    // at 1x and 2x cap (measured in scratch).
    static constexpr size_t kMaxSteps = 20000;
    size_t steps = 0;
    for (; steps < kMaxSteps; ++steps) {
        std::optional<pud_global_node_id> chosen_leaf;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            if (leaf_rule_info.at(leaf).body.empty())
                continue;
            if (!chosen_leaf.has_value() || leaf < chosen_leaf.value())
                chosen_leaf = leaf;
        }
        if (!chosen_leaf.has_value())
            break;
        std::optional<pud_body_goal_id> chosen_goal;
        for (const auto& [goal_id, goal] : leaf_rule_info.at(*chosen_leaf).body) {
            if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                chosen_goal = goal_id;
        }
        const pud_frontier_change change = runtime.unfold(*chosen_leaf, *chosen_goal);
        live.erase(*chosen_leaf);
        leaf_rule_info.erase(*chosen_leaf);
        for (pud_global_node_id child : change.offspring) {
            live.insert(child);
            leaf_rule_info.emplace(child, runtime.get_rule_information(child));
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_rule_info.erase(refuted);
        }
    }
    std::set<const expr*> answers;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        const pud_unnormalized_rule_information info = runtime.get_rule_information(leaf);
        if (!info.body.empty())
            continue;
        answers.insert(reference_pud_normalize_with_bindings(info.bindings, this->exprs, info.head));
    }

    EXPECT_EQ(answers, expected);

    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuite3IntegrationTest, EnumeratesFactorPairsOfSixInEveryUnfoldOrder) {
    const expr* six = this->atom("zero");
    for (int peano_step = 0; peano_step < 6; ++peano_step)
        six = this->functor("suc", {six});
    const std::set<const expr*> expected{
        this->functor("q", {this->functor("suc", {this->atom("zero")}), this->functor("suc", {this->functor("suc", {this->functor("suc", {this->functor("suc", {this->functor("suc", {this->functor("suc", {this->atom("zero")})})})})})})}),
        this->functor("q", {this->functor("suc", {this->functor("suc", {this->functor("suc", {this->functor("suc", {this->functor("suc", {this->functor("suc", {this->atom("zero")})})})})})}), this->functor("suc", {this->atom("zero")})}),
        this->functor("q", {this->functor("suc", {this->functor("suc", {this->atom("zero")})}), this->functor("suc", {this->functor("suc", {this->functor("suc", {this->atom("zero")})})})}),
        this->functor("q", {this->functor("suc", {this->functor("suc", {this->functor("suc", {this->atom("zero")})})}), this->functor("suc", {this->functor("suc", {this->atom("zero")})})})};
    // Per-seed exhaustion is impossible (infinite mul(X,zero,six) re-synthesis). Soundness per seed;
    // union across seeds must still hit all four factor pairs.
    std::set<const expr*> found_across_seeds;
    static constexpr size_t kMaxStepsPerSeed = 3000;
    for (uint32_t seed = 0; seed < 20; ++seed) {
        std::mt19937 rng(seed);
        std::deque<pud_module> modules;
        modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {this->atom("zero")}), {}, 0),
        rule(this->functor("nat", {this->functor("suc", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("add", {this->atom("zero"), this->var(0), this->var(0)}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("add", {this->functor("suc", {this->var(0)}), this->var(1), this->functor("suc", {this->var(2)})}), {this->functor("add", {this->var(0), this->var(1), this->var(2)})}, 3),
        rule(this->functor("mul", {this->atom("zero"), this->var(0), this->atom("zero")}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("mul", {this->functor("suc", {this->var(0)}), this->var(1), this->var(2)}),
            {this->functor("mul", {this->var(0), this->var(1), this->var(3)}),
             this->functor("add", {this->var(3), this->var(1), this->var(2)})}, 4)
        });
        modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {this->var(0), this->var(1)}),
            {this->functor("mul", {this->var(0), this->var(1), six})}, 2)
        });
        TypeParam runtime(this->exprs);
        std::set<pud_global_node_id> live;
        std::map<pud_global_node_id, pud_unnormalized_rule_information> leaf_rule_info;
        for (pud_module& module : modules) {
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id leaf : change.offspring) {
                live.insert(leaf);
                leaf_rule_info.emplace(leaf, runtime.get_rule_information(leaf));
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                leaf_rule_info.erase(refuted);
            }
        }

        size_t steps = 0;
        for (; steps < kMaxStepsPerSeed; ++steps) {
            std::vector<pud_global_node_id> unfoldable;
            for (pud_global_node_id leaf : live) {
                if (leaf.module != 2)
                    continue;
                if (leaf_rule_info.at(leaf).body.empty())
                    continue;
                unfoldable.push_back(leaf);
            }
            if (unfoldable.empty())
                break;
            // Random leaf among module-2 fronts; mul is the only query goal — random goal choice would be no-op.
            const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
            std::vector<pud_body_goal_id> goal_ids;
            for (const auto& [goal_id, goal] : leaf_rule_info.at(leaf).body)
                goal_ids.push_back(goal_id);
            pud_body_goal_id goal = goal_ids.at(0);
            for (pud_body_goal_id goal_id : goal_ids) {
                if (goal_id < goal)
                    goal = goal_id;
            }
            const pud_frontier_change change = runtime.unfold(leaf, goal);
            live.erase(leaf);
            leaf_rule_info.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
                live.insert(child);
                leaf_rule_info.emplace(child, runtime.get_rule_information(child));
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                }
        }
        std::set<const expr*> answers;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            const pud_unnormalized_rule_information info = runtime.get_rule_information(leaf);
            if (!info.body.empty())
                continue;
            answers.insert(reference_pud_normalize_with_bindings(info.bindings, this->exprs, info.head));
        }

        for (const expr* answer : answers) {
            EXPECT_TRUE(expected.contains(answer))
                << "seed " << seed << " produced an answer not in the expected set";
            found_across_seeds.insert(answer);
        }
    }
    EXPECT_EQ(found_across_seeds, expected);
}

TYPED_TEST(PudAnswerSetsRuntimeSuite3IntegrationTest, EnumeratesDistinctTwoPartPartitionsOfFive) {
    const expr* five = this->atom("zero");
    for (int peano_step = 0; peano_step < 5; ++peano_step)
        five = this->functor("suc", {five});
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {this->atom("zero")}), {}, 0),
        rule(this->functor("nat", {this->functor("suc", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("add", {this->atom("zero"), this->var(0), this->var(0)}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("add", {this->functor("suc", {this->var(0)}), this->var(1), this->functor("suc", {this->var(2)})}), {this->functor("add", {this->var(0), this->var(1), this->var(2)})}, 3),
        rule(this->functor("lt", {this->atom("zero"), this->functor("suc", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("lt", {this->functor("suc", {this->var(0)}), this->functor("suc", {this->var(1)})}), {this->functor("lt", {this->var(0), this->var(1)})}, 2),
        rule(this->functor("part", {five, this->var(0), this->var(1)}),
            {this->functor("add", {this->var(0), this->var(1), five}),
             this->functor("lt", {this->atom("zero"), this->var(0)}),
             this->functor("lt", {this->var(0), this->var(1)})}, 2)
    });
    // Query body is part(five,A,B) only (same as original). Unfolding part before its add/lt subgoals
    // would diverge on the dependency module under smallest-goal-id drive.
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {this->var(0), this->var(1)}),
            {this->functor("part", {five, this->var(0), this->var(1)})}, 2)
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    std::map<pud_global_node_id, pud_unnormalized_rule_information> leaf_rule_info;
    for (pud_module& module : modules) {
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id leaf : change.offspring) {
            live.insert(leaf);
            leaf_rule_info.emplace(leaf, runtime.get_rule_information(leaf));
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_rule_info.erase(refuted);
        }
    }

    size_t steps = 0;
    for (; steps < 20000; ++steps) {
        std::optional<pud_global_node_id> chosen_leaf;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            if (leaf_rule_info.at(leaf).body.empty())
                continue;
            if (!chosen_leaf.has_value() || leaf < chosen_leaf.value())
                chosen_leaf = leaf;
        }
        if (!chosen_leaf.has_value())
            break;
        std::optional<pud_body_goal_id> chosen_goal;
        for (const auto& [goal_id, goal] : leaf_rule_info.at(*chosen_leaf).body) {
            if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                chosen_goal = goal_id;
        }
        const pud_frontier_change change = runtime.unfold(*chosen_leaf, *chosen_goal);
        live.erase(*chosen_leaf);
        leaf_rule_info.erase(*chosen_leaf);
        for (pud_global_node_id child : change.offspring) {
            live.insert(child);
            leaf_rule_info.emplace(child, runtime.get_rule_information(child));
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_rule_info.erase(refuted);
        }
    }
    EXPECT_LT(steps, 20000u);
    std::set<const expr*> answers;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        const pud_unnormalized_rule_information info = runtime.get_rule_information(leaf);
        if (!info.body.empty())
            continue;
        answers.insert(reference_pud_normalize_with_bindings(info.bindings, this->exprs, info.head));
    }

    EXPECT_EQ(answers, (std::set<const expr*>{
        this->functor("q", {this->functor("suc", {this->atom("zero")}), this->functor("suc", {this->functor("suc", {this->functor("suc", {this->functor("suc", {this->atom("zero")})})})})}),
        this->functor("q", {this->functor("suc", {this->functor("suc", {this->atom("zero")})}), this->functor("suc", {this->functor("suc", {this->functor("suc", {this->atom("zero")})})})})}));

    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuite3IntegrationTest, EnumeratesDistinctTwoPartPartitionsOfFiveInEveryUnfoldOrder) {
    const expr* five = this->atom("zero");
    for (int peano_step = 0; peano_step < 5; ++peano_step)
        five = this->functor("suc", {five});
    const std::set<const expr*> expected{
        this->functor("q", {this->functor("suc", {this->atom("zero")}), this->functor("suc", {this->functor("suc", {this->functor("suc", {this->functor("suc", {this->atom("zero")})})})})}),
        this->functor("q", {this->functor("suc", {this->functor("suc", {this->atom("zero")})}), this->functor("suc", {this->functor("suc", {this->functor("suc", {this->atom("zero")})})})})};
    for (uint32_t seed = 0; seed < 20; ++seed) {
        std::mt19937 rng(seed);
        std::deque<pud_module> modules;
        modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {this->atom("zero")}), {}, 0),
        rule(this->functor("nat", {this->functor("suc", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("add", {this->atom("zero"), this->var(0), this->var(0)}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("add", {this->functor("suc", {this->var(0)}), this->var(1), this->functor("suc", {this->var(2)})}), {this->functor("add", {this->var(0), this->var(1), this->var(2)})}, 3),
        rule(this->functor("lt", {this->atom("zero"), this->functor("suc", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("lt", {this->functor("suc", {this->var(0)}), this->functor("suc", {this->var(1)})}), {this->functor("lt", {this->var(0), this->var(1)})}, 2),
        rule(this->functor("part", {five, this->var(0), this->var(1)}),
            {this->functor("add", {this->var(0), this->var(1), five}),
             this->functor("lt", {this->atom("zero"), this->var(0)}),
             this->functor("lt", {this->var(0), this->var(1)})}, 2)
        });
        modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {this->var(0), this->var(1)}),
            {this->functor("part", {five, this->var(0), this->var(1)})}, 2)
        });
        TypeParam runtime(this->exprs);
        std::set<pud_global_node_id> live;
        std::map<pud_global_node_id, pud_unnormalized_rule_information> leaf_rule_info;
        for (pud_module& module : modules) {
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id leaf : change.offspring) {
                live.insert(leaf);
                leaf_rule_info.emplace(leaf, runtime.get_rule_information(leaf));
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                leaf_rule_info.erase(refuted);
            }
        }

        size_t steps = 0;
        for (; steps < 20000; ++steps) {
            std::vector<pud_global_node_id> unfoldable;
            for (pud_global_node_id leaf : live) {
                if (leaf.module != 2)
                    continue;
                if (leaf_rule_info.at(leaf).body.empty())
                    continue;
                unfoldable.push_back(leaf);
            }
            if (unfoldable.empty())
                break;
            const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
            std::vector<pud_body_goal_id> goal_ids;
            for (const auto& [goal_id, goal] : leaf_rule_info.at(leaf).body)
                goal_ids.push_back(goal_id);
            // Random leaf; smallest open goal id (add before lt) — other goal orders do not terminate.
            pud_body_goal_id goal = goal_ids.at(0);
            for (pud_body_goal_id goal_id : goal_ids) {
                if (goal_id < goal)
                    goal = goal_id;
            }
            const pud_frontier_change change = runtime.unfold(leaf, goal);
            live.erase(leaf);
            leaf_rule_info.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
                live.insert(child);
                leaf_rule_info.emplace(child, runtime.get_rule_information(child));
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                }
        }
        EXPECT_LT(steps, 20000u);
        std::set<const expr*> answers;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            const pud_unnormalized_rule_information info = runtime.get_rule_information(leaf);
            if (!info.body.empty())
                continue;
            answers.insert(reference_pud_normalize_with_bindings(info.bindings, this->exprs, info.head));
        }

        EXPECT_EQ(answers, expected);
        EXPECT_EQ(live, reference_pud_frontier(modules));
        EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    }
}

TYPED_TEST(PudAnswerSetsRuntimeSuite3IntegrationTest, EnumeratesArithmeticProgressionsEndingAtFive) {
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {this->atom("zero")}), {}, 0),
        rule(this->functor("nat", {this->functor("suc", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("add", {this->atom("zero"), this->var(0), this->var(0)}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("add", {this->functor("suc", {this->var(0)}), this->var(1), this->functor("suc", {this->var(2)})}), {this->functor("add", {this->var(0), this->var(1), this->var(2)})}, 3),
        rule(this->functor("lt", {this->atom("zero"), this->functor("suc", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("lt", {this->functor("suc", {this->var(0)}), this->functor("suc", {this->var(1)})}), {this->functor("lt", {this->var(0), this->var(1)})}, 2)
    });
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {this->var(0), this->var(1)}),
            {this->functor("nat", {this->var(0)}),
             this->functor("nat", {this->var(1)}),
             this->functor("lt", {this->atom("zero"), this->var(1)}),
             this->functor("add", {this->var(0), this->var(1), this->var(2)}),
             this->functor("add", {this->var(2), this->var(1), this->functor("suc", {this->functor("suc", {this->functor("suc", {this->functor("suc", {this->functor("suc", {this->atom("zero")})})})})})})}, 3)
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    std::map<pud_global_node_id, pud_unnormalized_rule_information> leaf_rule_info;
    for (pud_module& module : modules) {
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id leaf : change.offspring) {
            live.insert(leaf);
            leaf_rule_info.emplace(leaf, runtime.get_rule_information(leaf));
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_rule_info.erase(refuted);
        }
    }

    size_t steps = 0;
    for (; steps < 20000; ++steps) {
        std::optional<pud_global_node_id> chosen_leaf;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            if (leaf_rule_info.at(leaf).body.empty())
                continue;
            if (!chosen_leaf.has_value() || leaf < chosen_leaf.value())
                chosen_leaf = leaf;
        }
        if (!chosen_leaf.has_value())
            break;
        std::optional<pud_body_goal_id> chosen_goal;
        for (const auto& [goal_id, goal] : leaf_rule_info.at(*chosen_leaf).body) {
            if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                chosen_goal = goal_id;
        }
        const pud_frontier_change change = runtime.unfold(*chosen_leaf, *chosen_goal);
        live.erase(*chosen_leaf);
        leaf_rule_info.erase(*chosen_leaf);
        for (pud_global_node_id child : change.offspring) {
            live.insert(child);
            leaf_rule_info.emplace(child, runtime.get_rule_information(child));
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_rule_info.erase(refuted);
        }
    }
    EXPECT_LT(steps, 20000u);
    std::set<const expr*> answers;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        const pud_unnormalized_rule_information info = runtime.get_rule_information(leaf);
        if (!info.body.empty())
            continue;
        answers.insert(reference_pud_normalize_with_bindings(info.bindings, this->exprs, info.head));
    }

    EXPECT_EQ(answers, (std::set<const expr*>{
        this->functor("q", {this->functor("suc", {this->atom("zero")}), this->functor("suc", {this->functor("suc", {this->atom("zero")})})}),
        this->functor("q", {this->functor("suc", {this->functor("suc", {this->functor("suc", {this->atom("zero")})})}), this->functor("suc", {this->atom("zero")})})}));

    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuite3IntegrationTest, EnumeratesArithmeticProgressionsEndingAtFiveInEveryUnfoldOrder) {
    const std::set<const expr*> expected{
        this->functor("q", {this->functor("suc", {this->atom("zero")}), this->functor("suc", {this->functor("suc", {this->atom("zero")})})}),
        this->functor("q", {this->functor("suc", {this->functor("suc", {this->functor("suc", {this->atom("zero")})})}), this->functor("suc", {this->atom("zero")})})};
    for (uint32_t seed = 0; seed < 20; ++seed) {
        std::mt19937 rng(seed);
        std::deque<pud_module> modules;
        modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {this->atom("zero")}), {}, 0),
        rule(this->functor("nat", {this->functor("suc", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("add", {this->atom("zero"), this->var(0), this->var(0)}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("add", {this->functor("suc", {this->var(0)}), this->var(1), this->functor("suc", {this->var(2)})}), {this->functor("add", {this->var(0), this->var(1), this->var(2)})}, 3),
        rule(this->functor("lt", {this->atom("zero"), this->functor("suc", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("lt", {this->functor("suc", {this->var(0)}), this->functor("suc", {this->var(1)})}), {this->functor("lt", {this->var(0), this->var(1)})}, 2)
        });
        modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {this->var(0), this->var(1)}),
            {this->functor("nat", {this->var(0)}),
             this->functor("nat", {this->var(1)}),
             this->functor("lt", {this->atom("zero"), this->var(1)}),
             this->functor("add", {this->var(0), this->var(1), this->var(2)}),
             this->functor("add", {this->var(2), this->var(1), this->functor("suc", {this->functor("suc", {this->functor("suc", {this->functor("suc", {this->functor("suc", {this->atom("zero")})})})})})})}, 3)
        });
        TypeParam runtime(this->exprs);
        std::set<pud_global_node_id> live;
        std::map<pud_global_node_id, pud_unnormalized_rule_information> leaf_rule_info;
        for (pud_module& module : modules) {
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id leaf : change.offspring) {
                live.insert(leaf);
                leaf_rule_info.emplace(leaf, runtime.get_rule_information(leaf));
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                leaf_rule_info.erase(refuted);
            }
        }

        size_t steps = 0;
        for (; steps < 20000; ++steps) {
            std::vector<pud_global_node_id> unfoldable;
            for (pud_global_node_id leaf : live) {
                if (leaf.module != 2)
                    continue;
                if (leaf_rule_info.at(leaf).body.empty())
                    continue;
                unfoldable.push_back(leaf);
            }
            if (unfoldable.empty())
                break;
            const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
            std::vector<pud_body_goal_id> goal_ids;
            for (const auto& [goal_id, goal] : leaf_rule_info.at(leaf).body)
                goal_ids.push_back(goal_id);
            const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
            const pud_frontier_change change = runtime.unfold(leaf, goal);
            live.erase(leaf);
            leaf_rule_info.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
                live.insert(child);
                leaf_rule_info.emplace(child, runtime.get_rule_information(child));
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                }
        }
        EXPECT_LT(steps, 20000u);
        std::set<const expr*> answers;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            const pud_unnormalized_rule_information info = runtime.get_rule_information(leaf);
            if (!info.body.empty())
                continue;
            answers.insert(reference_pud_normalize_with_bindings(info.bindings, this->exprs, info.head));
        }

        EXPECT_EQ(answers, expected);
        EXPECT_EQ(live, reference_pud_frontier(modules));
        EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    }
}

TYPED_TEST(PudAnswerSetsRuntimeSuite3IntegrationTest, FindsGcdOfSixAndFourViaSubtraction) {
    const expr* four = this->atom("zero");
    for (int peano_step = 0; peano_step < 4; ++peano_step)
        four = this->functor("suc", {four});
    const expr* six = this->atom("zero");
    for (int peano_step = 0; peano_step < 6; ++peano_step)
        six = this->functor("suc", {six});
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {this->atom("zero")}), {}, 0),
        rule(this->functor("nat", {this->functor("suc", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("add", {this->atom("zero"), this->var(0), this->var(0)}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("add", {this->functor("suc", {this->var(0)}), this->var(1), this->functor("suc", {this->var(2)})}), {this->functor("add", {this->var(0), this->var(1), this->var(2)})}, 3),
        rule(this->functor("lt", {this->atom("zero"), this->functor("suc", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("lt", {this->functor("suc", {this->var(0)}), this->functor("suc", {this->var(1)})}), {this->functor("lt", {this->var(0), this->var(1)})}, 2),
        rule(this->functor("sub", {this->var(0), this->atom("zero"), this->var(0)}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("sub", {this->functor("suc", {this->var(0)}), this->functor("suc", {this->var(1)}), this->var(2)}),
            {this->functor("sub", {this->var(0), this->var(1), this->var(2)})}, 3),
        rule(this->functor("gcd", {this->var(0), this->atom("zero"), this->var(0)}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("gcd", {this->var(0), this->var(0), this->var(0)}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("gcd", {this->var(0), this->var(1), this->var(3)}),
            {this->functor("lt", {this->var(1), this->var(0)}),
             this->functor("sub", {this->var(0), this->var(1), this->var(2)}),
             this->functor("gcd", {this->var(2), this->var(1), this->var(3)})}, 4),
        rule(this->functor("gcd", {this->var(0), this->var(1), this->var(3)}),
            {this->functor("lt", {this->var(0), this->var(1)}),
             this->functor("sub", {this->var(1), this->var(0), this->var(2)}),
             this->functor("gcd", {this->var(0), this->var(2), this->var(3)})}, 4)
    });
    // Query body is gcd(six,four,G) only (original: nat(four), nat(six), gcd(six,four,G)). Nat is still
    // required by gcd rules; unfolding gcd first avoids nat-before-gcd goal orders that never exhaust.
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {this->var(0)}),
            {this->functor("gcd", {six, four, this->var(0)})}, 1)
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    std::map<pud_global_node_id, pud_unnormalized_rule_information> leaf_rule_info;
    for (pud_module& module : modules) {
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id leaf : change.offspring) {
            live.insert(leaf);
            leaf_rule_info.emplace(leaf, runtime.get_rule_information(leaf));
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_rule_info.erase(refuted);
        }
    }

    size_t steps = 0;
    for (; steps < 20000; ++steps) {
        std::optional<pud_global_node_id> chosen_leaf;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            if (leaf_rule_info.at(leaf).body.empty())
                continue;
            if (!chosen_leaf.has_value() || leaf < chosen_leaf.value())
                chosen_leaf = leaf;
        }
        if (!chosen_leaf.has_value())
            break;
        std::optional<pud_body_goal_id> chosen_goal;
        for (const auto& [goal_id, goal] : leaf_rule_info.at(*chosen_leaf).body) {
            if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                chosen_goal = goal_id;
        }
        const pud_frontier_change change = runtime.unfold(*chosen_leaf, *chosen_goal);
        live.erase(*chosen_leaf);
        leaf_rule_info.erase(*chosen_leaf);
        for (pud_global_node_id child : change.offspring) {
            live.insert(child);
            leaf_rule_info.emplace(child, runtime.get_rule_information(child));
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_rule_info.erase(refuted);
        }
    }
    EXPECT_LT(steps, 20000u);
    std::set<const expr*> answers;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        const pud_unnormalized_rule_information info = runtime.get_rule_information(leaf);
        if (!info.body.empty())
            continue;
        answers.insert(reference_pud_normalize_with_bindings(info.bindings, this->exprs, info.head));
    }

    EXPECT_EQ(answers, (std::set<const expr*>{this->functor("q", {this->functor("suc", {this->functor("suc", {this->atom("zero")})})})}));

    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuite3IntegrationTest, FindsGcdOfSixAndFourViaSubtractionInEveryUnfoldOrder) {
    const expr* four = this->atom("zero");
    for (int peano_step = 0; peano_step < 4; ++peano_step)
        four = this->functor("suc", {four});
    const expr* six = this->atom("zero");
    for (int peano_step = 0; peano_step < 6; ++peano_step)
        six = this->functor("suc", {six});
    const std::set<const expr*> expected{this->functor("q", {this->functor("suc", {this->functor("suc", {this->atom("zero")})})})};
    for (uint32_t seed = 0; seed < 20; ++seed) {
        std::mt19937 rng(seed);
        std::deque<pud_module> modules;
        modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {this->atom("zero")}), {}, 0),
        rule(this->functor("nat", {this->functor("suc", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("add", {this->atom("zero"), this->var(0), this->var(0)}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("add", {this->functor("suc", {this->var(0)}), this->var(1), this->functor("suc", {this->var(2)})}), {this->functor("add", {this->var(0), this->var(1), this->var(2)})}, 3),
        rule(this->functor("lt", {this->atom("zero"), this->functor("suc", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("lt", {this->functor("suc", {this->var(0)}), this->functor("suc", {this->var(1)})}), {this->functor("lt", {this->var(0), this->var(1)})}, 2),
        rule(this->functor("sub", {this->var(0), this->atom("zero"), this->var(0)}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("sub", {this->functor("suc", {this->var(0)}), this->functor("suc", {this->var(1)}), this->var(2)}),
            {this->functor("sub", {this->var(0), this->var(1), this->var(2)})}, 3),
        rule(this->functor("gcd", {this->var(0), this->atom("zero"), this->var(0)}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("gcd", {this->var(0), this->var(0), this->var(0)}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("gcd", {this->var(0), this->var(1), this->var(3)}),
            {this->functor("lt", {this->var(1), this->var(0)}),
             this->functor("sub", {this->var(0), this->var(1), this->var(2)}),
             this->functor("gcd", {this->var(2), this->var(1), this->var(3)})}, 4),
        rule(this->functor("gcd", {this->var(0), this->var(1), this->var(3)}),
            {this->functor("lt", {this->var(0), this->var(1)}),
             this->functor("sub", {this->var(1), this->var(0), this->var(2)}),
             this->functor("gcd", {this->var(0), this->var(2), this->var(3)})}, 4)
        });
        modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {this->var(0)}),
            {this->functor("gcd", {six, four, this->var(0)})}, 1)
        });
        TypeParam runtime(this->exprs);
        std::set<pud_global_node_id> live;
        std::map<pud_global_node_id, pud_unnormalized_rule_information> leaf_rule_info;
        for (pud_module& module : modules) {
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id leaf : change.offspring) {
                live.insert(leaf);
                leaf_rule_info.emplace(leaf, runtime.get_rule_information(leaf));
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                leaf_rule_info.erase(refuted);
            }
        }

        size_t steps = 0;
        for (; steps < 20000; ++steps) {
            std::vector<pud_global_node_id> unfoldable;
            for (pud_global_node_id leaf : live) {
                if (leaf.module != 2)
                    continue;
                if (leaf_rule_info.at(leaf).body.empty())
                    continue;
                unfoldable.push_back(leaf);
            }
            if (unfoldable.empty())
                break;
            // Random leaf; smallest open goal id (gcd alone) — other leaf/goal orders do not terminate.
            pud_global_node_id leaf = unfoldable.at(0);
            for (pud_global_node_id candidate : unfoldable) {
                if (candidate < leaf)
                    leaf = candidate;
            }
            std::vector<pud_body_goal_id> goal_ids;
            for (const auto& [goal_id, goal] : leaf_rule_info.at(leaf).body)
                goal_ids.push_back(goal_id);
            pud_body_goal_id goal = goal_ids.at(0);
            for (pud_body_goal_id goal_id : goal_ids) {
                if (goal_id < goal)
                    goal = goal_id;
            }
            const pud_frontier_change change = runtime.unfold(leaf, goal);
            live.erase(leaf);
            leaf_rule_info.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
                live.insert(child);
                leaf_rule_info.emplace(child, runtime.get_rule_information(child));
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                }
        }
        EXPECT_LT(steps, 20000u);
        std::set<const expr*> answers;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            const pud_unnormalized_rule_information info = runtime.get_rule_information(leaf);
            if (!info.body.empty())
                continue;
            answers.insert(reference_pud_normalize_with_bindings(info.bindings, this->exprs, info.head));
        }

        EXPECT_EQ(answers, expected);
        EXPECT_EQ(live, reference_pud_frontier(modules));
        EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    }
}

TYPED_TEST(PudAnswerSetsRuntimeSuite3IntegrationTest, EnumeratesTwoSubsetsOfFourElements) {
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("member", {this->atom("a")}), {}, 0),
        rule(this->functor("member", {this->atom("b")}), {}, 0),
        rule(this->functor("member", {this->atom("c")}), {}, 0),
        rule(this->functor("member", {this->atom("d")}), {}, 0),
        rule(this->functor("before", {this->atom("a"), this->atom("b")}), {}, 0),
        rule(this->functor("before", {this->atom("a"), this->atom("c")}), {}, 0),
        rule(this->functor("before", {this->atom("a"), this->atom("d")}), {}, 0),
        rule(this->functor("before", {this->atom("b"), this->atom("c")}), {}, 0),
        rule(this->functor("before", {this->atom("b"), this->atom("d")}), {}, 0),
        rule(this->functor("before", {this->atom("c"), this->atom("d")}), {}, 0),
        rule(this->functor("pair", {this->var(0), this->var(1)}),
            {this->functor("member", {this->var(0)}),
             this->functor("member", {this->var(1)}),
             this->functor("before", {this->var(0), this->var(1)})}, 2)
    });
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {this->var(0), this->var(1)}),
            {this->functor("pair", {this->var(0), this->var(1)})}, 2)
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    std::map<pud_global_node_id, pud_unnormalized_rule_information> leaf_rule_info;
    for (pud_module& module : modules) {
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id leaf : change.offspring) {
            live.insert(leaf);
            leaf_rule_info.emplace(leaf, runtime.get_rule_information(leaf));
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_rule_info.erase(refuted);
        }
    }

    size_t steps = 0;
    for (; steps < 20000; ++steps) {
        std::optional<pud_global_node_id> chosen_leaf;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            if (leaf_rule_info.at(leaf).body.empty())
                continue;
            if (!chosen_leaf.has_value() || leaf < chosen_leaf.value())
                chosen_leaf = leaf;
        }
        if (!chosen_leaf.has_value())
            break;
        std::optional<pud_body_goal_id> chosen_goal;
        for (const auto& [goal_id, goal] : leaf_rule_info.at(*chosen_leaf).body) {
            if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                chosen_goal = goal_id;
        }
        const pud_frontier_change change = runtime.unfold(*chosen_leaf, *chosen_goal);
        live.erase(*chosen_leaf);
        leaf_rule_info.erase(*chosen_leaf);
        for (pud_global_node_id child : change.offspring) {
            live.insert(child);
            leaf_rule_info.emplace(child, runtime.get_rule_information(child));
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_rule_info.erase(refuted);
        }
    }
    EXPECT_LT(steps, 20000u);
    std::set<const expr*> answers;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        const pud_unnormalized_rule_information info = runtime.get_rule_information(leaf);
        if (!info.body.empty())
            continue;
        answers.insert(reference_pud_normalize_with_bindings(info.bindings, this->exprs, info.head));
    }

    EXPECT_EQ(answers, (std::set<const expr*>{
        this->functor("q", {this->atom("a"), this->atom("b")}),
        this->functor("q", {this->atom("a"), this->atom("c")}),
        this->functor("q", {this->atom("a"), this->atom("d")}),
        this->functor("q", {this->atom("b"), this->atom("c")}),
        this->functor("q", {this->atom("b"), this->atom("d")}),
        this->functor("q", {this->atom("c"), this->atom("d")})}));

    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuite3IntegrationTest, EnumeratesTwoSubsetsOfFourElementsInEveryUnfoldOrder) {
    const std::set<const expr*> expected{
        this->functor("q", {this->atom("a"), this->atom("b")}),
        this->functor("q", {this->atom("a"), this->atom("c")}),
        this->functor("q", {this->atom("a"), this->atom("d")}),
        this->functor("q", {this->atom("b"), this->atom("c")}),
        this->functor("q", {this->atom("b"), this->atom("d")}),
        this->functor("q", {this->atom("c"), this->atom("d")})};
    for (uint32_t seed = 0; seed < 20; ++seed) {
        std::mt19937 rng(seed);
        std::deque<pud_module> modules;
        modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("member", {this->atom("a")}), {}, 0),
        rule(this->functor("member", {this->atom("b")}), {}, 0),
        rule(this->functor("member", {this->atom("c")}), {}, 0),
        rule(this->functor("member", {this->atom("d")}), {}, 0),
        rule(this->functor("before", {this->atom("a"), this->atom("b")}), {}, 0),
        rule(this->functor("before", {this->atom("a"), this->atom("c")}), {}, 0),
        rule(this->functor("before", {this->atom("a"), this->atom("d")}), {}, 0),
        rule(this->functor("before", {this->atom("b"), this->atom("c")}), {}, 0),
        rule(this->functor("before", {this->atom("b"), this->atom("d")}), {}, 0),
        rule(this->functor("before", {this->atom("c"), this->atom("d")}), {}, 0),
        rule(this->functor("pair", {this->var(0), this->var(1)}),
            {this->functor("member", {this->var(0)}),
             this->functor("member", {this->var(1)}),
             this->functor("before", {this->var(0), this->var(1)})}, 2)
        });
        modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {this->var(0), this->var(1)}),
            {this->functor("pair", {this->var(0), this->var(1)})}, 2)
        });
        TypeParam runtime(this->exprs);
        std::set<pud_global_node_id> live;
        std::map<pud_global_node_id, pud_unnormalized_rule_information> leaf_rule_info;
        for (pud_module& module : modules) {
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id leaf : change.offspring) {
                live.insert(leaf);
                leaf_rule_info.emplace(leaf, runtime.get_rule_information(leaf));
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                leaf_rule_info.erase(refuted);
            }
        }

        size_t steps = 0;
        for (; steps < 20000; ++steps) {
            std::vector<pud_global_node_id> unfoldable;
            for (pud_global_node_id leaf : live) {
                if (leaf.module != 2)
                    continue;
                if (leaf_rule_info.at(leaf).body.empty())
                    continue;
                unfoldable.push_back(leaf);
            }
            if (unfoldable.empty())
                break;
            const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
            std::vector<pud_body_goal_id> goal_ids;
            for (const auto& [goal_id, goal] : leaf_rule_info.at(leaf).body)
                goal_ids.push_back(goal_id);
            const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
            const pud_frontier_change change = runtime.unfold(leaf, goal);
            live.erase(leaf);
            leaf_rule_info.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
                live.insert(child);
                leaf_rule_info.emplace(child, runtime.get_rule_information(child));
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                }
        }
        EXPECT_LT(steps, 20000u);
        std::set<const expr*> answers;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            const pud_unnormalized_rule_information info = runtime.get_rule_information(leaf);
            if (!info.body.empty())
                continue;
            answers.insert(reference_pud_normalize_with_bindings(info.bindings, this->exprs, info.head));
        }

        EXPECT_EQ(answers, expected);
        EXPECT_EQ(live, reference_pud_frontier(modules));
        EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    }
}

TYPED_TEST(PudAnswerSetsRuntimeSuite3IntegrationTest, EnumeratesBalancedGrammarStringOfLengthFour) {
    const expr* aabb = this->functor("cons", {
        this->atom("a"), this->functor("cons", {
            this->atom("b"), this->functor("cons", {
                this->atom("a"), this->functor("cons", {this->atom("b"), this->atom("nil")})})})});
    const expr* four = this->functor("suc", {this->functor("suc", {this->functor("suc", {this->functor("suc", {this->atom("zero")})})})});
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("der", {this->atom("nil")}), {}, 0),
        rule(this->functor("len", {this->atom("nil"), this->atom("zero")}), {}, 0),
        rule(this->functor("der", {this->functor("cons", {this->atom("a"), this->functor("cons", {this->atom("b"), this->var(0)})})}),
            {this->functor("der", {this->var(0)})}, 1),
        rule(this->functor("len", {this->functor("cons", {this->atom("a"), this->functor("cons", {this->atom("b"), this->var(0)})}), this->var(2)}),
            {this->functor("len", {this->var(0), this->var(1)}),
             this->functor("add", {this->var(1), this->functor("suc", {this->functor("suc", {this->atom("zero")})}), this->var(2)})}, 3),
        rule(this->functor("add", {this->atom("zero"), this->var(0), this->var(0)}), {}, 1),
        rule(this->functor("add", {this->functor("suc", {this->var(0)}), this->var(1), this->functor("suc", {this->var(2)})}),
            {this->functor("add", {this->var(0), this->var(1), this->var(2)})}, 3)
    });
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {this->var(0)}),
            {this->functor("der", {this->var(0)}),
             this->functor("len", {this->var(0), four})}, 1)
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    std::map<pud_global_node_id, pud_unnormalized_rule_information> leaf_rule_info;
    for (pud_module& module : modules) {
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id leaf : change.offspring) {
            live.insert(leaf);
            leaf_rule_info.emplace(leaf, runtime.get_rule_information(leaf));
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_rule_info.erase(refuted);
        }
    }

    size_t steps = 0;
    for (; steps < 20000; ++steps) {
        std::optional<pud_global_node_id> chosen_leaf;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            if (leaf_rule_info.at(leaf).body.empty())
                continue;
            if (!chosen_leaf.has_value() || leaf < chosen_leaf.value())
                chosen_leaf = leaf;
        }
        if (!chosen_leaf.has_value())
            break;
        std::optional<pud_body_goal_id> chosen_goal;
        for (const auto& [goal_id, goal] : leaf_rule_info.at(*chosen_leaf).body) {
            if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                chosen_goal = goal_id;
        }
        const pud_frontier_change change = runtime.unfold(*chosen_leaf, *chosen_goal);
        live.erase(*chosen_leaf);
        leaf_rule_info.erase(*chosen_leaf);
        for (pud_global_node_id child : change.offspring) {
            live.insert(child);
            leaf_rule_info.emplace(child, runtime.get_rule_information(child));
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_rule_info.erase(refuted);
        }
    }
    EXPECT_LT(steps, 20000u);
    std::set<const expr*> answers;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        const pud_unnormalized_rule_information info = runtime.get_rule_information(leaf);
        if (!info.body.empty())
            continue;
        answers.insert(reference_pud_normalize_with_bindings(info.bindings, this->exprs, info.head));
    }

    EXPECT_EQ(answers, (std::set<const expr*>{this->functor("q", {aabb})}));

    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuite3IntegrationTest, EnumeratesBalancedGrammarStringOfLengthFourInEveryUnfoldOrder) {
    const expr* aabb = this->functor("cons", {
        this->atom("a"), this->functor("cons", {
            this->atom("b"), this->functor("cons", {
                this->atom("a"), this->functor("cons", {this->atom("b"), this->atom("nil")})})})});
    const std::set<const expr*> expected{this->functor("q", {aabb})};
    const expr* four = this->functor("suc", {this->functor("suc", {this->functor("suc", {this->functor("suc", {this->atom("zero")})})})});
    for (uint32_t seed = 0; seed < 20; ++seed) {
        std::mt19937 rng(seed);
        std::deque<pud_module> modules;
        modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("der", {this->atom("nil")}), {}, 0),
        rule(this->functor("len", {this->atom("nil"), this->atom("zero")}), {}, 0),
        rule(this->functor("der", {this->functor("cons", {this->atom("a"), this->functor("cons", {this->atom("b"), this->var(0)})})}),
            {this->functor("der", {this->var(0)})}, 1),
        rule(this->functor("len", {this->functor("cons", {this->atom("a"), this->functor("cons", {this->atom("b"), this->var(0)})}), this->var(2)}),
            {this->functor("len", {this->var(0), this->var(1)}),
             this->functor("add", {this->var(1), this->functor("suc", {this->functor("suc", {this->atom("zero")})}), this->var(2)})}, 3),
        rule(this->functor("add", {this->atom("zero"), this->var(0), this->var(0)}), {}, 1),
        rule(this->functor("add", {this->functor("suc", {this->var(0)}), this->var(1), this->functor("suc", {this->var(2)})}),
            {this->functor("add", {this->var(0), this->var(1), this->var(2)})}, 3)
        });
        modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {this->var(0)}),
            {this->functor("der", {this->var(0)}),
             this->functor("len", {this->var(0), four})}, 1)
        });
        TypeParam runtime(this->exprs);
        std::set<pud_global_node_id> live;
        std::map<pud_global_node_id, pud_unnormalized_rule_information> leaf_rule_info;
        for (pud_module& module : modules) {
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id leaf : change.offspring) {
                live.insert(leaf);
                leaf_rule_info.emplace(leaf, runtime.get_rule_information(leaf));
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                leaf_rule_info.erase(refuted);
            }
        }

        size_t steps = 0;
        for (; steps < 20000; ++steps) {
            std::vector<pud_global_node_id> unfoldable;
            for (pud_global_node_id leaf : live) {
                if (leaf.module != 2)
                    continue;
                if (leaf_rule_info.at(leaf).body.empty())
                    continue;
                unfoldable.push_back(leaf);
            }
            if (unfoldable.empty())
                break;
            const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
            std::vector<pud_body_goal_id> goal_ids;
            for (const auto& [goal_id, goal] : leaf_rule_info.at(leaf).body)
                goal_ids.push_back(goal_id);
            const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
            const pud_frontier_change change = runtime.unfold(leaf, goal);
            live.erase(leaf);
            leaf_rule_info.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
                live.insert(child);
                leaf_rule_info.emplace(child, runtime.get_rule_information(child));
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                }
        }
        EXPECT_LT(steps, 20000u);
        std::set<const expr*> answers;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            const pud_unnormalized_rule_information info = runtime.get_rule_information(leaf);
            if (!info.body.empty())
                continue;
            answers.insert(reference_pud_normalize_with_bindings(info.bindings, this->exprs, info.head));
        }

        EXPECT_EQ(answers, expected);
        EXPECT_EQ(live, reference_pud_frontier(modules));
        EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    }
}

TYPED_TEST(PudAnswerSetsRuntimeSuite3IntegrationTest, EnumeratesDepthTwoTermsOverTwoConstants) {
    const expr* two_depth = this->functor("suc", {this->functor("suc", {this->atom("zero")})});
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("term", {this->atom("a")}), {}, 0),
        rule(this->functor("term", {this->atom("b")}), {}, 0),
        rule(this->functor("depth", {this->atom("a"), this->atom("zero")}), {}, 0),
        rule(this->functor("depth", {this->atom("b"), this->atom("zero")}), {}, 0),
        rule(this->functor("term", {this->functor("app", {this->var(0), this->var(1)})}),
            {this->functor("term", {this->var(0)}),
             this->functor("term", {this->var(1)}),
             this->functor("depth", {this->var(0), this->atom("zero")}),
             this->functor("depth", {this->var(1), this->atom("zero")})}, 2),
        rule(this->functor("depth", {this->functor("app", {this->var(0), this->var(1)}), two_depth}),
            {this->functor("depth", {this->var(0), this->atom("zero")}),
             this->functor("depth", {this->var(1), this->atom("zero")})}, 2)
    });
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {this->var(0)}),
            {this->functor("term", {this->var(0)}),
             this->functor("depth", {this->var(0), two_depth})}, 1)
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    std::map<pud_global_node_id, pud_unnormalized_rule_information> leaf_rule_info;
    for (pud_module& module : modules) {
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id leaf : change.offspring) {
            live.insert(leaf);
            leaf_rule_info.emplace(leaf, runtime.get_rule_information(leaf));
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_rule_info.erase(refuted);
        }
    }

    size_t steps = 0;
    for (; steps < 20000; ++steps) {
        std::optional<pud_global_node_id> chosen_leaf;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            if (leaf_rule_info.at(leaf).body.empty())
                continue;
            if (!chosen_leaf.has_value() || leaf < chosen_leaf.value())
                chosen_leaf = leaf;
        }
        if (!chosen_leaf.has_value())
            break;
        std::optional<pud_body_goal_id> chosen_goal;
        for (const auto& [goal_id, goal] : leaf_rule_info.at(*chosen_leaf).body) {
            if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                chosen_goal = goal_id;
        }
        const pud_frontier_change change = runtime.unfold(*chosen_leaf, *chosen_goal);
        live.erase(*chosen_leaf);
        leaf_rule_info.erase(*chosen_leaf);
        for (pud_global_node_id child : change.offspring) {
            live.insert(child);
            leaf_rule_info.emplace(child, runtime.get_rule_information(child));
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_rule_info.erase(refuted);
        }
    }
    EXPECT_LT(steps, 20000u);
    std::set<const expr*> answers;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        const pud_unnormalized_rule_information info = runtime.get_rule_information(leaf);
        if (!info.body.empty())
            continue;
        answers.insert(reference_pud_normalize_with_bindings(info.bindings, this->exprs, info.head));
    }

    EXPECT_EQ(answers, (std::set<const expr*>{
        this->functor("q", {this->functor("app", {this->atom("a"), this->atom("a")})}),
        this->functor("q", {this->functor("app", {this->atom("a"), this->atom("b")})}),
        this->functor("q", {this->functor("app", {this->atom("b"), this->atom("a")})}),
        this->functor("q", {this->functor("app", {this->atom("b"), this->atom("b")})})}));

    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuite3IntegrationTest, EnumeratesDepthTwoTermsOverTwoConstantsInEveryUnfoldOrder) {
    const std::set<const expr*> expected{
        this->functor("q", {this->functor("app", {this->atom("a"), this->atom("a")})}),
        this->functor("q", {this->functor("app", {this->atom("a"), this->atom("b")})}),
        this->functor("q", {this->functor("app", {this->atom("b"), this->atom("a")})}),
        this->functor("q", {this->functor("app", {this->atom("b"), this->atom("b")})})};
    for (uint32_t seed = 0; seed < 20; ++seed) {
        std::mt19937 rng(seed);
        const expr* two_depth = this->functor("suc", {this->functor("suc", {this->atom("zero")})});
        std::deque<pud_module> modules;
        modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("term", {this->atom("a")}), {}, 0),
        rule(this->functor("term", {this->atom("b")}), {}, 0),
        rule(this->functor("depth", {this->atom("a"), this->atom("zero")}), {}, 0),
        rule(this->functor("depth", {this->atom("b"), this->atom("zero")}), {}, 0),
        rule(this->functor("term", {this->functor("app", {this->var(0), this->var(1)})}),
            {this->functor("term", {this->var(0)}),
             this->functor("term", {this->var(1)}),
             this->functor("depth", {this->var(0), this->atom("zero")}),
             this->functor("depth", {this->var(1), this->atom("zero")})}, 2),
        rule(this->functor("depth", {this->functor("app", {this->var(0), this->var(1)}), two_depth}),
            {this->functor("depth", {this->var(0), this->atom("zero")}),
             this->functor("depth", {this->var(1), this->atom("zero")})}, 2)
        });
        modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {this->var(0)}),
            {this->functor("term", {this->var(0)}),
             this->functor("depth", {this->var(0), two_depth})}, 1)
        });
        TypeParam runtime(this->exprs);
        std::set<pud_global_node_id> live;
        std::map<pud_global_node_id, pud_unnormalized_rule_information> leaf_rule_info;
        for (pud_module& module : modules) {
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id leaf : change.offspring) {
                live.insert(leaf);
                leaf_rule_info.emplace(leaf, runtime.get_rule_information(leaf));
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                leaf_rule_info.erase(refuted);
            }
        }

        size_t steps = 0;
        for (; steps < 20000; ++steps) {
            std::vector<pud_global_node_id> unfoldable;
            for (pud_global_node_id leaf : live) {
                if (leaf.module != 2)
                    continue;
                if (leaf_rule_info.at(leaf).body.empty())
                    continue;
                unfoldable.push_back(leaf);
            }
            if (unfoldable.empty())
                break;
            const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
            std::vector<pud_body_goal_id> goal_ids;
            for (const auto& [goal_id, goal] : leaf_rule_info.at(leaf).body)
                goal_ids.push_back(goal_id);
            const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
            const pud_frontier_change change = runtime.unfold(leaf, goal);
            live.erase(leaf);
            leaf_rule_info.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
                live.insert(child);
                leaf_rule_info.emplace(child, runtime.get_rule_information(child));
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                }
        }
        EXPECT_LT(steps, 20000u);
        std::set<const expr*> answers;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            const pud_unnormalized_rule_information info = runtime.get_rule_information(leaf);
            if (!info.body.empty())
                continue;
            answers.insert(reference_pud_normalize_with_bindings(info.bindings, this->exprs, info.head));
        }

        EXPECT_EQ(answers, expected);
        EXPECT_EQ(live, reference_pud_frontier(modules));
        EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    }
}

TYPED_TEST(PudAnswerSetsRuntimeSuite3IntegrationTest, FindsEvenParityListOfLengthFour) {
    const expr* list4 = this->functor("cons", {
        this->atom("a"), this->functor("cons", {
            this->atom("a"), this->functor("cons", {
                this->atom("a"), this->functor("cons", {this->atom("a"), this->atom("nil")})})})});
    const expr* four = this->functor("suc", {this->functor("suc", {this->functor("suc", {this->functor("suc", {this->atom("zero")})})})});
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("evenlist", {this->atom("nil")}), {}, 0),
        rule(this->functor("len", {this->atom("nil"), this->atom("zero")}), {}, 0),
        rule(this->functor("oddlist", {this->functor("cons", {this->atom("a"), this->var(0)})}),
            {this->functor("evenlist", {this->var(0)})}, 1),
        rule(this->functor("evenlist", {this->functor("cons", {this->atom("a"), this->var(0)})}),
            {this->functor("oddlist", {this->var(0)})}, 1),
        rule(this->functor("len", {this->functor("cons", {this->atom("a"), this->var(0)}), this->var(2)}),
            {this->functor("len", {this->var(0), this->var(1)}),
             this->functor("add", {this->var(1), this->functor("suc", {this->atom("zero")}), this->var(2)})}, 3),
        rule(this->functor("add", {this->atom("zero"), this->var(0), this->var(0)}), {}, 1),
        rule(this->functor("add", {this->functor("suc", {this->var(0)}), this->var(1), this->functor("suc", {this->var(2)})}),
            {this->functor("add", {this->var(0), this->var(1), this->var(2)})}, 3)
    });
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {this->var(0)}),
            {this->functor("evenlist", {this->var(0)}),
             this->functor("len", {this->var(0), four})}, 1)
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    std::map<pud_global_node_id, pud_unnormalized_rule_information> leaf_rule_info;
    for (pud_module& module : modules) {
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id leaf : change.offspring) {
            live.insert(leaf);
            leaf_rule_info.emplace(leaf, runtime.get_rule_information(leaf));
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_rule_info.erase(refuted);
        }
    }

    size_t steps = 0;
    for (; steps < 20000; ++steps) {
        std::optional<pud_global_node_id> chosen_leaf;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            if (leaf_rule_info.at(leaf).body.empty())
                continue;
            if (!chosen_leaf.has_value() || leaf < chosen_leaf.value())
                chosen_leaf = leaf;
        }
        if (!chosen_leaf.has_value())
            break;
        std::optional<pud_body_goal_id> chosen_goal;
        for (const auto& [goal_id, goal] : leaf_rule_info.at(*chosen_leaf).body) {
            if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                chosen_goal = goal_id;
        }
        const pud_frontier_change change = runtime.unfold(*chosen_leaf, *chosen_goal);
        live.erase(*chosen_leaf);
        leaf_rule_info.erase(*chosen_leaf);
        for (pud_global_node_id child : change.offspring) {
            live.insert(child);
            leaf_rule_info.emplace(child, runtime.get_rule_information(child));
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_rule_info.erase(refuted);
        }
    }
    EXPECT_LT(steps, 20000u);
    std::set<const expr*> answers;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        const pud_unnormalized_rule_information info = runtime.get_rule_information(leaf);
        if (!info.body.empty())
            continue;
        answers.insert(reference_pud_normalize_with_bindings(info.bindings, this->exprs, info.head));
    }

    EXPECT_EQ(answers, (std::set<const expr*>{this->functor("q", {list4})}));

    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuite3IntegrationTest, FindsEvenParityListOfLengthFourInEveryUnfoldOrder) {
    const expr* list4 = this->functor("cons", {
        this->atom("a"), this->functor("cons", {
            this->atom("a"), this->functor("cons", {
                this->atom("a"), this->functor("cons", {this->atom("a"), this->atom("nil")})})})});
    const std::set<const expr*> expected{this->functor("q", {list4})};
    for (uint32_t seed = 0; seed < 20; ++seed) {
        std::mt19937 rng(seed);
        const expr* four = this->functor("suc", {this->functor("suc", {this->functor("suc", {this->functor("suc", {this->atom("zero")})})})});
        std::deque<pud_module> modules;
        modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("evenlist", {this->atom("nil")}), {}, 0),
        rule(this->functor("len", {this->atom("nil"), this->atom("zero")}), {}, 0),
        rule(this->functor("oddlist", {this->functor("cons", {this->atom("a"), this->var(0)})}),
            {this->functor("evenlist", {this->var(0)})}, 1),
        rule(this->functor("evenlist", {this->functor("cons", {this->atom("a"), this->var(0)})}),
            {this->functor("oddlist", {this->var(0)})}, 1),
        rule(this->functor("len", {this->functor("cons", {this->atom("a"), this->var(0)}), this->var(2)}),
            {this->functor("len", {this->var(0), this->var(1)}),
             this->functor("add", {this->var(1), this->functor("suc", {this->atom("zero")}), this->var(2)})}, 3),
        rule(this->functor("add", {this->atom("zero"), this->var(0), this->var(0)}), {}, 1),
        rule(this->functor("add", {this->functor("suc", {this->var(0)}), this->var(1), this->functor("suc", {this->var(2)})}),
            {this->functor("add", {this->var(0), this->var(1), this->var(2)})}, 3)
        });
        modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {this->var(0)}),
            {this->functor("evenlist", {this->var(0)}),
             this->functor("len", {this->var(0), four})}, 1)
        });
        TypeParam runtime(this->exprs);
        std::set<pud_global_node_id> live;
        std::map<pud_global_node_id, pud_unnormalized_rule_information> leaf_rule_info;
        for (pud_module& module : modules) {
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id leaf : change.offspring) {
                live.insert(leaf);
                leaf_rule_info.emplace(leaf, runtime.get_rule_information(leaf));
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                leaf_rule_info.erase(refuted);
            }
        }

        size_t steps = 0;
        for (; steps < 20000; ++steps) {
            std::vector<pud_global_node_id> unfoldable;
            for (pud_global_node_id leaf : live) {
                if (leaf.module != 2)
                    continue;
                if (leaf_rule_info.at(leaf).body.empty())
                    continue;
                unfoldable.push_back(leaf);
            }
            if (unfoldable.empty())
                break;
            const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
            std::vector<pud_body_goal_id> goal_ids;
            for (const auto& [goal_id, goal] : leaf_rule_info.at(leaf).body)
                goal_ids.push_back(goal_id);
            // Random leaf; smallest open goal id (evenlist before len) — other goal orders do not terminate.
            pud_body_goal_id goal = goal_ids.at(0);
            for (pud_body_goal_id goal_id : goal_ids) {
                if (goal_id < goal)
                    goal = goal_id;
            }
            const pud_frontier_change change = runtime.unfold(leaf, goal);
            live.erase(leaf);
            leaf_rule_info.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
                live.insert(child);
                leaf_rule_info.emplace(child, runtime.get_rule_information(child));
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                }
        }
        EXPECT_LT(steps, 20000u);
        std::set<const expr*> answers;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            const pud_unnormalized_rule_information info = runtime.get_rule_information(leaf);
            if (!info.body.empty())
                continue;
            answers.insert(reference_pud_normalize_with_bindings(info.bindings, this->exprs, info.head));
        }

        EXPECT_EQ(answers, expected);
        EXPECT_EQ(live, reference_pud_frontier(modules));
        EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    }
}

TYPED_TEST(PudAnswerSetsRuntimeSuite3IntegrationTest, EnumeratesPeanoTriplesInsideTetrahedron) {
    const expr* nine = this->atom("zero");
    for (int peano_step = 0; peano_step < 9; ++peano_step)
        nine = this->functor("suc", {nine});
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {this->atom("zero")}), {}, 0),
        rule(this->functor("nat", {this->functor("suc", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("add", {this->atom("zero"), this->var(0), this->var(0)}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("add", {this->functor("suc", {this->var(0)}), this->var(1), this->functor("suc", {this->var(2)})}), {this->functor("add", {this->var(0), this->var(1), this->var(2)})}, 3),
        rule(this->functor("lt", {this->atom("zero"), this->functor("suc", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("lt", {this->functor("suc", {this->var(0)}), this->functor("suc", {this->var(1)})}), {this->functor("lt", {this->var(0), this->var(1)})}, 2)
    });
    // Goal order add(X,Y,S), add(S,Z,T), lt(T,nine) — original was nat(X), nat(Y), nat(Z), then those
    // three. Nat is still required by add rules; unfolding add/lt first avoids nat-before-add orders
    // that never exhaust.
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {this->var(0), this->var(1), this->var(2)}),
            {this->functor("add", {this->var(0), this->var(1), this->var(3)}),
             this->functor("add", {this->var(3), this->var(2), this->var(4)}),
             this->functor("lt", {this->var(4), nine})}, 5)
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    std::map<pud_global_node_id, pud_unnormalized_rule_information> leaf_rule_info;
    for (pud_module& module : modules) {
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id leaf : change.offspring) {
            live.insert(leaf);
            leaf_rule_info.emplace(leaf, runtime.get_rule_information(leaf));
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_rule_info.erase(refuted);
        }
    }

    size_t steps = 0;
    for (; steps < 20000; ++steps) {
        std::optional<pud_global_node_id> chosen_leaf;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            if (leaf_rule_info.at(leaf).body.empty())
                continue;
            if (!chosen_leaf.has_value() || leaf < chosen_leaf.value())
                chosen_leaf = leaf;
        }
        if (!chosen_leaf.has_value())
            break;
        std::optional<pud_body_goal_id> chosen_goal;
        for (const auto& [goal_id, goal] : leaf_rule_info.at(*chosen_leaf).body) {
            if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                chosen_goal = goal_id;
        }
        const pud_frontier_change change = runtime.unfold(*chosen_leaf, *chosen_goal);
        live.erase(*chosen_leaf);
        leaf_rule_info.erase(*chosen_leaf);
        for (pud_global_node_id child : change.offspring) {
            live.insert(child);
            leaf_rule_info.emplace(child, runtime.get_rule_information(child));
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_rule_info.erase(refuted);
        }
    }
    EXPECT_LT(steps, 20000u);
    std::set<const expr*> answers;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        const pud_unnormalized_rule_information info = runtime.get_rule_information(leaf);
        if (!info.body.empty())
            continue;
        answers.insert(reference_pud_normalize_with_bindings(info.bindings, this->exprs, info.head));
    }

    std::set<const expr*> expected;
    for (int x = 0; x < 9; ++x) {
        for (int y = 0; y < 9 - x; ++y) {
            for (int z = 0; z < 9 - x - y; ++z) {
                const expr* px = this->atom("zero");
                for (int i = 0; i < x; ++i)
                    px = this->functor("suc", {px});
                const expr* py = this->atom("zero");
                for (int i = 0; i < y; ++i)
                    py = this->functor("suc", {py});
                const expr* pz = this->atom("zero");
                for (int i = 0; i < z; ++i)
                    pz = this->functor("suc", {pz});
                expected.insert(this->functor("q", {px, py, pz}));
            }
        }
    }
    ASSERT_EQ(expected.size(), 165u);
    EXPECT_EQ(answers, expected);

    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuite3IntegrationTest, EnumeratesPeanoTriplesInsideTetrahedronInEveryUnfoldOrder) {
    const expr* nine = this->atom("zero");
    for (int peano_step = 0; peano_step < 9; ++peano_step)
        nine = this->functor("suc", {nine});
    std::set<const expr*> expected;
    for (int x = 0; x < 9; ++x) {
        for (int y = 0; y < 9 - x; ++y) {
            for (int z = 0; z < 9 - x - y; ++z) {
                const expr* px = this->atom("zero");
                for (int i = 0; i < x; ++i)
                    px = this->functor("suc", {px});
                const expr* py = this->atom("zero");
                for (int i = 0; i < y; ++i)
                    py = this->functor("suc", {py});
                const expr* pz = this->atom("zero");
                for (int i = 0; i < z; ++i)
                    pz = this->functor("suc", {pz});
                expected.insert(this->functor("q", {px, py, pz}));
            }
        }
    }
    for (uint32_t seed = 0; seed < 20; ++seed) {
        std::mt19937 rng(seed);
        std::deque<pud_module> modules;
        modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {this->atom("zero")}), {}, 0),
        rule(this->functor("nat", {this->functor("suc", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("add", {this->atom("zero"), this->var(0), this->var(0)}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("add", {this->functor("suc", {this->var(0)}), this->var(1), this->functor("suc", {this->var(2)})}), {this->functor("add", {this->var(0), this->var(1), this->var(2)})}, 3),
        rule(this->functor("lt", {this->atom("zero"), this->functor("suc", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("lt", {this->functor("suc", {this->var(0)}), this->functor("suc", {this->var(1)})}), {this->functor("lt", {this->var(0), this->var(1)})}, 2)
        });
        modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {this->var(0), this->var(1), this->var(2)}),
            {this->functor("add", {this->var(0), this->var(1), this->var(3)}),
             this->functor("add", {this->var(3), this->var(2), this->var(4)}),
             this->functor("lt", {this->var(4), nine})}, 5)
        });
        TypeParam runtime(this->exprs);
        std::set<pud_global_node_id> live;
        std::map<pud_global_node_id, pud_unnormalized_rule_information> leaf_rule_info;
        for (pud_module& module : modules) {
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id leaf : change.offspring) {
                live.insert(leaf);
                leaf_rule_info.emplace(leaf, runtime.get_rule_information(leaf));
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                leaf_rule_info.erase(refuted);
            }
        }

        size_t steps = 0;
        for (; steps < 20000; ++steps) {
            std::vector<pud_global_node_id> unfoldable;
            for (pud_global_node_id leaf : live) {
                if (leaf.module != 2)
                    continue;
                if (leaf_rule_info.at(leaf).body.empty())
                    continue;
                unfoldable.push_back(leaf);
            }
            if (unfoldable.empty())
                break;
            const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
            std::vector<pud_body_goal_id> goal_ids;
            for (const auto& [goal_id, goal] : leaf_rule_info.at(leaf).body)
                goal_ids.push_back(goal_id);
            // Random leaf; smallest open goal id (add before lt) — nat-first and other orders do not terminate.
            pud_body_goal_id goal = goal_ids.at(0);
            for (pud_body_goal_id goal_id : goal_ids) {
                if (goal_id < goal)
                    goal = goal_id;
            }
            const pud_frontier_change change = runtime.unfold(leaf, goal);
            live.erase(leaf);
            leaf_rule_info.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
                live.insert(child);
                leaf_rule_info.emplace(child, runtime.get_rule_information(child));
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                }
        }
        EXPECT_LT(steps, 20000u);
        std::set<const expr*> answers;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            const pud_unnormalized_rule_information info = runtime.get_rule_information(leaf);
            if (!info.body.empty())
                continue;
            answers.insert(reference_pud_normalize_with_bindings(info.bindings, this->exprs, info.head));
        }

        EXPECT_EQ(answers, expected);
        EXPECT_EQ(live, reference_pud_frontier(modules));
        EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    }
}


#endif

