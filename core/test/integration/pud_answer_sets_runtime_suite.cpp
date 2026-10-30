#include <cstdint>
#include <cstdio>
#include <deque>
#include <map>
#include <optional>
#include <random>
#include <set>
#include <stack>
#include <variant>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "functor_fixture.hpp"
#include "reference_pud_normalize_with_bindings.hpp"
#include "reference_pud_frontier.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/pud_module.hpp"
#include "infrastructure/pud_runtime.hpp"

using ::testing::IsEmpty;

template<typename IRuntime>
struct PudAnswerSetsRuntimeSuiteIntegrationTest : public ::testing::Test {
    test_functors functors;
    expr_pool exprs;

    const expr* var(uint32_t index) { return exprs.make_var(index); }
    const expr* atom(const char* name) { return exprs.make_functor(functors.id(name), {}); }
    const expr* functor(const char* name, std::vector<const expr*> args) {
        return exprs.make_functor(functors.id(name), std::move(args));
    }
};
using pud_answer_sets_runtime_suite_runtimes_t = ::testing::Types<pud_runtime<pud_module, expr_pool>>;
TYPED_TEST_SUITE(PudAnswerSetsRuntimeSuiteIntegrationTest, pud_answer_sets_runtime_suite_runtimes_t);


TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, FindsSingleUnitSolution) {
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("f", {}), {}, 0)
    });
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {}), {this->functor("f", {})}, 0)
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



    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        EXPECT_TRUE(runtime.get_rule_information(leaf).body.empty());
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

    EXPECT_EQ(answers, (std::set<const expr*>{this->functor("q", {})}));
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, FindsSingleUnitSolutionInEveryUnfoldOrder) {
    const std::set<const expr*> expected{this->functor("q", {})};
    for (uint32_t seed = 0; seed < 20; ++seed) {
        std::mt19937 rng(seed);
        std::deque<pud_module> modules;
        modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("f", {}), {}, 0)
    });
        modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {}), {this->functor("f", {})}, 0)
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



        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            EXPECT_TRUE(runtime.get_rule_information(leaf).body.empty());
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
    }
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, RefutesWhenNoCandidates) {
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{});
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {}), {this->functor("f", {})}, 0)
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



    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        EXPECT_TRUE(runtime.get_rule_information(leaf).body.empty());
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

    EXPECT_EQ(answers, (std::set<const expr*>{}));
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, RefutesWhenNoCandidatesInEveryUnfoldOrder) {
    const std::set<const expr*> expected{};
    for (uint32_t seed = 0; seed < 20; ++seed) {
        std::mt19937 rng(seed);
        std::deque<pud_module> modules;
        modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{});
        modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {}), {this->functor("f", {})}, 0)
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



        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            EXPECT_TRUE(runtime.get_rule_information(leaf).body.empty());
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
    }
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, FindsClauseDerivedUnitSolution) {
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("f", {}), {this->functor("g", {})}, 0),
        rule(this->functor("g", {}), {}, 0)
    });
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {}), {this->functor("f", {})}, 0)
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



    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        EXPECT_TRUE(runtime.get_rule_information(leaf).body.empty());
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

    EXPECT_EQ(answers, (std::set<const expr*>{this->functor("q", {})}));
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, FindsClauseDerivedUnitSolutionInEveryUnfoldOrder) {
    const std::set<const expr*> expected{this->functor("q", {})};
    for (uint32_t seed = 0; seed < 20; ++seed) {
        std::mt19937 rng(seed);
        std::deque<pud_module> modules;
        modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("f", {}), {this->functor("g", {})}, 0),
        rule(this->functor("g", {}), {}, 0)
    });
        modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {}), {this->functor("f", {})}, 0)
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



        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            EXPECT_TRUE(runtime.get_rule_information(leaf).body.empty());
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
    }
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, FindsSolutionWithCorrectBindings) {
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("f", {this->atom("abc"), this->atom("123")}), {}, 0)
    });
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {this->var(0), this->var(1)}), {this->functor("f", {this->var(0), this->var(1)})}, 2)
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



    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        EXPECT_TRUE(runtime.get_rule_information(leaf).body.empty());
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

    EXPECT_EQ(answers, (std::set<const expr*>{this->functor("q", {this->atom("abc"), this->atom("123")})}));
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, FindsSolutionWithCorrectBindingsInEveryUnfoldOrder) {
    const std::set<const expr*> expected{this->functor("q", {this->atom("abc"), this->atom("123")})};
    for (uint32_t seed = 0; seed < 20; ++seed) {
        std::mt19937 rng(seed);
        std::deque<pud_module> modules;
        modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("f", {this->atom("abc"), this->atom("123")}), {}, 0)
    });
        modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {this->var(0), this->var(1)}), {this->functor("f", {this->var(0), this->var(1)})}, 2)
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



        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            EXPECT_TRUE(runtime.get_rule_information(leaf).body.empty());
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
    }
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, FindsClauseBodyBindingSolution) {
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("f", {this->var(0), this->var(1)}), {this->functor("g", {this->var(0)}), this->functor("h", {this->var(1)})}, 2),
        rule(this->functor("g", {this->atom("abc")}), {}, 0),
        rule(this->functor("h", {this->atom("123")}), {}, 0)
    });
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {this->var(0), this->var(1)}), {this->functor("f", {this->var(0), this->var(1)})}, 2)
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



    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        EXPECT_TRUE(runtime.get_rule_information(leaf).body.empty());
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

    EXPECT_EQ(answers, (std::set<const expr*>{this->functor("q", {this->atom("abc"), this->atom("123")})}));
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, FindsClauseBodyBindingSolutionInEveryUnfoldOrder) {
    const std::set<const expr*> expected{this->functor("q", {this->atom("abc"), this->atom("123")})};
    for (uint32_t seed = 0; seed < 20; ++seed) {
        std::mt19937 rng(seed);
        std::deque<pud_module> modules;
        modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("f", {this->var(0), this->var(1)}), {this->functor("g", {this->var(0)}), this->functor("h", {this->var(1)})}, 2),
        rule(this->functor("g", {this->atom("abc")}), {}, 0),
        rule(this->functor("h", {this->atom("123")}), {}, 0)
    });
        modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {this->var(0), this->var(1)}), {this->functor("f", {this->var(0), this->var(1)})}, 2)
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



        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            EXPECT_TRUE(runtime.get_rule_information(leaf).body.empty());
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
    }
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, RefutesAfterCdclOnUnsatClauseBranches) {
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("a", {}), {this->functor("b", {})}, 0),
        rule(this->functor("a", {}), {this->functor("c", {})}, 0)
    });
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {}), {this->functor("a", {})}, 0)
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    size_t refuted_total = 0;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id leaf : change.offspring) {
            live.insert(leaf);
        }
        refuted_total += change.refuted.size();
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
        refuted_total += change.refuted.size();
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
        }
    }
    EXPECT_LT(steps, 2000u);



    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        EXPECT_TRUE(runtime.get_rule_information(leaf).body.empty());
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

    EXPECT_EQ(answers, (std::set<const expr*>{}));
    EXPECT_GE(refuted_total, 2u);
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, RefutesAfterCdclOnUnsatClauseBranchesInEveryUnfoldOrder) {
    const std::set<const expr*> expected{};
    for (uint32_t seed = 0; seed < 20; ++seed) {
        std::mt19937 rng(seed);
        std::deque<pud_module> modules;
        modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("a", {}), {this->functor("b", {})}, 0),
        rule(this->functor("a", {}), {this->functor("c", {})}, 0)
    });
        modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {}), {this->functor("a", {})}, 0)
    });
        TypeParam runtime(this->exprs);
        std::set<pud_global_node_id> live;
            size_t refuted_total = 0;
        for (pud_module& module : modules) {
            install_shared_functor_names(module, this->functors);
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id leaf : change.offspring) {
                live.insert(leaf);
                }
            refuted_total += change.refuted.size();
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
            refuted_total += change.refuted.size();
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                }
        }
        EXPECT_LT(steps, 2000u);



        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            EXPECT_TRUE(runtime.get_rule_information(leaf).body.empty());
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
        EXPECT_GE(refuted_total, 2u);
    }
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesTwoGroundChoiceSolutions) {
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("f", {}), {}, 0),
        rule(this->functor("f", {}), {}, 0)
    });
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {}), {this->functor("f", {})}, 0)
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



    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        EXPECT_TRUE(runtime.get_rule_information(leaf).body.empty());
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

    EXPECT_EQ(answers, (std::set<const expr*>{this->functor("q", {})}));
    size_t solved_live_leaves = 0;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        if (!runtime.get_rule_information(leaf).body.empty())
            continue;
        ++solved_live_leaves;
    }
    EXPECT_EQ(solved_live_leaves, 2u);
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesTwoGroundChoiceSolutionsInEveryUnfoldOrder) {
    const std::set<const expr*> expected{this->functor("q", {})};
    for (uint32_t seed = 0; seed < 20; ++seed) {
        std::mt19937 rng(seed);
        std::deque<pud_module> modules;
        modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("f", {}), {}, 0),
        rule(this->functor("f", {}), {}, 0)
    });
        modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {}), {this->functor("f", {})}, 0)
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



        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            EXPECT_TRUE(runtime.get_rule_information(leaf).body.empty());
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

        size_t solved_live_leaves = 0;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            if (!runtime.get_rule_information(leaf).body.empty())
                continue;
            ++solved_live_leaves;
        }
        EXPECT_EQ(solved_live_leaves, 2u);
        EXPECT_EQ(live, reference_pud_frontier(modules));
    }
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, RefutesAfterEnumeratingAllGroundBranches) {
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("f", {}), {}, 0),
        rule(this->functor("f", {}), {}, 0)
    });
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {}), {this->functor("f", {})}, 0)
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



    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        EXPECT_TRUE(runtime.get_rule_information(leaf).body.empty());
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

    EXPECT_EQ(answers, (std::set<const expr*>{this->functor("q", {})}));
    size_t solved_live_leaves = 0;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        if (!runtime.get_rule_information(leaf).body.empty())
            continue;
        ++solved_live_leaves;
    }
    EXPECT_EQ(solved_live_leaves, 2u);
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, RefutesAfterEnumeratingAllGroundBranchesInEveryUnfoldOrder) {
    const std::set<const expr*> expected{this->functor("q", {})};
    for (uint32_t seed = 0; seed < 20; ++seed) {
        std::mt19937 rng(seed);
        std::deque<pud_module> modules;
        modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("f", {}), {}, 0),
        rule(this->functor("f", {}), {}, 0)
    });
        modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {}), {this->functor("f", {})}, 0)
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



        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            EXPECT_TRUE(runtime.get_rule_information(leaf).body.empty());
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

        size_t solved_live_leaves = 0;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            if (!runtime.get_rule_information(leaf).body.empty())
                continue;
            ++solved_live_leaves;
        }
        EXPECT_EQ(solved_live_leaves, 2u);
        EXPECT_EQ(live, reference_pud_frontier(modules));
    }
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesTwoVarChoiceSolutions) {
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("f", {this->atom("abc")}), {}, 0),
        rule(this->functor("f", {this->atom("xyz")}), {}, 0)
    });
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {this->var(0)}), {this->functor("f", {this->var(0)})}, 1)
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



    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        EXPECT_TRUE(runtime.get_rule_information(leaf).body.empty());
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

    EXPECT_EQ(answers, (std::set<const expr*>{this->functor("q", {this->atom("abc")}), this->functor("q", {this->atom("xyz")})}));
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesTwoVarChoiceSolutionsInEveryUnfoldOrder) {
    const std::set<const expr*> expected{this->functor("q", {this->atom("abc")}), this->functor("q", {this->atom("xyz")})};
    for (uint32_t seed = 0; seed < 20; ++seed) {
        std::mt19937 rng(seed);
        std::deque<pud_module> modules;
        modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("f", {this->atom("abc")}), {}, 0),
        rule(this->functor("f", {this->atom("xyz")}), {}, 0)
    });
        modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {this->var(0)}), {this->functor("f", {this->var(0)})}, 1)
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



        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            EXPECT_TRUE(runtime.get_rule_information(leaf).body.empty());
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
    }
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, RefutesAfterEnumeratingAllVarBranches) {
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("f", {this->atom("abc")}), {}, 0),
        rule(this->functor("f", {this->atom("xyz")}), {}, 0)
    });
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {this->var(0)}), {this->functor("f", {this->var(0)})}, 1)
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



    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        EXPECT_TRUE(runtime.get_rule_information(leaf).body.empty());
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

    EXPECT_EQ(answers, (std::set<const expr*>{this->functor("q", {this->atom("abc")}), this->functor("q", {this->atom("xyz")})}));
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, RefutesAfterEnumeratingAllVarBranchesInEveryUnfoldOrder) {
    const std::set<const expr*> expected{this->functor("q", {this->atom("abc")}), this->functor("q", {this->atom("xyz")})};
    for (uint32_t seed = 0; seed < 20; ++seed) {
        std::mt19937 rng(seed);
        std::deque<pud_module> modules;
        modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("f", {this->atom("abc")}), {}, 0),
        rule(this->functor("f", {this->atom("xyz")}), {}, 0)
    });
        modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {this->var(0)}), {this->functor("f", {this->var(0)})}, 1)
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



        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            EXPECT_TRUE(runtime.get_rule_information(leaf).body.empty());
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
    }
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesTwoGoalSharedVarSolutions) {
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("f", {this->atom("abc")}), {}, 0),
        rule(this->functor("f", {this->atom("xyz")}), {}, 0),
        rule(this->functor("g", {this->atom("abc")}), {}, 0),
        rule(this->functor("g", {this->atom("xyz")}), {}, 0)
    });
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {this->var(0)}), {this->functor("f", {this->var(0)}), this->functor("g", {this->var(0)})}, 1)
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



    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        EXPECT_TRUE(runtime.get_rule_information(leaf).body.empty());
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

    EXPECT_EQ(answers, (std::set<const expr*>{this->functor("q", {this->atom("abc")}), this->functor("q", {this->atom("xyz")})}));
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesTwoGoalSharedVarSolutionsInEveryUnfoldOrder) {
    const std::set<const expr*> expected{this->functor("q", {this->atom("abc")}), this->functor("q", {this->atom("xyz")})};
    for (uint32_t seed = 0; seed < 20; ++seed) {
        std::mt19937 rng(seed);
        std::deque<pud_module> modules;
        modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("f", {this->atom("abc")}), {}, 0),
        rule(this->functor("f", {this->atom("xyz")}), {}, 0),
        rule(this->functor("g", {this->atom("abc")}), {}, 0),
        rule(this->functor("g", {this->atom("xyz")}), {}, 0)
    });
        modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {this->var(0)}), {this->functor("f", {this->var(0)}), this->functor("g", {this->var(0)})}, 1)
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



        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            EXPECT_TRUE(runtime.get_rule_information(leaf).body.empty());
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
    }
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesFourVarBindingSolutions) {
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("f", {this->atom("a")}), {}, 0),
        rule(this->functor("f", {this->atom("b")}), {}, 0),
        rule(this->functor("f", {this->atom("c")}), {}, 0),
        rule(this->functor("f", {this->atom("d")}), {}, 0)
    });
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {this->var(0)}), {this->functor("f", {this->var(0)})}, 1)
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



    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        EXPECT_TRUE(runtime.get_rule_information(leaf).body.empty());
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

    EXPECT_EQ(answers, (std::set<const expr*>{this->functor("q", {this->atom("a")}), this->functor("q", {this->atom("b")}), this->functor("q", {this->atom("c")}), this->functor("q", {this->atom("d")})}));
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesFourVarBindingSolutionsInEveryUnfoldOrder) {
    const std::set<const expr*> expected{this->functor("q", {this->atom("a")}), this->functor("q", {this->atom("b")}), this->functor("q", {this->atom("c")}), this->functor("q", {this->atom("d")})};
    for (uint32_t seed = 0; seed < 20; ++seed) {
        std::mt19937 rng(seed);
        std::deque<pud_module> modules;
        modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("f", {this->atom("a")}), {}, 0),
        rule(this->functor("f", {this->atom("b")}), {}, 0),
        rule(this->functor("f", {this->atom("c")}), {}, 0),
        rule(this->functor("f", {this->atom("d")}), {}, 0)
    });
        modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {this->var(0)}), {this->functor("f", {this->var(0)})}, 1)
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



        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            EXPECT_TRUE(runtime.get_rule_information(leaf).body.empty());
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
    }
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesFourClauseBodyFactChoices) {
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("f", {}), {this->functor("g", {this->var(0)})}, 1),
        rule(this->functor("g", {this->atom("a")}), {}, 0),
        rule(this->functor("g", {this->atom("b")}), {}, 0),
        rule(this->functor("g", {this->atom("c")}), {}, 0),
        rule(this->functor("g", {this->atom("d")}), {}, 0)
    });
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {}), {this->functor("f", {})}, 0)
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



    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        EXPECT_TRUE(runtime.get_rule_information(leaf).body.empty());
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

    EXPECT_EQ(answers, (std::set<const expr*>{this->functor("q", {})}));
    size_t solved_live_leaves = 0;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        if (!runtime.get_rule_information(leaf).body.empty())
            continue;
        ++solved_live_leaves;
    }
    EXPECT_EQ(solved_live_leaves, 4u);
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesFourClauseBodyFactChoicesInEveryUnfoldOrder) {
    const std::set<const expr*> expected{this->functor("q", {})};
    for (uint32_t seed = 0; seed < 20; ++seed) {
        std::mt19937 rng(seed);
        std::deque<pud_module> modules;
        modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("f", {}), {this->functor("g", {this->var(0)})}, 1),
        rule(this->functor("g", {this->atom("a")}), {}, 0),
        rule(this->functor("g", {this->atom("b")}), {}, 0),
        rule(this->functor("g", {this->atom("c")}), {}, 0),
        rule(this->functor("g", {this->atom("d")}), {}, 0)
    });
        modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {}), {this->functor("f", {})}, 0)
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



        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            EXPECT_TRUE(runtime.get_rule_information(leaf).body.empty());
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

        size_t solved_live_leaves = 0;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            if (!runtime.get_rule_information(leaf).body.empty())
                continue;
            ++solved_live_leaves;
        }
        EXPECT_EQ(solved_live_leaves, 4u);
        EXPECT_EQ(live, reference_pud_frontier(modules));
    }
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, FindsUniqueSharedVarConjunctionThenRefutes) {
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("is_a", {this->atom("1")}), {}, 0),
        rule(this->functor("is_a", {this->atom("2")}), {}, 0),
        rule(this->functor("is_b", {this->atom("2")}), {}, 0),
        rule(this->functor("is_b", {this->atom("3")}), {}, 0)
    });
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {this->var(0)}), {this->functor("is_a", {this->var(0)}), this->functor("is_b", {this->var(0)})}, 1)
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



    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        EXPECT_TRUE(runtime.get_rule_information(leaf).body.empty());
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

    EXPECT_EQ(answers, (std::set<const expr*>{this->functor("q", {this->atom("2")})}));
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, FindsUniqueSharedVarConjunctionThenRefutesInEveryUnfoldOrder) {
    const std::set<const expr*> expected{this->functor("q", {this->atom("2")})};
    for (uint32_t seed = 0; seed < 20; ++seed) {
        std::mt19937 rng(seed);
        std::deque<pud_module> modules;
        modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("is_a", {this->atom("1")}), {}, 0),
        rule(this->functor("is_a", {this->atom("2")}), {}, 0),
        rule(this->functor("is_b", {this->atom("2")}), {}, 0),
        rule(this->functor("is_b", {this->atom("3")}), {}, 0)
    });
        modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {this->var(0)}), {this->functor("is_a", {this->var(0)}), this->functor("is_b", {this->var(0)})}, 1)
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



        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            EXPECT_TRUE(runtime.get_rule_information(leaf).body.empty());
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
    }
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, ConflictedTickNotSolved) {
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{});
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {}), {this->functor("f", {})}, 0)
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



    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        EXPECT_TRUE(runtime.get_rule_information(leaf).body.empty());
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

    EXPECT_EQ(answers, (std::set<const expr*>{}));
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, ConflictedTickNotSolvedInEveryUnfoldOrder) {
    const std::set<const expr*> expected{};
    for (uint32_t seed = 0; seed < 20; ++seed) {
        std::mt19937 rng(seed);
        std::deque<pud_module> modules;
        modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{});
        modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {}), {this->functor("f", {})}, 0)
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



        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            EXPECT_TRUE(runtime.get_rule_information(leaf).body.empty());
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
    }
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesTwoParentBindingsForAlice) {
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("parent", {this->atom("bob"), this->atom("alice")}), {}, 0),
        rule(this->functor("parent", {this->atom("carol"), this->atom("alice")}), {}, 0),
        rule(this->functor("parent", {this->atom("dave"), this->atom("bob")}), {}, 0)
    });
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {this->var(0)}), {this->functor("parent", {this->var(0), this->atom("alice")})}, 1)
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



    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        EXPECT_TRUE(runtime.get_rule_information(leaf).body.empty());
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

    EXPECT_EQ(answers, (std::set<const expr*>{this->functor("q", {this->atom("bob")}), this->functor("q", {this->atom("carol")})}));
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesTwoParentBindingsForAliceInEveryUnfoldOrder) {
    const std::set<const expr*> expected{this->functor("q", {this->atom("bob")}), this->functor("q", {this->atom("carol")})};
    for (uint32_t seed = 0; seed < 20; ++seed) {
        std::mt19937 rng(seed);
        std::deque<pud_module> modules;
        modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("parent", {this->atom("bob"), this->atom("alice")}), {}, 0),
        rule(this->functor("parent", {this->atom("carol"), this->atom("alice")}), {}, 0),
        rule(this->functor("parent", {this->atom("dave"), this->atom("bob")}), {}, 0)
    });
        modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {this->var(0)}), {this->functor("parent", {this->var(0), this->atom("alice")})}, 1)
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



        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            EXPECT_TRUE(runtime.get_rule_information(leaf).body.empty());
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
    }
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesPeanoLessThanSeven) {
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {this->atom("zero")}), {}, 0),
        rule(this->functor("nat", {this->functor("suc", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("lt", {this->atom("zero"), this->functor("suc", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("lt", {this->functor("suc", {this->var(0)}), this->functor("suc", {this->var(1)})}), {this->functor("lt", {this->var(0), this->var(1)})}, 2)
    });
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {this->var(0)}),
            {this->functor("nat", {this->var(0)}),
             this->functor("lt", {this->var(0), this->functor("suc", {this->functor("suc", {this->functor("suc", {this->functor("suc", {this->functor("suc", {this->functor("suc", {this->functor("suc", {this->atom("zero")})})})})})})})})}, 1)
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
    for (; steps < 8000; ++steps) {
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
    EXPECT_LT(steps, 8000u);



    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        EXPECT_TRUE(runtime.get_rule_information(leaf).body.empty());
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


    std::set<const expr*> expected;
    for (int n = 0; n < 7; ++n) {
        const expr* peano = this->atom("zero");
        for (int i = 0; i < n; ++i)
            peano = this->functor("suc", {peano});
        expected.insert(this->functor("q", {peano}));
    }

    EXPECT_EQ(answers, expected);
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesPeanoLessThanSevenInEveryUnfoldOrder) {

    std::set<const expr*> expected;
    for (int n = 0; n < 7; ++n) {
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
        rule(this->functor("nat", {this->functor("suc", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("lt", {this->atom("zero"), this->functor("suc", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("lt", {this->functor("suc", {this->var(0)}), this->functor("suc", {this->var(1)})}), {this->functor("lt", {this->var(0), this->var(1)})}, 2)
        });
        modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
            rule(this->functor("q", {this->var(0)}),
            {this->functor("nat", {this->var(0)}),
             this->functor("lt", {this->var(0), this->functor("suc", {this->functor("suc", {this->functor("suc", {this->functor("suc", {this->functor("suc", {this->functor("suc", {this->functor("suc", {this->atom("zero")})})})})})})})})}, 1)
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
        for (; steps < 8000; ++steps) {
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
        EXPECT_LT(steps, 8000u);



        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            EXPECT_TRUE(runtime.get_rule_information(leaf).body.empty());
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
    }
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesSatPAndQOrR) {
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("bool", {this->atom("true")}), {}, 0),
        rule(this->functor("bool", {this->atom("false")}), {}, 0),
        rule(this->functor("or", {this->atom("true"), this->var(0), this->atom("true")}), {this->functor("bool", {this->var(0)})}, 1),
        rule(this->functor("or", {this->atom("false"), this->var(0), this->var(0)}), {this->functor("bool", {this->var(0)})}, 1),
        rule(this->functor("and", {this->atom("true"), this->var(0), this->var(0)}), {this->functor("bool", {this->var(0)})}, 1),
        rule(this->functor("and", {this->atom("false"), this->var(0), this->atom("false")}), {this->functor("bool", {this->var(0)})}, 1)
    });
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {this->var(0), this->var(1), this->var(2)}),
            {this->functor("bool", {this->var(0)}), this->functor("bool", {this->var(1)}), this->functor("bool", {this->var(2)}),
             this->functor("or", {this->var(1), this->var(2), this->var(3)}),
             this->functor("and", {this->var(0), this->var(3), this->atom("true")})}, 4)
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



    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        EXPECT_TRUE(runtime.get_rule_information(leaf).body.empty());
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

    EXPECT_EQ(answers, (std::set<const expr*>{this->functor("q", {this->atom("true"), this->atom("true"), this->atom("true")}),
        this->functor("q", {this->atom("true"), this->atom("true"), this->atom("false")}),
        this->functor("q", {this->atom("true"), this->atom("false"), this->atom("true")})}));
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesSatPAndQOrRInEveryUnfoldOrder) {
    const std::set<const expr*> expected{this->functor("q", {this->atom("true"), this->atom("true"), this->atom("true")}),
        this->functor("q", {this->atom("true"), this->atom("true"), this->atom("false")}),
        this->functor("q", {this->atom("true"), this->atom("false"), this->atom("true")})};
    for (uint32_t seed = 0; seed < 20; ++seed) {
        std::mt19937 rng(seed);
        std::deque<pud_module> modules;
        modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("bool", {this->atom("true")}), {}, 0),
        rule(this->functor("bool", {this->atom("false")}), {}, 0),
        rule(this->functor("or", {this->atom("true"), this->var(0), this->atom("true")}), {this->functor("bool", {this->var(0)})}, 1),
        rule(this->functor("or", {this->atom("false"), this->var(0), this->var(0)}), {this->functor("bool", {this->var(0)})}, 1),
        rule(this->functor("and", {this->atom("true"), this->var(0), this->var(0)}), {this->functor("bool", {this->var(0)})}, 1),
        rule(this->functor("and", {this->atom("false"), this->var(0), this->atom("false")}), {this->functor("bool", {this->var(0)})}, 1)
    });
        modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {this->var(0), this->var(1), this->var(2)}),
            {this->functor("bool", {this->var(0)}), this->functor("bool", {this->var(1)}), this->functor("bool", {this->var(2)}),
             this->functor("or", {this->var(1), this->var(2), this->var(3)}),
             this->functor("and", {this->var(0), this->var(3), this->atom("true")})}, 4)
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



        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            EXPECT_TRUE(runtime.get_rule_information(leaf).body.empty());
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
    }
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesTwoSatAssignmentsForImpliesQ) {
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("bool", {this->atom("true")}), {}, 0),
        rule(this->functor("bool", {this->atom("false")}), {}, 0),
        rule(this->functor("not", {this->atom("true"), this->atom("false")}), {}, 0),
        rule(this->functor("not", {this->atom("false"), this->atom("true")}), {}, 0),
        rule(this->functor("or", {this->atom("true"), this->atom("true"), this->atom("true")}), {}, 0),
        rule(this->functor("or", {this->atom("true"), this->atom("false"), this->atom("true")}), {}, 0),
        rule(this->functor("or", {this->atom("false"), this->atom("true"), this->atom("true")}), {}, 0),
        rule(this->functor("or", {this->atom("false"), this->atom("false"), this->atom("false")}), {}, 0)
    });
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {this->var(0), this->var(1)}),
            {this->functor("bool", {this->var(0)}), this->functor("bool", {this->var(1)}),
             this->functor("or", {this->var(0), this->var(1), this->atom("true")}),
             this->functor("not", {this->var(0), this->var(2)}),
             this->functor("or", {this->var(2), this->var(1), this->atom("true")})}, 3)
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



    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        EXPECT_TRUE(runtime.get_rule_information(leaf).body.empty());
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

    EXPECT_EQ(answers, (std::set<const expr*>{this->functor("q", {this->atom("true"), this->atom("true")}), this->functor("q", {this->atom("false"), this->atom("true")})}));
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesTwoSatAssignmentsForImpliesQInEveryUnfoldOrder) {
    const std::set<const expr*> expected{this->functor("q", {this->atom("true"), this->atom("true")}), this->functor("q", {this->atom("false"), this->atom("true")})};
    for (uint32_t seed = 0; seed < 20; ++seed) {
        std::mt19937 rng(seed);
        std::deque<pud_module> modules;
        modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("bool", {this->atom("true")}), {}, 0),
        rule(this->functor("bool", {this->atom("false")}), {}, 0),
        rule(this->functor("not", {this->atom("true"), this->atom("false")}), {}, 0),
        rule(this->functor("not", {this->atom("false"), this->atom("true")}), {}, 0),
        rule(this->functor("or", {this->atom("true"), this->atom("true"), this->atom("true")}), {}, 0),
        rule(this->functor("or", {this->atom("true"), this->atom("false"), this->atom("true")}), {}, 0),
        rule(this->functor("or", {this->atom("false"), this->atom("true"), this->atom("true")}), {}, 0),
        rule(this->functor("or", {this->atom("false"), this->atom("false"), this->atom("false")}), {}, 0)
    });
        modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {this->var(0), this->var(1)}),
            {this->functor("bool", {this->var(0)}), this->functor("bool", {this->var(1)}),
             this->functor("or", {this->var(0), this->var(1), this->atom("true")}),
             this->functor("not", {this->var(0), this->var(2)}),
             this->functor("or", {this->var(2), this->var(1), this->atom("true")})}, 3)
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



        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            EXPECT_TRUE(runtime.get_rule_information(leaf).body.empty());
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
    }
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesTwoPathTwoColorings) {
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("color", {this->atom("red")}), {}, 0),
        rule(this->functor("color", {this->atom("blue")}), {}, 0),
        rule(this->functor("diff", {this->atom("red"), this->atom("blue")}), {}, 0),
        rule(this->functor("diff", {this->atom("blue"), this->atom("red")}), {}, 0),
    });
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(
            this->functor("q", {this->var(0), this->var(1), this->var(2)}),
            {
                this->functor("color", {this->var(0)}),
                this->functor("color", {this->var(1)}),
                this->functor("color", {this->var(2)}),
                this->functor("diff", {this->var(0), this->var(1)}),
                this->functor("diff", {this->var(1), this->var(2)}),
            },
            3),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    std::set<pud_global_node_id> unfoldable;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id leaf : change.offspring) {
            live.insert(leaf);
            if (leaf.module == 2 && !runtime.get_rule_information(leaf).body.empty())
                unfoldable.insert(leaf);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            unfoldable.erase(refuted);
        }
    }

    size_t steps = 0;
    for (; steps < 2000; ++steps) {
        if (unfoldable.empty())
            break;
        const pud_global_node_id chosen_leaf = *unfoldable.begin();
        std::optional<pud_body_goal_id> chosen_goal;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(chosen_leaf).body) {
            if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                chosen_goal = goal_id;
        }
        const pud_frontier_change change = runtime.unfold(chosen_leaf, *chosen_goal);
        live.erase(chosen_leaf);
        unfoldable.erase(chosen_leaf);
        for (pud_global_node_id child : change.offspring) {
            live.insert(child);
            if (child.module == 2 && !runtime.get_rule_information(child).body.empty())
                unfoldable.insert(child);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            unfoldable.erase(refuted);
        }
    }
    EXPECT_LT(steps, 2000u);
    std::set<pud_global_node_id> reference_unfoldable;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        if (!runtime.get_rule_information(leaf).body.empty())
            reference_unfoldable.insert(leaf);
    }
    EXPECT_EQ(unfoldable, reference_unfoldable);

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
        this->functor("q", {this->atom("red"), this->atom("blue"), this->atom("red")}),
        this->functor("q", {this->atom("blue"), this->atom("red"), this->atom("blue")}),
    }));
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesTwoPathTwoColoringsInEveryUnfoldOrder) {
    const std::set<const expr*> expected = (std::set<const expr*>{
        this->functor("q", {this->atom("red"), this->atom("blue"), this->atom("red")}),
        this->functor("q", {this->atom("blue"), this->atom("red"), this->atom("blue")}),
    });
    for (uint32_t seed = 0; seed < 20; ++seed) {
        std::mt19937 rng(seed);
            std::deque<pud_module> modules;
            modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
                rule(this->functor("color", {this->atom("red")}), {}, 0),
                rule(this->functor("color", {this->atom("blue")}), {}, 0),
                rule(this->functor("diff", {this->atom("red"), this->atom("blue")}), {}, 0),
                rule(this->functor("diff", {this->atom("blue"), this->atom("red")}), {}, 0),
            });
            modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
                rule(
                    this->functor("q", {this->var(0), this->var(1), this->var(2)}),
                    {
                        this->functor("color", {this->var(0)}),
                        this->functor("color", {this->var(1)}),
                        this->functor("color", {this->var(2)}),
                        this->functor("diff", {this->var(0), this->var(1)}),
                        this->functor("diff", {this->var(1), this->var(2)}),
                    },
                    3),
            });
        TypeParam runtime(this->exprs);
        std::set<pud_global_node_id> live;
        std::set<pud_global_node_id> unfoldable;
        for (pud_module& module : modules) {
            install_shared_functor_names(module, this->functors);
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id leaf : change.offspring) {
                live.insert(leaf);
                    if (leaf.module == 2 && !runtime.get_rule_information(leaf).body.empty())
                    unfoldable.insert(leaf);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                        unfoldable.erase(refuted);
            }
        }

        size_t steps = 0;
        for (; steps < 2000; ++steps) {
            if (unfoldable.empty())
                break;
            const pud_global_node_id leaf = *std::next(unfoldable.begin(), rng() % unfoldable.size());
            std::vector<pud_body_goal_id> goal_ids;
            for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
                goal_ids.push_back(goal_id);
            const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
            const pud_frontier_change change = runtime.unfold(leaf, goal);
            live.erase(leaf);
            unfoldable.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
                live.insert(child);
                if (child.module == 2 && !runtime.get_rule_information(child).body.empty())
                    unfoldable.insert(child);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                        unfoldable.erase(refuted);
            }
        }
        EXPECT_LT(steps, 2000u);
        std::set<pud_global_node_id> reference_unfoldable;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            if (!runtime.get_rule_information(leaf).body.empty())
                reference_unfoldable.insert(leaf);
        }
        EXPECT_EQ(unfoldable, reference_unfoldable);

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
    }
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesK3ThreeColorings) {
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("color", {this->atom("red")}), {}, 0),
        rule(this->functor("color", {this->atom("green")}), {}, 0),
        rule(this->functor("color", {this->atom("blue")}), {}, 0),
        rule(this->functor("diff", {this->atom("red"), this->atom("green")}), {}, 0),
        rule(this->functor("diff", {this->atom("red"), this->atom("blue")}), {}, 0),
        rule(this->functor("diff", {this->atom("green"), this->atom("red")}), {}, 0),
        rule(this->functor("diff", {this->atom("green"), this->atom("blue")}), {}, 0),
        rule(this->functor("diff", {this->atom("blue"), this->atom("red")}), {}, 0),
        rule(this->functor("diff", {this->atom("blue"), this->atom("green")}), {}, 0),
    });
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(
            this->functor("q", {this->var(0), this->var(1), this->var(2)}),
            {
                this->functor("color", {this->var(0)}),
                this->functor("color", {this->var(1)}),
                this->functor("color", {this->var(2)}),
                this->functor("diff", {this->var(0), this->var(1)}),
                this->functor("diff", {this->var(0), this->var(2)}),
                this->functor("diff", {this->var(1), this->var(2)}),
            },
            3),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    std::set<pud_global_node_id> unfoldable;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id leaf : change.offspring) {
            live.insert(leaf);
            if (leaf.module == 2 && !runtime.get_rule_information(leaf).body.empty())
                unfoldable.insert(leaf);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            unfoldable.erase(refuted);
        }
    }

    size_t steps = 0;
    for (; steps < 5000; ++steps) {
        if (unfoldable.empty())
            break;
        const pud_global_node_id chosen_leaf = *unfoldable.begin();
        std::optional<pud_body_goal_id> chosen_goal;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(chosen_leaf).body) {
            if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                chosen_goal = goal_id;
        }
        const pud_frontier_change change = runtime.unfold(chosen_leaf, *chosen_goal);
        live.erase(chosen_leaf);
        unfoldable.erase(chosen_leaf);
        for (pud_global_node_id child : change.offspring) {
            live.insert(child);
            if (child.module == 2 && !runtime.get_rule_information(child).body.empty())
                unfoldable.insert(child);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            unfoldable.erase(refuted);
        }
    }
    EXPECT_LT(steps, 5000u);
    std::set<pud_global_node_id> reference_unfoldable;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        if (!runtime.get_rule_information(leaf).body.empty())
            reference_unfoldable.insert(leaf);
    }
    EXPECT_EQ(unfoldable, reference_unfoldable);

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
        this->functor("q", {this->atom("red"), this->atom("green"), this->atom("blue")}),
        this->functor("q", {this->atom("red"), this->atom("blue"), this->atom("green")}),
        this->functor("q", {this->atom("green"), this->atom("red"), this->atom("blue")}),
        this->functor("q", {this->atom("green"), this->atom("blue"), this->atom("red")}),
        this->functor("q", {this->atom("blue"), this->atom("red"), this->atom("green")}),
        this->functor("q", {this->atom("blue"), this->atom("green"), this->atom("red")}),
    }));
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesK3ThreeColoringsInEveryUnfoldOrder) {
    const std::set<const expr*> expected = (std::set<const expr*>{
        this->functor("q", {this->atom("red"), this->atom("green"), this->atom("blue")}),
        this->functor("q", {this->atom("red"), this->atom("blue"), this->atom("green")}),
        this->functor("q", {this->atom("green"), this->atom("red"), this->atom("blue")}),
        this->functor("q", {this->atom("green"), this->atom("blue"), this->atom("red")}),
        this->functor("q", {this->atom("blue"), this->atom("red"), this->atom("green")}),
        this->functor("q", {this->atom("blue"), this->atom("green"), this->atom("red")}),
    });
    for (uint32_t seed = 0; seed < 20; ++seed) {
        std::mt19937 rng(seed);
            std::deque<pud_module> modules;
            modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
                rule(this->functor("color", {this->atom("red")}), {}, 0),
                rule(this->functor("color", {this->atom("green")}), {}, 0),
                rule(this->functor("color", {this->atom("blue")}), {}, 0),
                rule(this->functor("diff", {this->atom("red"), this->atom("green")}), {}, 0),
                rule(this->functor("diff", {this->atom("red"), this->atom("blue")}), {}, 0),
                rule(this->functor("diff", {this->atom("green"), this->atom("red")}), {}, 0),
                rule(this->functor("diff", {this->atom("green"), this->atom("blue")}), {}, 0),
                rule(this->functor("diff", {this->atom("blue"), this->atom("red")}), {}, 0),
                rule(this->functor("diff", {this->atom("blue"), this->atom("green")}), {}, 0),
            });
            modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
                rule(
                    this->functor("q", {this->var(0), this->var(1), this->var(2)}),
                    {
                        this->functor("color", {this->var(0)}),
                        this->functor("color", {this->var(1)}),
                        this->functor("color", {this->var(2)}),
                        this->functor("diff", {this->var(0), this->var(1)}),
                        this->functor("diff", {this->var(0), this->var(2)}),
                        this->functor("diff", {this->var(1), this->var(2)}),
                    },
                    3),
            });
        TypeParam runtime(this->exprs);
        std::set<pud_global_node_id> live;
        std::set<pud_global_node_id> unfoldable;
        for (pud_module& module : modules) {
            install_shared_functor_names(module, this->functors);
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id leaf : change.offspring) {
                live.insert(leaf);
                    if (leaf.module == 2 && !runtime.get_rule_information(leaf).body.empty())
                    unfoldable.insert(leaf);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                        unfoldable.erase(refuted);
            }
        }

        size_t steps = 0;
        for (; steps < 5000; ++steps) {
            if (unfoldable.empty())
                break;
            const pud_global_node_id leaf = *std::next(unfoldable.begin(), rng() % unfoldable.size());
            std::vector<pud_body_goal_id> goal_ids;
            for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
                goal_ids.push_back(goal_id);
            const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
            const pud_frontier_change change = runtime.unfold(leaf, goal);
            live.erase(leaf);
            unfoldable.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
                live.insert(child);
                if (child.module == 2 && !runtime.get_rule_information(child).body.empty())
                    unfoldable.insert(child);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                        unfoldable.erase(refuted);
            }
        }
        EXPECT_LT(steps, 5000u);
        std::set<pud_global_node_id> reference_unfoldable;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            if (!runtime.get_rule_information(leaf).body.empty())
                reference_unfoldable.insert(leaf);
        }
        EXPECT_EQ(unfoldable, reference_unfoldable);

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
    }
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesK3TailFourNodeColorings) {
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("color", {this->atom("red")}), {}, 0),
        rule(this->functor("color", {this->atom("green")}), {}, 0),
        rule(this->functor("color", {this->atom("blue")}), {}, 0),
        rule(this->functor("diff", {this->atom("red"), this->atom("green")}), {}, 0),
        rule(this->functor("diff", {this->atom("red"), this->atom("blue")}), {}, 0),
        rule(this->functor("diff", {this->atom("green"), this->atom("red")}), {}, 0),
        rule(this->functor("diff", {this->atom("green"), this->atom("blue")}), {}, 0),
        rule(this->functor("diff", {this->atom("blue"), this->atom("red")}), {}, 0),
        rule(this->functor("diff", {this->atom("blue"), this->atom("green")}), {}, 0),
    });
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(
            this->functor("q", {this->var(0), this->var(1), this->var(2), this->var(3)}),
            {
                this->functor("color", {this->var(0)}),
                this->functor("color", {this->var(1)}),
                this->functor("color", {this->var(2)}),
                this->functor("color", {this->var(3)}),
                this->functor("diff", {this->var(0), this->var(1)}),
                this->functor("diff", {this->var(0), this->var(2)}),
                this->functor("diff", {this->var(1), this->var(2)}),
                this->functor("diff", {this->var(0), this->var(3)}),
            },
            4),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    std::set<pud_global_node_id> unfoldable;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id leaf : change.offspring) {
            live.insert(leaf);
            if (leaf.module == 2 && !runtime.get_rule_information(leaf).body.empty())
                unfoldable.insert(leaf);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            unfoldable.erase(refuted);
        }
    }

    size_t steps = 0;
    for (; steps < 8000; ++steps) {
        if (unfoldable.empty())
            break;
        const pud_global_node_id chosen_leaf = *unfoldable.begin();
        std::optional<pud_body_goal_id> chosen_goal;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(chosen_leaf).body) {
            if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                chosen_goal = goal_id;
        }
        const pud_frontier_change change = runtime.unfold(chosen_leaf, *chosen_goal);
        live.erase(chosen_leaf);
        unfoldable.erase(chosen_leaf);
        for (pud_global_node_id child : change.offspring) {
            live.insert(child);
            if (child.module == 2 && !runtime.get_rule_information(child).body.empty())
                unfoldable.insert(child);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            unfoldable.erase(refuted);
        }
    }
    EXPECT_LT(steps, 8000u);
    std::set<pud_global_node_id> reference_unfoldable;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        if (!runtime.get_rule_information(leaf).body.empty())
            reference_unfoldable.insert(leaf);
    }
    EXPECT_EQ(unfoldable, reference_unfoldable);

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
        this->functor("q", {this->atom("red"), this->atom("green"), this->atom("blue"), this->atom("green")}),
        this->functor("q", {this->atom("red"), this->atom("green"), this->atom("blue"), this->atom("blue")}),
        this->functor("q", {this->atom("red"), this->atom("blue"), this->atom("green"), this->atom("green")}),
        this->functor("q", {this->atom("red"), this->atom("blue"), this->atom("green"), this->atom("blue")}),
        this->functor("q", {this->atom("green"), this->atom("red"), this->atom("blue"), this->atom("red")}),
        this->functor("q", {this->atom("green"), this->atom("red"), this->atom("blue"), this->atom("blue")}),
        this->functor("q", {this->atom("green"), this->atom("blue"), this->atom("red"), this->atom("red")}),
        this->functor("q", {this->atom("green"), this->atom("blue"), this->atom("red"), this->atom("blue")}),
        this->functor("q", {this->atom("blue"), this->atom("red"), this->atom("green"), this->atom("red")}),
        this->functor("q", {this->atom("blue"), this->atom("red"), this->atom("green"), this->atom("green")}),
        this->functor("q", {this->atom("blue"), this->atom("green"), this->atom("red"), this->atom("red")}),
        this->functor("q", {this->atom("blue"), this->atom("green"), this->atom("red"), this->atom("green")}),
    }));
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesK3TailFourNodeColoringsInEveryUnfoldOrder) {
    const std::set<const expr*> expected = (std::set<const expr*>{
        this->functor("q", {this->atom("red"), this->atom("green"), this->atom("blue"), this->atom("green")}),
        this->functor("q", {this->atom("red"), this->atom("green"), this->atom("blue"), this->atom("blue")}),
        this->functor("q", {this->atom("red"), this->atom("blue"), this->atom("green"), this->atom("green")}),
        this->functor("q", {this->atom("red"), this->atom("blue"), this->atom("green"), this->atom("blue")}),
        this->functor("q", {this->atom("green"), this->atom("red"), this->atom("blue"), this->atom("red")}),
        this->functor("q", {this->atom("green"), this->atom("red"), this->atom("blue"), this->atom("blue")}),
        this->functor("q", {this->atom("green"), this->atom("blue"), this->atom("red"), this->atom("red")}),
        this->functor("q", {this->atom("green"), this->atom("blue"), this->atom("red"), this->atom("blue")}),
        this->functor("q", {this->atom("blue"), this->atom("red"), this->atom("green"), this->atom("red")}),
        this->functor("q", {this->atom("blue"), this->atom("red"), this->atom("green"), this->atom("green")}),
        this->functor("q", {this->atom("blue"), this->atom("green"), this->atom("red"), this->atom("red")}),
        this->functor("q", {this->atom("blue"), this->atom("green"), this->atom("red"), this->atom("green")}),
    });
    for (uint32_t seed = 0; seed < 20; ++seed) {
        std::mt19937 rng(seed);
            std::deque<pud_module> modules;
            modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
                rule(this->functor("color", {this->atom("red")}), {}, 0),
                rule(this->functor("color", {this->atom("green")}), {}, 0),
                rule(this->functor("color", {this->atom("blue")}), {}, 0),
                rule(this->functor("diff", {this->atom("red"), this->atom("green")}), {}, 0),
                rule(this->functor("diff", {this->atom("red"), this->atom("blue")}), {}, 0),
                rule(this->functor("diff", {this->atom("green"), this->atom("red")}), {}, 0),
                rule(this->functor("diff", {this->atom("green"), this->atom("blue")}), {}, 0),
                rule(this->functor("diff", {this->atom("blue"), this->atom("red")}), {}, 0),
                rule(this->functor("diff", {this->atom("blue"), this->atom("green")}), {}, 0),
            });
            modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
                rule(
                    this->functor("q", {this->var(0), this->var(1), this->var(2), this->var(3)}),
                    {
                        this->functor("color", {this->var(0)}),
                        this->functor("color", {this->var(1)}),
                        this->functor("color", {this->var(2)}),
                        this->functor("color", {this->var(3)}),
                        this->functor("diff", {this->var(0), this->var(1)}),
                        this->functor("diff", {this->var(0), this->var(2)}),
                        this->functor("diff", {this->var(1), this->var(2)}),
                        this->functor("diff", {this->var(0), this->var(3)}),
                    },
                    4),
            });
        TypeParam runtime(this->exprs);
        std::set<pud_global_node_id> live;
        std::set<pud_global_node_id> unfoldable;
        for (pud_module& module : modules) {
            install_shared_functor_names(module, this->functors);
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id leaf : change.offspring) {
                live.insert(leaf);
                    if (leaf.module == 2 && !runtime.get_rule_information(leaf).body.empty())
                    unfoldable.insert(leaf);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                        unfoldable.erase(refuted);
            }
        }

        size_t steps = 0;
        for (; steps < 8000; ++steps) {
            if (unfoldable.empty())
                break;
            const pud_global_node_id leaf = *std::next(unfoldable.begin(), rng() % unfoldable.size());
            std::vector<pud_body_goal_id> goal_ids;
            for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
                goal_ids.push_back(goal_id);
            const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
            const pud_frontier_change change = runtime.unfold(leaf, goal);
            live.erase(leaf);
            unfoldable.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
                live.insert(child);
                if (child.module == 2 && !runtime.get_rule_information(child).body.empty())
                    unfoldable.insert(child);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                        unfoldable.erase(refuted);
            }
        }
        EXPECT_LT(steps, 8000u);
        std::set<pud_global_node_id> reference_unfoldable;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            if (!runtime.get_rule_information(leaf).body.empty())
                reference_unfoldable.insert(leaf);
        }
        EXPECT_EQ(unfoldable, reference_unfoldable);

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
    }
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesFourVarSatThreeClauses) {
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("bool", {this->atom("true")}), {}, 0),
        rule(this->functor("bool", {this->atom("false")}), {}, 0),
        rule(this->functor("not", {this->atom("true"), this->atom("false")}), {}, 0),
        rule(this->functor("not", {this->atom("false"), this->atom("true")}), {}, 0),
        rule(this->functor("or", {this->atom("true"), this->atom("true"), this->atom("true")}), {}, 0),
        rule(this->functor("or", {this->atom("true"), this->atom("false"), this->atom("true")}), {}, 0),
        rule(this->functor("or", {this->atom("false"), this->atom("true"), this->atom("true")}), {}, 0),
        rule(this->functor("or", {this->atom("false"), this->atom("false"), this->atom("false")}), {}, 0),
        rule(this->functor("and", {this->atom("true"), this->atom("true"), this->atom("true")}), {}, 0),
        rule(this->functor("and", {this->atom("true"), this->atom("false"), this->atom("false")}), {}, 0),
        rule(this->functor("and", {this->atom("false"), this->atom("true"), this->atom("false")}), {}, 0),
        rule(this->functor("and", {this->atom("false"), this->atom("false"), this->atom("false")}), {}, 0),
    });
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(
            this->functor("q", {this->var(0), this->var(1), this->var(2), this->var(3)}),
            {
                this->functor("bool", {this->var(0)}),
                this->functor("bool", {this->var(1)}),
                this->functor("bool", {this->var(2)}),
                this->functor("bool", {this->var(3)}),
                this->functor("or", {this->var(0), this->var(1), this->var(4)}),
                this->functor("or", {this->var(2), this->var(3), this->var(5)}),
                this->functor("not", {this->var(0), this->var(6)}),
                this->functor("not", {this->var(2), this->var(7)}),
                this->functor("or", {this->var(6), this->var(7), this->var(8)}),
                this->functor("and", {this->var(4), this->var(5), this->var(9)}),
                this->functor("and", {this->var(9), this->var(8), this->atom("true")}),
            },
            10),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    std::set<pud_global_node_id> unfoldable;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id leaf : change.offspring) {
            live.insert(leaf);
            if (leaf.module == 2 && !runtime.get_rule_information(leaf).body.empty())
                unfoldable.insert(leaf);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            unfoldable.erase(refuted);
        }
    }

    size_t steps = 0;
    for (; steps < 10000; ++steps) {
        if (unfoldable.empty())
            break;
        const pud_global_node_id chosen_leaf = *unfoldable.begin();
        std::optional<pud_body_goal_id> chosen_goal;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(chosen_leaf).body) {
            if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                chosen_goal = goal_id;
        }
        const pud_frontier_change change = runtime.unfold(chosen_leaf, *chosen_goal);
        live.erase(chosen_leaf);
        unfoldable.erase(chosen_leaf);
        for (pud_global_node_id child : change.offspring) {
            live.insert(child);
            if (child.module == 2 && !runtime.get_rule_information(child).body.empty())
                unfoldable.insert(child);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            unfoldable.erase(refuted);
        }
    }
    EXPECT_LT(steps, 10000u);
    std::set<pud_global_node_id> reference_unfoldable;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        if (!runtime.get_rule_information(leaf).body.empty())
            reference_unfoldable.insert(leaf);
    }
    EXPECT_EQ(unfoldable, reference_unfoldable);

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
        this->functor("q", {this->atom("true"), this->atom("true"), this->atom("false"), this->atom("true")}),
        this->functor("q", {this->atom("true"), this->atom("false"), this->atom("false"), this->atom("true")}),
        this->functor("q", {this->atom("false"), this->atom("true"), this->atom("true"), this->atom("true")}),
        this->functor("q", {this->atom("false"), this->atom("true"), this->atom("true"), this->atom("false")}),
        this->functor("q", {this->atom("false"), this->atom("true"), this->atom("false"), this->atom("true")}),
    }));
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesFourVarSatThreeClausesInEveryUnfoldOrder) {
    const std::set<const expr*> expected = (std::set<const expr*>{
        this->functor("q", {this->atom("true"), this->atom("true"), this->atom("false"), this->atom("true")}),
        this->functor("q", {this->atom("true"), this->atom("false"), this->atom("false"), this->atom("true")}),
        this->functor("q", {this->atom("false"), this->atom("true"), this->atom("true"), this->atom("true")}),
        this->functor("q", {this->atom("false"), this->atom("true"), this->atom("true"), this->atom("false")}),
        this->functor("q", {this->atom("false"), this->atom("true"), this->atom("false"), this->atom("true")}),
    });
    for (uint32_t seed = 0; seed < 20; ++seed) {
        std::mt19937 rng(seed);
            std::deque<pud_module> modules;
            modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
                rule(this->functor("bool", {this->atom("true")}), {}, 0),
                rule(this->functor("bool", {this->atom("false")}), {}, 0),
                rule(this->functor("not", {this->atom("true"), this->atom("false")}), {}, 0),
                rule(this->functor("not", {this->atom("false"), this->atom("true")}), {}, 0),
                rule(this->functor("or", {this->atom("true"), this->atom("true"), this->atom("true")}), {}, 0),
                rule(this->functor("or", {this->atom("true"), this->atom("false"), this->atom("true")}), {}, 0),
                rule(this->functor("or", {this->atom("false"), this->atom("true"), this->atom("true")}), {}, 0),
                rule(this->functor("or", {this->atom("false"), this->atom("false"), this->atom("false")}), {}, 0),
                rule(this->functor("and", {this->atom("true"), this->atom("true"), this->atom("true")}), {}, 0),
                rule(this->functor("and", {this->atom("true"), this->atom("false"), this->atom("false")}), {}, 0),
                rule(this->functor("and", {this->atom("false"), this->atom("true"), this->atom("false")}), {}, 0),
                rule(this->functor("and", {this->atom("false"), this->atom("false"), this->atom("false")}), {}, 0),
            });
            modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
                rule(
                    this->functor("q", {this->var(0), this->var(1), this->var(2), this->var(3)}),
                    {
                        this->functor("bool", {this->var(0)}),
                        this->functor("bool", {this->var(1)}),
                        this->functor("bool", {this->var(2)}),
                        this->functor("bool", {this->var(3)}),
                        this->functor("or", {this->var(0), this->var(1), this->var(4)}),
                        this->functor("or", {this->var(2), this->var(3), this->var(5)}),
                        this->functor("not", {this->var(0), this->var(6)}),
                        this->functor("not", {this->var(2), this->var(7)}),
                        this->functor("or", {this->var(6), this->var(7), this->var(8)}),
                        this->functor("and", {this->var(4), this->var(5), this->var(9)}),
                        this->functor("and", {this->var(9), this->var(8), this->atom("true")}),
                    },
                    10),
            });
        TypeParam runtime(this->exprs);
        std::set<pud_global_node_id> live;
        std::set<pud_global_node_id> unfoldable;
        for (pud_module& module : modules) {
            install_shared_functor_names(module, this->functors);
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id leaf : change.offspring) {
                live.insert(leaf);
                    if (leaf.module == 2 && !runtime.get_rule_information(leaf).body.empty())
                    unfoldable.insert(leaf);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                        unfoldable.erase(refuted);
            }
        }

        size_t steps = 0;
        for (; steps < 10000; ++steps) {
            if (unfoldable.empty())
                break;
            const pud_global_node_id leaf = *std::next(unfoldable.begin(), rng() % unfoldable.size());
            std::vector<pud_body_goal_id> goal_ids;
            for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
                goal_ids.push_back(goal_id);
            const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
            const pud_frontier_change change = runtime.unfold(leaf, goal);
            live.erase(leaf);
            unfoldable.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
                live.insert(child);
                if (child.module == 2 && !runtime.get_rule_information(child).body.empty())
                    unfoldable.insert(child);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                        unfoldable.erase(refuted);
            }
        }
        EXPECT_LT(steps, 10000u);
        std::set<pud_global_node_id> reference_unfoldable;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            if (!runtime.get_rule_information(leaf).body.empty())
                reference_unfoldable.insert(leaf);
        }
        EXPECT_EQ(unfoldable, reference_unfoldable);

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
    }
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesAddPairsSummingLessThanTen) {
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {this->atom("zero")}), {}, 0),
        rule(this->functor("nat", {this->functor("suc", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("add", {this->atom("zero"), this->var(0), this->var(0)}), {this->functor("nat", {this->var(0)})}, 1),
        rule(
            this->functor("add", {this->functor("suc", {this->var(0)}), this->var(1), this->functor("suc", {this->var(2)})}),
            {this->functor("add", {this->var(0), this->var(1), this->var(2)})},
            3),
        rule(this->functor("lt", {this->atom("zero"), this->functor("suc", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(
            this->functor("lt", {this->functor("suc", {this->var(0)}), this->functor("suc", {this->var(1)})}),
            {this->functor("lt", {this->var(0), this->var(1)})},
            2),
    });
    const expr* ten = this->atom("zero");
    for (int peano_step = 0; peano_step < 10; ++peano_step)
        ten = this->functor("suc", {ten});
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(
            this->functor("q", {this->var(0), this->var(1)}),
            {this->functor("add", {this->var(0), this->var(1), this->var(2)}), this->functor("lt", {this->var(2), ten})},
            3),
    });
    std::set<const expr*> expected;
    for (int x = 0; x < 10; ++x) {
        const expr* x_term = this->atom("zero");
        for (int peano_step = 0; peano_step < x; ++peano_step)
            x_term = this->functor("suc", {x_term});
        for (int y = 0; y < 10 - x; ++y) {
            const expr* y_term = this->atom("zero");
            for (int peano_step = 0; peano_step < y; ++peano_step)
                y_term = this->functor("suc", {y_term});
            expected.insert(this->functor("q", {x_term, y_term}));
        }
    }
    ASSERT_EQ(expected.size(), 55u);
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    std::set<pud_global_node_id> unfoldable;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id leaf : change.offspring) {
            live.insert(leaf);
            if (leaf.module == 2 && !runtime.get_rule_information(leaf).body.empty())
                unfoldable.insert(leaf);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            unfoldable.erase(refuted);
        }
    }

    size_t steps = 0;
    for (; steps < 15000; ++steps) {
        if (unfoldable.empty())
            break;
        const pud_global_node_id chosen_leaf = *unfoldable.begin();
        std::optional<pud_body_goal_id> chosen_goal;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(chosen_leaf).body) {
            if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                chosen_goal = goal_id;
        }
        const pud_frontier_change change = runtime.unfold(chosen_leaf, *chosen_goal);
        live.erase(chosen_leaf);
        unfoldable.erase(chosen_leaf);
        for (pud_global_node_id child : change.offspring) {
            live.insert(child);
            if (child.module == 2 && !runtime.get_rule_information(child).body.empty())
                unfoldable.insert(child);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            unfoldable.erase(refuted);
        }
    }
    EXPECT_LT(steps, 15000u);
    std::set<pud_global_node_id> reference_unfoldable;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        if (!runtime.get_rule_information(leaf).body.empty())
            reference_unfoldable.insert(leaf);
    }
    EXPECT_EQ(unfoldable, reference_unfoldable);

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

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesAddPairsSummingLessThanTenInEveryUnfoldOrder) {
    for (uint32_t seed = 0; seed < 20; ++seed) {
        std::mt19937 rng(seed);
            std::deque<pud_module> modules;
            modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
                rule(this->functor("nat", {this->atom("zero")}), {}, 0),
                rule(this->functor("nat", {this->functor("suc", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
                rule(this->functor("add", {this->atom("zero"), this->var(0), this->var(0)}), {this->functor("nat", {this->var(0)})}, 1),
                rule(
                    this->functor("add", {this->functor("suc", {this->var(0)}), this->var(1), this->functor("suc", {this->var(2)})}),
                    {this->functor("add", {this->var(0), this->var(1), this->var(2)})},
                    3),
                rule(this->functor("lt", {this->atom("zero"), this->functor("suc", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
                rule(
                    this->functor("lt", {this->functor("suc", {this->var(0)}), this->functor("suc", {this->var(1)})}),
                    {this->functor("lt", {this->var(0), this->var(1)})},
                    2),
            });
            const expr* ten = this->atom("zero");
            for (int peano_step = 0; peano_step < 10; ++peano_step)
                ten = this->functor("suc", {ten});
            modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
                rule(
                    this->functor("q", {this->var(0), this->var(1)}),
                    {this->functor("add", {this->var(0), this->var(1), this->var(2)}), this->functor("lt", {this->var(2), ten})},
                    3),
            });
            std::set<const expr*> expected;
            for (int x = 0; x < 10; ++x) {
                const expr* x_term = this->atom("zero");
                for (int peano_step = 0; peano_step < x; ++peano_step)
                    x_term = this->functor("suc", {x_term});
                for (int y = 0; y < 10 - x; ++y) {
                    const expr* y_term = this->atom("zero");
                    for (int peano_step = 0; peano_step < y; ++peano_step)
                        y_term = this->functor("suc", {y_term});
                    expected.insert(this->functor("q", {x_term, y_term}));
                }
            }
            ASSERT_EQ(expected.size(), 55u);
        TypeParam runtime(this->exprs);
        std::set<pud_global_node_id> live;
        std::set<pud_global_node_id> unfoldable;
        for (pud_module& module : modules) {
            install_shared_functor_names(module, this->functors);
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id leaf : change.offspring) {
                live.insert(leaf);
                    if (leaf.module == 2 && !runtime.get_rule_information(leaf).body.empty())
                    unfoldable.insert(leaf);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                        unfoldable.erase(refuted);
            }
        }

        size_t steps = 0;
        for (; steps < 15000; ++steps) {
            if (unfoldable.empty())
                break;
            const pud_global_node_id leaf = *std::next(unfoldable.begin(), rng() % unfoldable.size());
            std::vector<pud_body_goal_id> goal_ids;
            for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
                goal_ids.push_back(goal_id);
            const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
            const pud_frontier_change change = runtime.unfold(leaf, goal);
            live.erase(leaf);
            unfoldable.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
                live.insert(child);
                if (child.module == 2 && !runtime.get_rule_information(child).body.empty())
                    unfoldable.insert(child);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                        unfoldable.erase(refuted);
            }
        }
        EXPECT_LT(steps, 15000u);
        std::set<pud_global_node_id> reference_unfoldable;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            if (!runtime.get_rule_information(leaf).body.empty())
                reference_unfoldable.insert(leaf);
        }
        EXPECT_EQ(unfoldable, reference_unfoldable);

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
    }
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesAddPairsSummingExactlyTen) {
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {this->atom("zero")}), {}, 0),
        rule(this->functor("nat", {this->functor("suc", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("add", {this->atom("zero"), this->var(0), this->var(0)}), {this->functor("nat", {this->var(0)})}, 1),
        rule(
            this->functor("add", {this->functor("suc", {this->var(0)}), this->var(1), this->functor("suc", {this->var(2)})}),
            {this->functor("add", {this->var(0), this->var(1), this->var(2)})},
            3),
    });
    const expr* ten = this->atom("zero");
    for (int peano_step = 0; peano_step < 10; ++peano_step)
        ten = this->functor("suc", {ten});
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(
            this->functor("q", {this->var(0), this->var(1)}),
            {this->functor("nat", {this->var(0)}), this->functor("nat", {this->var(1)}), this->functor("add", {this->var(0), this->var(1), ten})},
            2),
    });
    std::set<const expr*> expected;
    for (int x = 0; x <= 10; ++x) {
        const expr* x_term = this->atom("zero");
        for (int peano_step = 0; peano_step < x; ++peano_step)
            x_term = this->functor("suc", {x_term});
        const expr* y_term = this->atom("zero");
        for (int peano_step = 0; peano_step < 10 - x; ++peano_step)
            y_term = this->functor("suc", {y_term});
        expected.insert(this->functor("q", {x_term, y_term}));
    }
    ASSERT_EQ(expected.size(), 11u);
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    std::set<pud_global_node_id> unfoldable;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id leaf : change.offspring) {
            live.insert(leaf);
            if (leaf.module == 2 && !runtime.get_rule_information(leaf).body.empty())
                unfoldable.insert(leaf);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            unfoldable.erase(refuted);
        }
    }

    size_t steps = 0;
    for (; steps < 10000; ++steps) {
        if (unfoldable.empty())
            break;
        const pud_global_node_id chosen_leaf = *unfoldable.begin();
        std::optional<pud_body_goal_id> chosen_goal;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(chosen_leaf).body) {
            if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                chosen_goal = goal_id;
        }
        const pud_frontier_change change = runtime.unfold(chosen_leaf, *chosen_goal);
        live.erase(chosen_leaf);
        unfoldable.erase(chosen_leaf);
        for (pud_global_node_id child : change.offspring) {
            live.insert(child);
            if (child.module == 2 && !runtime.get_rule_information(child).body.empty())
                unfoldable.insert(child);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            unfoldable.erase(refuted);
        }
    }
    EXPECT_LT(steps, 10000u);
    std::set<pud_global_node_id> reference_unfoldable;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        if (!runtime.get_rule_information(leaf).body.empty())
            reference_unfoldable.insert(leaf);
    }
    EXPECT_EQ(unfoldable, reference_unfoldable);

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

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesAddPairsSummingExactlyTenInEveryUnfoldOrder) {
    for (uint32_t seed = 0; seed < 20; ++seed) {
        std::mt19937 rng(seed);
            std::deque<pud_module> modules;
            modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
                rule(this->functor("nat", {this->atom("zero")}), {}, 0),
                rule(this->functor("nat", {this->functor("suc", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
                rule(this->functor("add", {this->atom("zero"), this->var(0), this->var(0)}), {this->functor("nat", {this->var(0)})}, 1),
                rule(
                    this->functor("add", {this->functor("suc", {this->var(0)}), this->var(1), this->functor("suc", {this->var(2)})}),
                    {this->functor("add", {this->var(0), this->var(1), this->var(2)})},
                    3),
            });
            const expr* ten = this->atom("zero");
            for (int peano_step = 0; peano_step < 10; ++peano_step)
                ten = this->functor("suc", {ten});
            modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
                rule(
                    this->functor("q", {this->var(0), this->var(1)}),
                    {this->functor("nat", {this->var(0)}), this->functor("nat", {this->var(1)}), this->functor("add", {this->var(0), this->var(1), ten})},
                    2),
            });
            std::set<const expr*> expected;
            for (int x = 0; x <= 10; ++x) {
                const expr* x_term = this->atom("zero");
                for (int peano_step = 0; peano_step < x; ++peano_step)
                    x_term = this->functor("suc", {x_term});
                const expr* y_term = this->atom("zero");
                for (int peano_step = 0; peano_step < 10 - x; ++peano_step)
                    y_term = this->functor("suc", {y_term});
                expected.insert(this->functor("q", {x_term, y_term}));
            }
            ASSERT_EQ(expected.size(), 11u);
        TypeParam runtime(this->exprs);
        std::set<pud_global_node_id> live;
        std::set<pud_global_node_id> unfoldable;
        for (pud_module& module : modules) {
            install_shared_functor_names(module, this->functors);
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id leaf : change.offspring) {
                live.insert(leaf);
                    if (leaf.module == 2 && !runtime.get_rule_information(leaf).body.empty())
                    unfoldable.insert(leaf);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                        unfoldable.erase(refuted);
            }
        }

        size_t steps = 0;
        for (; steps < 10000; ++steps) {
            if (unfoldable.empty())
                break;
            const pud_global_node_id leaf = *std::next(unfoldable.begin(), rng() % unfoldable.size());
            std::vector<pud_body_goal_id> goal_ids;
            for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
                goal_ids.push_back(goal_id);
            const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
            const pud_frontier_change change = runtime.unfold(leaf, goal);
            live.erase(leaf);
            unfoldable.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
                live.insert(child);
                if (child.module == 2 && !runtime.get_rule_information(child).body.empty())
                    unfoldable.insert(child);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                        unfoldable.erase(refuted);
            }
        }
        EXPECT_LT(steps, 10000u);
        std::set<pud_global_node_id> reference_unfoldable;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            if (!runtime.get_rule_information(leaf).body.empty())
                reference_unfoldable.insert(leaf);
        }
        EXPECT_EQ(unfoldable, reference_unfoldable);

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
    }
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesMulPairsProductEight) {
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {this->atom("zero")}), {}, 0),
        rule(this->functor("nat", {this->functor("suc", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("add", {this->atom("zero"), this->var(0), this->var(0)}), {this->functor("nat", {this->var(0)})}, 1),
        rule(
            this->functor("add", {this->functor("suc", {this->var(0)}), this->var(1), this->functor("suc", {this->var(2)})}),
            {this->functor("add", {this->var(0), this->var(1), this->var(2)})},
            3),
        rule(this->functor("mul", {this->atom("zero"), this->var(0), this->atom("zero")}), {this->functor("nat", {this->var(0)})}, 1),
        rule(
            this->functor("mul", {this->functor("suc", {this->var(0)}), this->var(1), this->var(2)}),
            {this->functor("mul", {this->var(0), this->var(1), this->var(3)}), this->functor("add", {this->var(3), this->var(1), this->var(2)})},
            4),
    });
    const expr* eight_product = this->atom("zero");
    for (int peano_step = 0; peano_step < 8; ++peano_step)
        eight_product = this->functor("suc", {eight_product});
    // Query body is mul(X,Y,eight) only (original: nat(X), nat(Y), mul(X,Y,eight)). Nat is still required
    // by mul rules; unfolding mul before nat avoids nat-first goal orders that never exhaust.
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(
            this->functor("q", {this->var(0), this->var(1)}),
            {this->functor("mul", {this->var(0), this->var(1), eight_product})},
            2),
    });
    const expr* one = this->functor("suc", {this->atom("zero")});
    const expr* two = this->functor("suc", {one});
    const expr* four = this->functor("suc", {this->functor("suc", {two})});
    std::set<const expr*> expected{
        this->functor("q", {one, eight_product}),
        this->functor("q", {two, four}),
        this->functor("q", {four, two}),
        this->functor("q", {eight_product, one}),
    };
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    std::set<pud_global_node_id> unfoldable;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id leaf : change.offspring) {
            live.insert(leaf);
            if (leaf.module == 2 && !runtime.get_rule_information(leaf).body.empty())
                unfoldable.insert(leaf);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            unfoldable.erase(refuted);
        }
    }

    // Derivation never exhausts: mul(X,zero,eight) re-synthesizes for every larger X via the Y=zero rule,
    // so unfoldable never empties. Deterministic smallest-leaf drive collects all four factor pairs by
    // step 365 (measured; same answer set at 20k and 40k caps). kMaxSteps=20000 is well above that.
    static constexpr size_t kMaxSteps = 20000;
    size_t steps = 0;
    for (; steps < kMaxSteps; ++steps) {
        if (unfoldable.empty())
            break;
        const pud_global_node_id chosen_leaf = *unfoldable.begin();
        std::optional<pud_body_goal_id> chosen_goal;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(chosen_leaf).body) {
            if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                chosen_goal = goal_id;
        }
        const pud_frontier_change change = runtime.unfold(chosen_leaf, *chosen_goal);
        live.erase(chosen_leaf);
        unfoldable.erase(chosen_leaf);
        for (pud_global_node_id child : change.offspring) {
            live.insert(child);
            if (child.module == 2 && !runtime.get_rule_information(child).body.empty())
                unfoldable.insert(child);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            unfoldable.erase(refuted);
        }
    }
    std::set<pud_global_node_id> reference_unfoldable;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        if (!runtime.get_rule_information(leaf).body.empty())
            reference_unfoldable.insert(leaf);
    }
    EXPECT_EQ(unfoldable, reference_unfoldable);

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

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesMulPairsProductEightInEveryUnfoldOrder) {
    const expr* eight_product = this->atom("zero");
    for (int peano_step = 0; peano_step < 8; ++peano_step)
        eight_product = this->functor("suc", {eight_product});
    const expr* one = this->functor("suc", {this->atom("zero")});
    const expr* two = this->functor("suc", {one});
    const expr* four = this->functor("suc", {this->functor("suc", {two})});
    const std::set<const expr*> expected{
        this->functor("q", {one, eight_product}),
        this->functor("q", {two, four}),
        this->functor("q", {four, two}),
        this->functor("q", {eight_product, one}),
    };
    // Per-seed exhaustion is impossible (same infinite Y=zero branch as the deterministic test). Union over
    // 20 seeds reaches all four factor pairs at 3k/6k/12k steps per seed (measured).
    std::set<const expr*> found_across_seeds;
    static constexpr size_t kMaxStepsPerSeed = 3000;
    for (uint32_t seed = 0; seed < 20; ++seed) {
        std::mt19937 rng(seed);
            std::deque<pud_module> modules;
            modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
                rule(this->functor("nat", {this->atom("zero")}), {}, 0),
                rule(this->functor("nat", {this->functor("suc", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
                rule(this->functor("add", {this->atom("zero"), this->var(0), this->var(0)}), {this->functor("nat", {this->var(0)})}, 1),
                rule(
                    this->functor("add", {this->functor("suc", {this->var(0)}), this->var(1), this->functor("suc", {this->var(2)})}),
                    {this->functor("add", {this->var(0), this->var(1), this->var(2)})},
                    3),
                rule(this->functor("mul", {this->atom("zero"), this->var(0), this->atom("zero")}), {this->functor("nat", {this->var(0)})}, 1),
                rule(
                    this->functor("mul", {this->functor("suc", {this->var(0)}), this->var(1), this->var(2)}),
                    {this->functor("mul", {this->var(0), this->var(1), this->var(3)}), this->functor("add", {this->var(3), this->var(1), this->var(2)})},
                    4),
            });
            modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
                rule(
                    this->functor("q", {this->var(0), this->var(1)}),
                    {this->functor("mul", {this->var(0), this->var(1), eight_product})},
                    2),
            });
        TypeParam runtime(this->exprs);
        std::set<pud_global_node_id> live;
        std::set<pud_global_node_id> unfoldable;
        for (pud_module& module : modules) {
            install_shared_functor_names(module, this->functors);
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id leaf : change.offspring) {
                live.insert(leaf);
                    if (leaf.module == 2 && !runtime.get_rule_information(leaf).body.empty())
                    unfoldable.insert(leaf);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                        unfoldable.erase(refuted);
            }
        }

        size_t steps = 0;
        for (; steps < kMaxStepsPerSeed; ++steps) {
            if (unfoldable.empty())
                break;
            const pud_global_node_id leaf = *std::next(unfoldable.begin(), rng() % unfoldable.size());
            std::vector<pud_body_goal_id> goal_ids;
            for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
                goal_ids.push_back(goal_id);
            const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
            const pud_frontier_change change = runtime.unfold(leaf, goal);
            live.erase(leaf);
            unfoldable.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
                live.insert(child);
                if (child.module == 2 && !runtime.get_rule_information(child).body.empty())
                    unfoldable.insert(child);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                    unfoldable.erase(refuted);
            }
        }
        std::set<pud_global_node_id> reference_unfoldable;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            if (!runtime.get_rule_information(leaf).body.empty())
                reference_unfoldable.insert(leaf);
        }
        EXPECT_EQ(unfoldable, reference_unfoldable);

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

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesDualBoundedSharedXSums) {
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {this->atom("zero")}), {}, 0),
        rule(this->functor("nat", {this->functor("suc", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("add", {this->atom("zero"), this->var(0), this->var(0)}), {this->functor("nat", {this->var(0)})}, 1),
        rule(
            this->functor("add", {this->functor("suc", {this->var(0)}), this->var(1), this->functor("suc", {this->var(2)})}),
            {this->functor("add", {this->var(0), this->var(1), this->var(2)})},
            3),
        rule(this->functor("lt", {this->atom("zero"), this->functor("suc", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(
            this->functor("lt", {this->functor("suc", {this->var(0)}), this->functor("suc", {this->var(1)})}),
            {this->functor("lt", {this->var(0), this->var(1)})},
            2),
    });
    const expr* bound = this->atom("zero");
    for (int peano_step = 0; peano_step < 4; ++peano_step)
        bound = this->functor("suc", {bound});
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(
            this->functor("q", {this->var(0), this->var(1), this->var(2)}),
            {
                this->functor("nat", {this->var(0)}),
                this->functor("nat", {this->var(1)}),
                this->functor("nat", {this->var(2)}),
                this->functor("add", {this->var(0), this->var(1), this->var(3)}),
                this->functor("add", {this->var(0), this->var(2), this->var(4)}),
                this->functor("lt", {this->var(3), bound}),
                this->functor("lt", {this->var(4), bound}),
            },
            5),
    });
    std::set<const expr*> expected;
    for (int x = 0; x < 4; ++x) {
        const expr* x_term = this->atom("zero");
        for (int peano_step = 0; peano_step < x; ++peano_step)
            x_term = this->functor("suc", {x_term});
        for (int y = 0; y < 4 - x; ++y) {
            const expr* y_term = this->atom("zero");
            for (int peano_step = 0; peano_step < y; ++peano_step)
                y_term = this->functor("suc", {y_term});
            for (int z = 0; z < 4 - x; ++z) {
                const expr* z_term = this->atom("zero");
                for (int peano_step = 0; peano_step < z; ++peano_step)
                    z_term = this->functor("suc", {z_term});
                expected.insert(this->functor("q", {x_term, y_term, z_term}));
            }
        }
    }
    ASSERT_EQ(expected.size(), 30u);
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    std::set<pud_global_node_id> unfoldable;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id leaf : change.offspring) {
            live.insert(leaf);
            if (leaf.module == 2 && !runtime.get_rule_information(leaf).body.empty())
                unfoldable.insert(leaf);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            unfoldable.erase(refuted);
        }
    }

    size_t steps = 0;
    for (; steps < 15000; ++steps) {
        if (unfoldable.empty())
            break;
        const pud_global_node_id chosen_leaf = *unfoldable.begin();
        std::optional<pud_body_goal_id> chosen_goal;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(chosen_leaf).body) {
            if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                chosen_goal = goal_id;
        }
        const pud_frontier_change change = runtime.unfold(chosen_leaf, *chosen_goal);
        live.erase(chosen_leaf);
        unfoldable.erase(chosen_leaf);
        for (pud_global_node_id child : change.offspring) {
            live.insert(child);
            if (child.module == 2 && !runtime.get_rule_information(child).body.empty())
                unfoldable.insert(child);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            unfoldable.erase(refuted);
        }
    }
    EXPECT_LT(steps, 15000u);
    std::set<pud_global_node_id> reference_unfoldable;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        if (!runtime.get_rule_information(leaf).body.empty())
            reference_unfoldable.insert(leaf);
    }
    EXPECT_EQ(unfoldable, reference_unfoldable);

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

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesDualBoundedSharedXSumsInEveryUnfoldOrder) {
    for (uint32_t seed = 0; seed < 20; ++seed) {
        std::mt19937 rng(seed);
            std::deque<pud_module> modules;
            modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
                rule(this->functor("nat", {this->atom("zero")}), {}, 0),
                rule(this->functor("nat", {this->functor("suc", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
                rule(this->functor("add", {this->atom("zero"), this->var(0), this->var(0)}), {this->functor("nat", {this->var(0)})}, 1),
                rule(
                    this->functor("add", {this->functor("suc", {this->var(0)}), this->var(1), this->functor("suc", {this->var(2)})}),
                    {this->functor("add", {this->var(0), this->var(1), this->var(2)})},
                    3),
                rule(this->functor("lt", {this->atom("zero"), this->functor("suc", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
                rule(
                    this->functor("lt", {this->functor("suc", {this->var(0)}), this->functor("suc", {this->var(1)})}),
                    {this->functor("lt", {this->var(0), this->var(1)})},
                    2),
            });
            const expr* bound = this->atom("zero");
            for (int peano_step = 0; peano_step < 4; ++peano_step)
                bound = this->functor("suc", {bound});
            modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
                rule(
                    this->functor("q", {this->var(0), this->var(1), this->var(2)}),
                    {
                        this->functor("nat", {this->var(0)}),
                        this->functor("nat", {this->var(1)}),
                        this->functor("nat", {this->var(2)}),
                        this->functor("add", {this->var(0), this->var(1), this->var(3)}),
                        this->functor("add", {this->var(0), this->var(2), this->var(4)}),
                        this->functor("lt", {this->var(3), bound}),
                        this->functor("lt", {this->var(4), bound}),
                    },
                    5),
            });
            std::set<const expr*> expected;
            for (int x = 0; x < 4; ++x) {
                const expr* x_term = this->atom("zero");
                for (int peano_step = 0; peano_step < x; ++peano_step)
                    x_term = this->functor("suc", {x_term});
                for (int y = 0; y < 4 - x; ++y) {
                    const expr* y_term = this->atom("zero");
                    for (int peano_step = 0; peano_step < y; ++peano_step)
                        y_term = this->functor("suc", {y_term});
                    for (int z = 0; z < 4 - x; ++z) {
                        const expr* z_term = this->atom("zero");
                        for (int peano_step = 0; peano_step < z; ++peano_step)
                            z_term = this->functor("suc", {z_term});
                        expected.insert(this->functor("q", {x_term, y_term, z_term}));
                    }
                }
            }
            ASSERT_EQ(expected.size(), 30u);
        TypeParam runtime(this->exprs);
        std::set<pud_global_node_id> live;
        std::set<pud_global_node_id> unfoldable;
        for (pud_module& module : modules) {
            install_shared_functor_names(module, this->functors);
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id leaf : change.offspring) {
                live.insert(leaf);
                    if (leaf.module == 2 && !runtime.get_rule_information(leaf).body.empty())
                    unfoldable.insert(leaf);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                        unfoldable.erase(refuted);
            }
        }

        size_t steps = 0;
        for (; steps < 15000; ++steps) {
            if (unfoldable.empty())
                break;
            const pud_global_node_id leaf = *std::next(unfoldable.begin(), rng() % unfoldable.size());
            std::vector<pud_body_goal_id> goal_ids;
            for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
                goal_ids.push_back(goal_id);
            const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
            const pud_frontier_change change = runtime.unfold(leaf, goal);
            live.erase(leaf);
            unfoldable.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
                live.insert(child);
                if (child.module == 2 && !runtime.get_rule_information(child).body.empty())
                    unfoldable.insert(child);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                        unfoldable.erase(refuted);
            }
        }
        EXPECT_LT(steps, 15000u);
        std::set<pud_global_node_id> reference_unfoldable;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            if (!runtime.get_rule_information(leaf).body.empty())
                reference_unfoldable.insert(leaf);
        }
        EXPECT_EQ(unfoldable, reference_unfoldable);

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
    }
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, CatalanMinimalCaseOneNode) {
    const expr* nil = this->atom("nil");
    const expr* zero = this->atom("zero");
    const expr* one = this->functor("suc", {zero});
    const expr* lone = this->functor("bin", {nil, nil});
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {zero}), {}, 0),
        rule(this->functor("nat", {this->functor("suc", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("add", {zero, this->var(0), this->var(0)}), {this->functor("nat", {this->var(0)})}, 1),
        rule(
            this->functor("add", {this->functor("suc", {this->var(0)}), this->var(1), this->functor("suc", {this->var(2)})}),
            {this->functor("add", {this->var(0), this->var(1), this->var(2)})},
            3),
        rule(this->functor("wf", {nil}), {}, 0),
        rule(
            this->functor("wf", {this->functor("bin", {this->var(0), this->var(1)})}),
            {this->functor("wf", {this->var(0)}), this->functor("wf", {this->var(1)})},
            2),
        rule(this->functor("nodes", {nil, zero}), {}, 0),
        rule(
            this->functor("nodes", {this->functor("bin", {this->var(0), this->var(1)}), this->var(2)}),
            {
                this->functor("nodes", {this->var(0), this->var(3)}),
                this->functor("nodes", {this->var(1), this->var(4)}),
                this->functor("add", {this->var(3), this->var(4), this->var(5)}),
                this->functor("add", {one, this->var(5), this->var(2)}),
            },
            6),
    });
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(
            this->functor("q", {this->var(0)}),
            {this->functor("wf", {this->var(0)}), this->functor("nodes", {this->var(0), one})},
            1),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    std::set<pud_global_node_id> unfoldable;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id leaf : change.offspring) {
            live.insert(leaf);
            if (leaf.module == 2 && !runtime.get_rule_information(leaf).body.empty())
                unfoldable.insert(leaf);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            unfoldable.erase(refuted);
        }
    }
    size_t steps = 0;
    for (; steps < 500; ++steps) {
        if (unfoldable.empty())
            break;
        const pud_global_node_id chosen_leaf = *unfoldable.begin();
        std::optional<pud_body_goal_id> chosen_goal;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(chosen_leaf).body) {
            if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                chosen_goal = goal_id;
        }
        const pud_frontier_change change = runtime.unfold(chosen_leaf, *chosen_goal);
        live.erase(chosen_leaf);
        unfoldable.erase(chosen_leaf);
        for (pud_global_node_id child : change.offspring) {
            live.insert(child);
            if (child.module == 2 && !runtime.get_rule_information(child).body.empty())
                unfoldable.insert(child);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            unfoldable.erase(refuted);
        }
    }
    EXPECT_LT(steps, 500u);
    std::set<const expr*> answers;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        const pud_unnormalized_rule_information info = runtime.get_rule_information(leaf);
        if (!info.body.empty())
            continue;
        answers.insert(reference_pud_normalize_with_bindings(info.bindings, this->exprs, info.head));
    }
    EXPECT_EQ(answers, (std::set<const expr*>{this->functor("q", {lone})}));
    // No legacy RuntimeParamTest counterpart; smoke coverage for Catalan-shaped rules at n=1.
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesCatalanTreesWithFiveNodes) {
    const expr* nil = this->atom("nil");
    const expr* zero = this->atom("zero");
    const expr* one = this->functor("suc", {zero});
    const expr* s1 = this->functor("bin", {nil, nil});
    const expr* s2_left_chain = this->functor("bin", {nil, s1});
    const expr* s2_right_chain = this->functor("bin", {s1, nil});
    const expr* s3_0 = this->functor("bin", {nil, s2_left_chain});
    const expr* s3_1 = this->functor("bin", {nil, s2_right_chain});
    const expr* s3_2 = this->functor("bin", {s1, s1});
    const expr* s3_3 = this->functor("bin", {s2_left_chain, nil});
    const expr* s3_4 = this->functor("bin", {s2_right_chain, nil});
    const expr* s4_0 = this->functor("bin", {nil, s3_0});
    const expr* s4_1 = this->functor("bin", {nil, s3_1});
    const expr* s4_2 = this->functor("bin", {nil, s3_2});
    const expr* s4_3 = this->functor("bin", {nil, s3_3});
    const expr* s4_4 = this->functor("bin", {nil, s3_4});
    const expr* s4_5 = this->functor("bin", {s1, s2_left_chain});
    const expr* s4_6 = this->functor("bin", {s1, s2_right_chain});
    const expr* s4_7 = this->functor("bin", {s2_left_chain, s1});
    const expr* s4_8 = this->functor("bin", {s2_right_chain, s1});
    const expr* s4_9 = this->functor("bin", {s3_0, nil});
    const expr* s4_10 = this->functor("bin", {s3_1, nil});
    const expr* s4_11 = this->functor("bin", {s3_2, nil});
    const expr* s4_12 = this->functor("bin", {s3_3, nil});
    const expr* s4_13 = this->functor("bin", {s3_4, nil});
    std::set<const expr*> expected{
    this->functor("q", {this->functor("bin", {nil, s4_0})}),
    this->functor("q", {this->functor("bin", {nil, s4_1})}),
    this->functor("q", {this->functor("bin", {nil, s4_2})}),
    this->functor("q", {this->functor("bin", {nil, s4_3})}),
    this->functor("q", {this->functor("bin", {nil, s4_4})}),
    this->functor("q", {this->functor("bin", {nil, s4_5})}),
    this->functor("q", {this->functor("bin", {nil, s4_6})}),
    this->functor("q", {this->functor("bin", {nil, s4_7})}),
    this->functor("q", {this->functor("bin", {nil, s4_8})}),
    this->functor("q", {this->functor("bin", {nil, s4_9})}),
    this->functor("q", {this->functor("bin", {nil, s4_10})}),
    this->functor("q", {this->functor("bin", {nil, s4_11})}),
    this->functor("q", {this->functor("bin", {nil, s4_12})}),
    this->functor("q", {this->functor("bin", {nil, s4_13})}),
    this->functor("q", {this->functor("bin", {s1, s3_0})}),
    this->functor("q", {this->functor("bin", {s1, s3_1})}),
    this->functor("q", {this->functor("bin", {s1, s3_2})}),
    this->functor("q", {this->functor("bin", {s1, s3_3})}),
    this->functor("q", {this->functor("bin", {s1, s3_4})}),
    this->functor("q", {this->functor("bin", {s2_left_chain, s2_left_chain})}),
    this->functor("q", {this->functor("bin", {s2_left_chain, s2_right_chain})}),
    this->functor("q", {this->functor("bin", {s2_right_chain, s2_left_chain})}),
    this->functor("q", {this->functor("bin", {s2_right_chain, s2_right_chain})}),
    this->functor("q", {this->functor("bin", {s3_0, s1})}),
    this->functor("q", {this->functor("bin", {s3_1, s1})}),
    this->functor("q", {this->functor("bin", {s3_2, s1})}),
    this->functor("q", {this->functor("bin", {s3_3, s1})}),
    this->functor("q", {this->functor("bin", {s3_4, s1})}),
    this->functor("q", {this->functor("bin", {s4_0, nil})}),
    this->functor("q", {this->functor("bin", {s4_1, nil})}),
    this->functor("q", {this->functor("bin", {s4_2, nil})}),
    this->functor("q", {this->functor("bin", {s4_3, nil})}),
    this->functor("q", {this->functor("bin", {s4_4, nil})}),
    this->functor("q", {this->functor("bin", {s4_5, nil})}),
    this->functor("q", {this->functor("bin", {s4_6, nil})}),
    this->functor("q", {this->functor("bin", {s4_7, nil})}),
    this->functor("q", {this->functor("bin", {s4_8, nil})}),
    this->functor("q", {this->functor("bin", {s4_9, nil})}),
    this->functor("q", {this->functor("bin", {s4_10, nil})}),
    this->functor("q", {this->functor("bin", {s4_11, nil})}),
    this->functor("q", {this->functor("bin", {s4_12, nil})}),
    this->functor("q", {this->functor("bin", {s4_13, nil})}),
    };
    ASSERT_EQ(expected.size(), 42u);
    const expr* five = zero;
    for (int peano_step = 0; peano_step < 5; ++peano_step)
        five = this->functor("suc", {five});
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {zero}), {}, 0),
        rule(this->functor("nat", {this->functor("suc", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
        rule(this->functor("add", {zero, this->var(0), this->var(0)}), {this->functor("nat", {this->var(0)})}, 1),
        rule(
            this->functor("add", {this->functor("suc", {this->var(0)}), this->var(1), this->functor("suc", {this->var(2)})}),
            {this->functor("add", {this->var(0), this->var(1), this->var(2)})},
            3),
        rule(this->functor("wf", {nil}), {}, 0),
        rule(
            this->functor("wf", {this->functor("bin", {this->var(0), this->var(1)})}),
            {this->functor("wf", {this->var(0)}), this->functor("wf", {this->var(1)})},
            2),
        rule(this->functor("nodes", {nil, zero}), {}, 0),
        rule(
            this->functor("nodes", {this->functor("bin", {this->var(0), this->var(1)}), this->var(2)}),
            {
                this->functor("nodes", {this->var(0), this->var(3)}),
                this->functor("nodes", {this->var(1), this->var(4)}),
                this->functor("add", {this->var(3), this->var(4), this->var(5)}),
                this->functor("add", {one, this->var(5), this->var(2)}),
            },
            6),
    });
    // Query matches runtime_test_suite goal order (wf then nodes); the drive picks goals by predicate rank
    // (add, nodes, nat, wf) because goal ids are not predicate-ordered and smallest-id never terminates here.
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(
            this->functor("q", {this->var(0)}),
            {this->functor("wf", {this->var(0)}), this->functor("nodes", {this->var(0), five})},
            1),
    });

    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    std::set<pud_global_node_id> unfoldable;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id leaf : change.offspring) {
            live.insert(leaf);
            if (leaf.module == 2 && !runtime.get_rule_information(leaf).body.empty())
                unfoldable.insert(leaf);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            unfoldable.erase(refuted);
        }
    }

    const uint32_t add_functor = this->functors.id("add");
    const uint32_t nodes_functor = this->functors.id("nodes");
    const uint32_t nat_functor = this->functors.id("nat");
    const uint32_t wf_functor = this->functors.id("wf");
    size_t steps = 0;
    // Finite program: unfold add before nodes before nat before wf (goal ids alone are not predicate order).
    for (; steps < 200000; ++steps) {
        if (unfoldable.empty())
            break;
        const pud_global_node_id chosen_leaf = *unfoldable.begin();
        std::optional<pud_body_goal_id> chosen_goal;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(chosen_leaf).body) {
            if (!std::holds_alternative<expr::functor>(goal->content)) {
                if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                    chosen_goal = goal_id;
                continue;
            }
            const expr::functor& goal_head = std::get<expr::functor>(goal->content);
            int goal_rank = 4;
            if (goal_head.id == add_functor)
                goal_rank = 0;
            else if (goal_head.id == nodes_functor)
                goal_rank = 1;
            else if (goal_head.id == nat_functor)
                goal_rank = 2;
            else if (goal_head.id == wf_functor)
                goal_rank = 3;
            if (!chosen_goal.has_value()) {
                chosen_goal = goal_id;
                continue;
            }
            const expr* chosen_expr = runtime.get_rule_information(chosen_leaf).body.at(*chosen_goal);
            int chosen_rank = 4;
            if (std::holds_alternative<expr::functor>(chosen_expr->content)) {
                const expr::functor& chosen_head = std::get<expr::functor>(chosen_expr->content);
                if (chosen_head.id == add_functor)
                    chosen_rank = 0;
                else if (chosen_head.id == nodes_functor)
                    chosen_rank = 1;
                else if (chosen_head.id == nat_functor)
                    chosen_rank = 2;
                else if (chosen_head.id == wf_functor)
                    chosen_rank = 3;
            }
            if (goal_rank < chosen_rank || (goal_rank == chosen_rank && goal_id < *chosen_goal))
                chosen_goal = goal_id;
        }
        const pud_frontier_change change = runtime.unfold(chosen_leaf, *chosen_goal);
        live.erase(chosen_leaf);
        unfoldable.erase(chosen_leaf);
        for (pud_global_node_id child : change.offspring) {
            live.insert(child);
            if (child.module == 2 && !runtime.get_rule_information(child).body.empty())
                unfoldable.insert(child);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            unfoldable.erase(refuted);
        }
    }
    EXPECT_LT(steps, 200000u);
    std::set<pud_global_node_id> reference_unfoldable;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        if (!runtime.get_rule_information(leaf).body.empty())
            reference_unfoldable.insert(leaf);
    }
    EXPECT_EQ(unfoldable, reference_unfoldable);

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

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesCatalanTreesWithFiveNodesInEveryUnfoldOrder) {
    std::set<const expr*> found_across_seeds;
    for (uint32_t seed = 0; seed < 20; ++seed) {
        std::mt19937 rng(seed);
            const expr* nil = this->atom("nil");
            const expr* zero = this->atom("zero");
            const expr* one = this->functor("suc", {zero});
            const expr* s1 = this->functor("bin", {nil, nil});
            const expr* s2_left_chain = this->functor("bin", {nil, s1});
            const expr* s2_right_chain = this->functor("bin", {s1, nil});
            const expr* s3_0 = this->functor("bin", {nil, s2_left_chain});
            const expr* s3_1 = this->functor("bin", {nil, s2_right_chain});
            const expr* s3_2 = this->functor("bin", {s1, s1});
            const expr* s3_3 = this->functor("bin", {s2_left_chain, nil});
            const expr* s3_4 = this->functor("bin", {s2_right_chain, nil});
            const expr* s4_0 = this->functor("bin", {nil, s3_0});
            const expr* s4_1 = this->functor("bin", {nil, s3_1});
            const expr* s4_2 = this->functor("bin", {nil, s3_2});
            const expr* s4_3 = this->functor("bin", {nil, s3_3});
            const expr* s4_4 = this->functor("bin", {nil, s3_4});
            const expr* s4_5 = this->functor("bin", {s1, s2_left_chain});
            const expr* s4_6 = this->functor("bin", {s1, s2_right_chain});
            const expr* s4_7 = this->functor("bin", {s2_left_chain, s1});
            const expr* s4_8 = this->functor("bin", {s2_right_chain, s1});
            const expr* s4_9 = this->functor("bin", {s3_0, nil});
            const expr* s4_10 = this->functor("bin", {s3_1, nil});
            const expr* s4_11 = this->functor("bin", {s3_2, nil});
            const expr* s4_12 = this->functor("bin", {s3_3, nil});
            const expr* s4_13 = this->functor("bin", {s3_4, nil});
            std::set<const expr*> expected{
            this->functor("q", {this->functor("bin", {nil, s4_0})}),
            this->functor("q", {this->functor("bin", {nil, s4_1})}),
            this->functor("q", {this->functor("bin", {nil, s4_2})}),
            this->functor("q", {this->functor("bin", {nil, s4_3})}),
            this->functor("q", {this->functor("bin", {nil, s4_4})}),
            this->functor("q", {this->functor("bin", {nil, s4_5})}),
            this->functor("q", {this->functor("bin", {nil, s4_6})}),
            this->functor("q", {this->functor("bin", {nil, s4_7})}),
            this->functor("q", {this->functor("bin", {nil, s4_8})}),
            this->functor("q", {this->functor("bin", {nil, s4_9})}),
            this->functor("q", {this->functor("bin", {nil, s4_10})}),
            this->functor("q", {this->functor("bin", {nil, s4_11})}),
            this->functor("q", {this->functor("bin", {nil, s4_12})}),
            this->functor("q", {this->functor("bin", {nil, s4_13})}),
            this->functor("q", {this->functor("bin", {s1, s3_0})}),
            this->functor("q", {this->functor("bin", {s1, s3_1})}),
            this->functor("q", {this->functor("bin", {s1, s3_2})}),
            this->functor("q", {this->functor("bin", {s1, s3_3})}),
            this->functor("q", {this->functor("bin", {s1, s3_4})}),
            this->functor("q", {this->functor("bin", {s2_left_chain, s2_left_chain})}),
            this->functor("q", {this->functor("bin", {s2_left_chain, s2_right_chain})}),
            this->functor("q", {this->functor("bin", {s2_right_chain, s2_left_chain})}),
            this->functor("q", {this->functor("bin", {s2_right_chain, s2_right_chain})}),
            this->functor("q", {this->functor("bin", {s3_0, s1})}),
            this->functor("q", {this->functor("bin", {s3_1, s1})}),
            this->functor("q", {this->functor("bin", {s3_2, s1})}),
            this->functor("q", {this->functor("bin", {s3_3, s1})}),
            this->functor("q", {this->functor("bin", {s3_4, s1})}),
            this->functor("q", {this->functor("bin", {s4_0, nil})}),
            this->functor("q", {this->functor("bin", {s4_1, nil})}),
            this->functor("q", {this->functor("bin", {s4_2, nil})}),
            this->functor("q", {this->functor("bin", {s4_3, nil})}),
            this->functor("q", {this->functor("bin", {s4_4, nil})}),
            this->functor("q", {this->functor("bin", {s4_5, nil})}),
            this->functor("q", {this->functor("bin", {s4_6, nil})}),
            this->functor("q", {this->functor("bin", {s4_7, nil})}),
            this->functor("q", {this->functor("bin", {s4_8, nil})}),
            this->functor("q", {this->functor("bin", {s4_9, nil})}),
            this->functor("q", {this->functor("bin", {s4_10, nil})}),
            this->functor("q", {this->functor("bin", {s4_11, nil})}),
            this->functor("q", {this->functor("bin", {s4_12, nil})}),
            this->functor("q", {this->functor("bin", {s4_13, nil})}),
            };
            ASSERT_EQ(expected.size(), 42u);
            const expr* five = zero;
            for (int peano_step = 0; peano_step < 5; ++peano_step)
                five = this->functor("suc", {five});
                std::deque<pud_module> modules;
                modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
                    rule(this->functor("nat", {zero}), {}, 0),
                    rule(this->functor("nat", {this->functor("suc", {this->var(0)})}), {this->functor("nat", {this->var(0)})}, 1),
                    rule(this->functor("add", {zero, this->var(0), this->var(0)}), {this->functor("nat", {this->var(0)})}, 1),
                    rule(
                        this->functor("add", {this->functor("suc", {this->var(0)}), this->var(1), this->functor("suc", {this->var(2)})}),
                        {this->functor("add", {this->var(0), this->var(1), this->var(2)})},
                        3),
                    rule(this->functor("wf", {nil}), {}, 0),
                    rule(
                        this->functor("wf", {this->functor("bin", {this->var(0), this->var(1)})}),
                        {this->functor("wf", {this->var(0)}), this->functor("wf", {this->var(1)})},
                        2),
                    rule(this->functor("nodes", {nil, zero}), {}, 0),
                    rule(
                        this->functor("nodes", {this->functor("bin", {this->var(0), this->var(1)}), this->var(2)}),
                        {
                            this->functor("nodes", {this->var(0), this->var(3)}),
                            this->functor("nodes", {this->var(1), this->var(4)}),
                            this->functor("add", {this->var(3), this->var(4), this->var(5)}),
                            this->functor("add", {one, this->var(5), this->var(2)}),
                        },
                        6),
                });
                modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
                    rule(
                        this->functor("q", {this->var(0)}),
                        {this->functor("wf", {this->var(0)}), this->functor("nodes", {this->var(0), five})},
                        1),
            });
        TypeParam runtime(this->exprs);
        std::set<pud_global_node_id> live;
        std::set<pud_global_node_id> unfoldable;
        for (pud_module& module : modules) {
            install_shared_functor_names(module, this->functors);
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id leaf : change.offspring) {
                live.insert(leaf);
                    if (leaf.module == 2 && !runtime.get_rule_information(leaf).body.empty())
                    unfoldable.insert(leaf);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                    unfoldable.erase(refuted);
            }
        }

        const uint32_t add_functor = this->functors.id("add");
        const uint32_t nodes_functor = this->functors.id("nodes");
        const uint32_t nat_functor = this->functors.id("nat");
        const uint32_t wf_functor = this->functors.id("wf");
        static constexpr size_t kMaxStepsPerSeed = 40000;
        size_t steps = 0;
        for (; steps < kMaxStepsPerSeed; ++steps) {
            if (unfoldable.empty())
                break;
            const pud_global_node_id leaf = *std::next(unfoldable.begin(), rng() % unfoldable.size());
            std::optional<pud_body_goal_id> chosen_goal;
            for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body) {
                if (!std::holds_alternative<expr::functor>(goal->content)) {
                    if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                        chosen_goal = goal_id;
                    continue;
                }
                const expr::functor& goal_head = std::get<expr::functor>(goal->content);
                int goal_rank = 4;
                if (goal_head.id == add_functor)
                    goal_rank = 0;
                else if (goal_head.id == nodes_functor)
                    goal_rank = 1;
                else if (goal_head.id == nat_functor)
                    goal_rank = 2;
                else if (goal_head.id == wf_functor)
                    goal_rank = 3;
                if (!chosen_goal.has_value()) {
                    chosen_goal = goal_id;
                    continue;
                }
                const expr* chosen_expr = runtime.get_rule_information(leaf).body.at(*chosen_goal);
                int chosen_rank = 4;
                if (std::holds_alternative<expr::functor>(chosen_expr->content)) {
                    const expr::functor& chosen_head = std::get<expr::functor>(chosen_expr->content);
                    if (chosen_head.id == add_functor)
                        chosen_rank = 0;
                    else if (chosen_head.id == nodes_functor)
                        chosen_rank = 1;
                    else if (chosen_head.id == nat_functor)
                        chosen_rank = 2;
                    else if (chosen_head.id == wf_functor)
                        chosen_rank = 3;
                }
                if (goal_rank < chosen_rank || (goal_rank == chosen_rank && goal_id < *chosen_goal))
                    chosen_goal = goal_id;
            }
            const pud_body_goal_id goal = *chosen_goal;
            const pud_frontier_change change = runtime.unfold(leaf, goal);
            live.erase(leaf);
            unfoldable.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
                live.insert(child);
                if (child.module == 2 && !runtime.get_rule_information(child).body.empty())
                    unfoldable.insert(child);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                        unfoldable.erase(refuted);
            }
        }
        std::set<pud_global_node_id> reference_unfoldable;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            if (!runtime.get_rule_information(leaf).body.empty())
                reference_unfoldable.insert(leaf);
        }
        EXPECT_EQ(unfoldable, reference_unfoldable);

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
    EXPECT_EQ(found_across_seeds.size(), 42u);
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesFourTwoGoalGroundCombinations) {
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("f", {}), {}, 0),
        rule(this->functor("f", {}), {}, 0),
        rule(this->functor("g", {}), {}, 0),
        rule(this->functor("g", {}), {}, 0),
    });
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {}), {this->functor("f", {}), this->functor("g", {})}, 0),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    std::set<pud_global_node_id> unfoldable;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id leaf : change.offspring) {
            live.insert(leaf);
            if (leaf.module == 2 && !runtime.get_rule_information(leaf).body.empty())
                unfoldable.insert(leaf);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            unfoldable.erase(refuted);
        }
    }

    size_t steps = 0;
    for (; steps < 2000; ++steps) {
        if (unfoldable.empty())
            break;
        const pud_global_node_id chosen_leaf = *unfoldable.begin();
        std::optional<pud_body_goal_id> chosen_goal;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(chosen_leaf).body) {
            if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                chosen_goal = goal_id;
        }
        const pud_frontier_change change = runtime.unfold(chosen_leaf, *chosen_goal);
        live.erase(chosen_leaf);
        unfoldable.erase(chosen_leaf);
        for (pud_global_node_id child : change.offspring) {
            live.insert(child);
            if (child.module == 2 && !runtime.get_rule_information(child).body.empty())
                unfoldable.insert(child);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            unfoldable.erase(refuted);
        }
    }
    EXPECT_LT(steps, 2000u);
    std::set<pud_global_node_id> reference_unfoldable;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        if (!runtime.get_rule_information(leaf).body.empty())
            reference_unfoldable.insert(leaf);
    }
    EXPECT_EQ(unfoldable, reference_unfoldable);

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
    size_t solved_live_leaves = 0;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        if (!runtime.get_rule_information(leaf).body.empty())
            continue;
        ++solved_live_leaves;
    }
    EXPECT_EQ(solved_live_leaves, 4u);
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesFourTwoGoalGroundCombinationsInEveryUnfoldOrder) {
    const std::set<const expr*> expected = (std::set<const expr*>{this->functor("q", {})});
    for (uint32_t seed = 0; seed < 20; ++seed) {
        std::mt19937 rng(seed);
            std::deque<pud_module> modules;
            modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
                rule(this->functor("f", {}), {}, 0),
                rule(this->functor("f", {}), {}, 0),
                rule(this->functor("g", {}), {}, 0),
                rule(this->functor("g", {}), {}, 0),
            });
            modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
                rule(this->functor("q", {}), {this->functor("f", {}), this->functor("g", {})}, 0),
            });
        TypeParam runtime(this->exprs);
        std::set<pud_global_node_id> live;
        std::set<pud_global_node_id> unfoldable;
        for (pud_module& module : modules) {
            install_shared_functor_names(module, this->functors);
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id leaf : change.offspring) {
                live.insert(leaf);
                    if (leaf.module == 2 && !runtime.get_rule_information(leaf).body.empty())
                    unfoldable.insert(leaf);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                        unfoldable.erase(refuted);
            }
        }

        size_t steps = 0;
        for (; steps < 2000; ++steps) {
            if (unfoldable.empty())
                break;
            const pud_global_node_id leaf = *std::next(unfoldable.begin(), rng() % unfoldable.size());
            std::vector<pud_body_goal_id> goal_ids;
            for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
                goal_ids.push_back(goal_id);
            const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
            const pud_frontier_change change = runtime.unfold(leaf, goal);
            live.erase(leaf);
            unfoldable.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
                live.insert(child);
                if (child.module == 2 && !runtime.get_rule_information(child).body.empty())
                    unfoldable.insert(child);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                        unfoldable.erase(refuted);
            }
        }
        EXPECT_LT(steps, 2000u);
        std::set<pud_global_node_id> reference_unfoldable;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            if (!runtime.get_rule_information(leaf).body.empty())
                reference_unfoldable.insert(leaf);
        }
        EXPECT_EQ(unfoldable, reference_unfoldable);

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
        size_t solved_live_leaves = 0;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            if (!runtime.get_rule_information(leaf).body.empty())
                continue;
            ++solved_live_leaves;
        }
        EXPECT_EQ(solved_live_leaves, 4u);
    }
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesEightThreeGoalGroundCombinations) {
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("f", {}), {}, 0),
        rule(this->functor("f", {}), {}, 0),
        rule(this->functor("g", {}), {}, 0),
        rule(this->functor("g", {}), {}, 0),
        rule(this->functor("h", {}), {}, 0),
        rule(this->functor("h", {}), {}, 0),
    });
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {}), {this->functor("f", {}), this->functor("g", {}), this->functor("h", {})}, 0),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    std::set<pud_global_node_id> unfoldable;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id leaf : change.offspring) {
            live.insert(leaf);
            if (leaf.module == 2 && !runtime.get_rule_information(leaf).body.empty())
                unfoldable.insert(leaf);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            unfoldable.erase(refuted);
        }
    }

    size_t steps = 0;
    for (; steps < 2000; ++steps) {
        if (unfoldable.empty())
            break;
        const pud_global_node_id chosen_leaf = *unfoldable.begin();
        std::optional<pud_body_goal_id> chosen_goal;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(chosen_leaf).body) {
            if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                chosen_goal = goal_id;
        }
        const pud_frontier_change change = runtime.unfold(chosen_leaf, *chosen_goal);
        live.erase(chosen_leaf);
        unfoldable.erase(chosen_leaf);
        for (pud_global_node_id child : change.offspring) {
            live.insert(child);
            if (child.module == 2 && !runtime.get_rule_information(child).body.empty())
                unfoldable.insert(child);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            unfoldable.erase(refuted);
        }
    }
    EXPECT_LT(steps, 2000u);
    std::set<pud_global_node_id> reference_unfoldable;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        if (!runtime.get_rule_information(leaf).body.empty())
            reference_unfoldable.insert(leaf);
    }
    EXPECT_EQ(unfoldable, reference_unfoldable);

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
    size_t solved_live_leaves = 0;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        if (!runtime.get_rule_information(leaf).body.empty())
            continue;
        ++solved_live_leaves;
    }
    EXPECT_EQ(solved_live_leaves, 8u);
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesEightThreeGoalGroundCombinationsInEveryUnfoldOrder) {
    const std::set<const expr*> expected = (std::set<const expr*>{this->functor("q", {})});
    for (uint32_t seed = 0; seed < 20; ++seed) {
        std::mt19937 rng(seed);
            std::deque<pud_module> modules;
            modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
                rule(this->functor("f", {}), {}, 0),
                rule(this->functor("f", {}), {}, 0),
                rule(this->functor("g", {}), {}, 0),
                rule(this->functor("g", {}), {}, 0),
                rule(this->functor("h", {}), {}, 0),
                rule(this->functor("h", {}), {}, 0),
            });
            modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
                rule(this->functor("q", {}), {this->functor("f", {}), this->functor("g", {}), this->functor("h", {})}, 0),
            });
        TypeParam runtime(this->exprs);
        std::set<pud_global_node_id> live;
        std::set<pud_global_node_id> unfoldable;
        for (pud_module& module : modules) {
            install_shared_functor_names(module, this->functors);
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id leaf : change.offspring) {
                live.insert(leaf);
                    if (leaf.module == 2 && !runtime.get_rule_information(leaf).body.empty())
                    unfoldable.insert(leaf);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                        unfoldable.erase(refuted);
            }
        }

        size_t steps = 0;
        for (; steps < 2000; ++steps) {
            if (unfoldable.empty())
                break;
            const pud_global_node_id leaf = *std::next(unfoldable.begin(), rng() % unfoldable.size());
            std::vector<pud_body_goal_id> goal_ids;
            for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
                goal_ids.push_back(goal_id);
            const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
            const pud_frontier_change change = runtime.unfold(leaf, goal);
            live.erase(leaf);
            unfoldable.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
                live.insert(child);
                if (child.module == 2 && !runtime.get_rule_information(child).body.empty())
                    unfoldable.insert(child);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                        unfoldable.erase(refuted);
            }
        }
        EXPECT_LT(steps, 2000u);
        std::set<pud_global_node_id> reference_unfoldable;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            if (!runtime.get_rule_information(leaf).body.empty())
                reference_unfoldable.insert(leaf);
        }
        EXPECT_EQ(unfoldable, reference_unfoldable);

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
        size_t solved_live_leaves = 0;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            if (!runtime.get_rule_information(leaf).body.empty())
                continue;
            ++solved_live_leaves;
        }
        EXPECT_EQ(solved_live_leaves, 8u);
    }
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesManySharedVarGroundHeads) {
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("f", {}), {}, 0),
        rule(this->functor("f", {}), {}, 0),
        rule(this->functor("g", {this->atom("abc"), this->atom("xyz"), this->atom("pqr")}), {}, 0),
        rule(this->functor("g", {this->atom("def"), this->atom("xyz"), this->atom("pqr")}), {}, 0),
        rule(this->functor("g", {this->atom("ghi"), this->atom("xyz"), this->atom("pqr")}), {}, 0),
        rule(this->functor("g", {this->atom("jkl"), this->atom("xyz"), this->atom("pqr")}), {}, 0),
        rule(this->functor("g", {this->atom("mno"), this->atom("xyz"), this->atom("pqr")}), {}, 0),
    });
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(
            this->functor("q", {this->var(0), this->var(1), this->var(2)}),
            {this->functor("f", {}), this->functor("g", {this->var(0), this->var(1), this->var(2)})},
            3),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    std::set<pud_global_node_id> unfoldable;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id leaf : change.offspring) {
            live.insert(leaf);
            if (leaf.module == 2 && !runtime.get_rule_information(leaf).body.empty())
                unfoldable.insert(leaf);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            unfoldable.erase(refuted);
        }
    }

    size_t steps = 0;
    for (; steps < 2000; ++steps) {
        if (unfoldable.empty())
            break;
        const pud_global_node_id chosen_leaf = *unfoldable.begin();
        std::optional<pud_body_goal_id> chosen_goal;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(chosen_leaf).body) {
            if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                chosen_goal = goal_id;
        }
        const pud_frontier_change change = runtime.unfold(chosen_leaf, *chosen_goal);
        live.erase(chosen_leaf);
        unfoldable.erase(chosen_leaf);
        for (pud_global_node_id child : change.offspring) {
            live.insert(child);
            if (child.module == 2 && !runtime.get_rule_information(child).body.empty())
                unfoldable.insert(child);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            unfoldable.erase(refuted);
        }
    }
    EXPECT_LT(steps, 2000u);
    std::set<pud_global_node_id> reference_unfoldable;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        if (!runtime.get_rule_information(leaf).body.empty())
            reference_unfoldable.insert(leaf);
    }
    EXPECT_EQ(unfoldable, reference_unfoldable);

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
        this->functor("q", {this->atom("abc"), this->atom("xyz"), this->atom("pqr")}),
        this->functor("q", {this->atom("def"), this->atom("xyz"), this->atom("pqr")}),
        this->functor("q", {this->atom("ghi"), this->atom("xyz"), this->atom("pqr")}),
        this->functor("q", {this->atom("jkl"), this->atom("xyz"), this->atom("pqr")}),
        this->functor("q", {this->atom("mno"), this->atom("xyz"), this->atom("pqr")}),
    }));
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesManySharedVarGroundHeadsInEveryUnfoldOrder) {
    const std::set<const expr*> expected = (std::set<const expr*>{
        this->functor("q", {this->atom("abc"), this->atom("xyz"), this->atom("pqr")}),
        this->functor("q", {this->atom("def"), this->atom("xyz"), this->atom("pqr")}),
        this->functor("q", {this->atom("ghi"), this->atom("xyz"), this->atom("pqr")}),
        this->functor("q", {this->atom("jkl"), this->atom("xyz"), this->atom("pqr")}),
        this->functor("q", {this->atom("mno"), this->atom("xyz"), this->atom("pqr")}),
    });
    for (uint32_t seed = 0; seed < 20; ++seed) {
        std::mt19937 rng(seed);
            std::deque<pud_module> modules;
            modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
                rule(this->functor("f", {}), {}, 0),
                rule(this->functor("f", {}), {}, 0),
                rule(this->functor("g", {this->atom("abc"), this->atom("xyz"), this->atom("pqr")}), {}, 0),
                rule(this->functor("g", {this->atom("def"), this->atom("xyz"), this->atom("pqr")}), {}, 0),
                rule(this->functor("g", {this->atom("ghi"), this->atom("xyz"), this->atom("pqr")}), {}, 0),
                rule(this->functor("g", {this->atom("jkl"), this->atom("xyz"), this->atom("pqr")}), {}, 0),
                rule(this->functor("g", {this->atom("mno"), this->atom("xyz"), this->atom("pqr")}), {}, 0),
            });
            modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
                rule(
                    this->functor("q", {this->var(0), this->var(1), this->var(2)}),
                    {this->functor("f", {}), this->functor("g", {this->var(0), this->var(1), this->var(2)})},
                    3),
            });
        TypeParam runtime(this->exprs);
        std::set<pud_global_node_id> live;
        std::set<pud_global_node_id> unfoldable;
        for (pud_module& module : modules) {
            install_shared_functor_names(module, this->functors);
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id leaf : change.offspring) {
                live.insert(leaf);
                    if (leaf.module == 2 && !runtime.get_rule_information(leaf).body.empty())
                    unfoldable.insert(leaf);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                        unfoldable.erase(refuted);
            }
        }

        size_t steps = 0;
        for (; steps < 2000; ++steps) {
            if (unfoldable.empty())
                break;
            const pud_global_node_id leaf = *std::next(unfoldable.begin(), rng() % unfoldable.size());
            std::vector<pud_body_goal_id> goal_ids;
            for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
                goal_ids.push_back(goal_id);
            const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
            const pud_frontier_change change = runtime.unfold(leaf, goal);
            live.erase(leaf);
            unfoldable.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
                live.insert(child);
                if (child.module == 2 && !runtime.get_rule_information(child).body.empty())
                    unfoldable.insert(child);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                        unfoldable.erase(refuted);
            }
        }
        EXPECT_LT(steps, 2000u);
        std::set<pud_global_node_id> reference_unfoldable;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            if (!runtime.get_rule_information(leaf).body.empty())
                reference_unfoldable.insert(leaf);
        }
        EXPECT_EQ(unfoldable, reference_unfoldable);

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
    }
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesThreeGroundBranches) {
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
        install_shared_functor_names(module, this->functors);
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

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesThreeGroundBranchesInEveryUnfoldOrder) {
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
            install_shared_functor_names(module, this->functors);
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

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, SolvesRecursiveClauseTreeWithoutBranching) {
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
        install_shared_functor_names(module, this->functors);
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

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, SolvesRecursiveClauseTreeWithoutBranchingInEveryUnfoldOrder) {
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
            install_shared_functor_names(module, this->functors);
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

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesTransitiveReachFromA) {
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
        install_shared_functor_names(module, this->functors);
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

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesTransitiveReachFromAInEveryUnfoldOrder) {
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
            install_shared_functor_names(module, this->functors);
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

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesEvenPeanoLessThanEight) {
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
        install_shared_functor_names(module, this->functors);
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

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesEvenPeanoLessThanEightInEveryUnfoldOrder) {
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
            install_shared_functor_names(module, this->functors);
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

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesListSplitsForThreeElementList) {
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
        install_shared_functor_names(module, this->functors);
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

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesListSplitsForThreeElementListInEveryUnfoldOrder) {
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
            install_shared_functor_names(module, this->functors);
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

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesTwoChoiceClauseSolutions) {
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
        install_shared_functor_names(module, this->functors);
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

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesTwoChoiceClauseSolutionsInEveryUnfoldOrder) {
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
            install_shared_functor_names(module, this->functors);
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



TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesCollatzOneStepPreimagesOfTen) {
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
        install_shared_functor_names(module, this->functors);
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

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesCollatzOneStepPreimagesOfTenInEveryUnfoldOrder) {
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
            install_shared_functor_names(module, this->functors);
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

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesFibIndicesWithValueBelowFive) {
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
        install_shared_functor_names(module, this->functors);
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

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesFibIndicesWithValueBelowFiveInEveryUnfoldOrder) {
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
            install_shared_functor_names(module, this->functors);
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

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesFactorPairsOfSix) {
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
        install_shared_functor_names(module, this->functors);
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

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesFactorPairsOfSixInEveryUnfoldOrder) {
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
            install_shared_functor_names(module, this->functors);
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

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesDistinctTwoPartPartitionsOfFive) {
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
        install_shared_functor_names(module, this->functors);
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

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesDistinctTwoPartPartitionsOfFiveInEveryUnfoldOrder) {
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
            install_shared_functor_names(module, this->functors);
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

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesArithmeticProgressionsEndingAtFive) {
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
        install_shared_functor_names(module, this->functors);
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

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesArithmeticProgressionsEndingAtFiveInEveryUnfoldOrder) {
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
            install_shared_functor_names(module, this->functors);
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

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, FindsGcdOfSixAndFourViaSubtraction) {
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
        install_shared_functor_names(module, this->functors);
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

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, FindsGcdOfSixAndFourViaSubtractionInEveryUnfoldOrder) {
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
            install_shared_functor_names(module, this->functors);
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

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesTwoSubsetsOfFourElements) {
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
        install_shared_functor_names(module, this->functors);
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

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesTwoSubsetsOfFourElementsInEveryUnfoldOrder) {
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
            install_shared_functor_names(module, this->functors);
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

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesBalancedGrammarStringOfLengthFour) {
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
        install_shared_functor_names(module, this->functors);
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

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesBalancedGrammarStringOfLengthFourInEveryUnfoldOrder) {
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
            install_shared_functor_names(module, this->functors);
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

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesDepthTwoTermsOverTwoConstants) {
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
        install_shared_functor_names(module, this->functors);
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

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesDepthTwoTermsOverTwoConstantsInEveryUnfoldOrder) {
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
            install_shared_functor_names(module, this->functors);
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

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, FindsEvenParityListOfLengthFour) {
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
        install_shared_functor_names(module, this->functors);
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

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, FindsEvenParityListOfLengthFourInEveryUnfoldOrder) {
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
            install_shared_functor_names(module, this->functors);
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

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesPeanoTriplesInsideTetrahedron) {
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
        install_shared_functor_names(module, this->functors);
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

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesPeanoTriplesInsideTetrahedronInEveryUnfoldOrder) {
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
            install_shared_functor_names(module, this->functors);
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

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesLatticePathsFourByFour) {
    const expr* zero = this->atom("zero");
    const expr* four = zero;
    for (int peano_index = 0; peano_index < 4; ++peano_index)
        four = this->functor("suc", {four});
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("at", {zero, zero}), {}, 0),
        rule(
            this->functor("at", {this->functor("suc", {this->var(0)}), this->var(1)}),
            {this->functor("at", {this->var(0), this->var(1)})},
            2),
        rule(
            this->functor("at", {this->var(0), this->functor("suc", {this->var(1)})}),
            {this->functor("at", {this->var(0), this->var(1)})},
            2),
    });
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {}), {this->functor("at", {four, four})}, 0),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    std::set<pud_global_node_id> unfoldable;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id offspring : change.offspring) {
            live.insert(offspring);
            if (offspring.module == 2 && !runtime.get_rule_information(offspring).body.empty())
                unfoldable.insert(offspring);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            unfoldable.erase(refuted);
        }
    }
    size_t steps = 0;
    for (; steps < 2000; ++steps) {
        if (unfoldable.empty())
            break;
        const pud_global_node_id chosen_leaf = *unfoldable.begin();
        std::optional<pud_body_goal_id> chosen_goal;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(chosen_leaf).body) {
            if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                chosen_goal = goal_id;
        }
        const pud_frontier_change change = runtime.unfold(chosen_leaf, *chosen_goal);
        live.erase(chosen_leaf);
        unfoldable.erase(chosen_leaf);
        for (pud_global_node_id child : change.offspring) {
            live.insert(child);
            if (child.module == 2 && !runtime.get_rule_information(child).body.empty())
                unfoldable.insert(child);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            unfoldable.erase(refuted);
        }
    }
    EXPECT_LT(steps, 2000u);
    std::set<pud_global_node_id> expected_unfoldable;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        if (!runtime.get_rule_information(leaf).body.empty())
            expected_unfoldable.insert(leaf);
    }
    EXPECT_EQ(unfoldable, expected_unfoldable);
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
    EXPECT_EQ(solved_live_leaves, 70u);
    EXPECT_EQ(answers, (std::set<const expr*>{this->functor("q", {})}));
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesLatticePathsFourByFourInEveryUnfoldOrder) {
    const expr* zero = this->atom("zero");
    const expr* four = zero;
    for (int peano_index = 0; peano_index < 4; ++peano_index)
        four = this->functor("suc", {four});
    const std::set<const expr*> expected{this->functor("q", {})};
    for (uint32_t seed = 0; seed < 20; ++seed) {
        std::mt19937 rng(seed);
        std::deque<pud_module> modules;
        modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
            rule(this->functor("at", {zero, zero}), {}, 0),
            rule(
                this->functor("at", {this->functor("suc", {this->var(0)}), this->var(1)}),
                {this->functor("at", {this->var(0), this->var(1)})},
                2),
            rule(
                this->functor("at", {this->var(0), this->functor("suc", {this->var(1)})}),
                {this->functor("at", {this->var(0), this->var(1)})},
                2),
        });
        modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
            rule(this->functor("q", {}), {this->functor("at", {four, four})}, 0),
        });
        TypeParam runtime(this->exprs);
        std::set<pud_global_node_id> live;
            std::set<pud_global_node_id> unfoldable;
        for (pud_module& module : modules) {
            install_shared_functor_names(module, this->functors);
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id offspring : change.offspring) {
                live.insert(offspring);
            if (offspring.module == 2 && !runtime.get_rule_information(offspring).body.empty())
                    unfoldable.insert(offspring);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                unfoldable.erase(refuted);
            }
        }
        size_t steps = 0;
        for (; steps < 2000; ++steps) {
            if (unfoldable.empty())
                break;
            const pud_global_node_id leaf = *std::next(unfoldable.begin(), rng() % unfoldable.size());
            std::vector<pud_body_goal_id> goal_ids;
            for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
                goal_ids.push_back(goal_id);
            const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
            const pud_frontier_change change = runtime.unfold(leaf, goal);
            live.erase(leaf);
            unfoldable.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
            live.insert(child);
            if (child.module == 2 && !runtime.get_rule_information(child).body.empty())
                unfoldable.insert(child);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                unfoldable.erase(refuted);
            }
        }
        EXPECT_LT(steps, 2000u);
        std::set<pud_global_node_id> expected_unfoldable;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            if (!runtime.get_rule_information(leaf).body.empty())
                expected_unfoldable.insert(leaf);
        }
        EXPECT_EQ(unfoldable, expected_unfoldable);
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
        EXPECT_EQ(live, reference_pud_frontier(modules));
        EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    }
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, CompositionsMinimalCaseOrderedCompositionsOfTwo) {
    const expr* zero = this->atom("zero");
    const expr* one = zero;
    for (int peano_index = 0; peano_index < 1; ++peano_index)
        one = this->functor("suc", {one});
    const expr* two = one;
    for (int peano_index = 0; peano_index < 1; ++peano_index)
        two = this->functor("suc", {two});
    const expr* nil = this->atom("nil");
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {zero}), {}, 0),
        rule(this->functor("compose", {zero, nil}), {}, 0),
        rule(
            this->functor("nat", {this->functor("suc", {this->var(0)})}),
            {this->functor("nat", {this->var(0)})},
            1),
        rule(
            this->functor("add", {zero, this->var(0), this->var(0)}),
            {this->functor("nat", {this->var(0)})},
            1),
        rule(
            this->functor("add", {this->functor("suc", {this->var(0)}), this->var(1), this->functor("suc", {this->var(2)})}),
            {this->functor("add", {this->var(0), this->var(1), this->var(2)})},
            3),
        rule(
            this->functor("compose", {this->var(0), this->functor("cons", {one, this->var(2)})}),
            {this->functor("add", {one, this->var(1), this->var(0)}), this->functor("compose", {this->var(1), this->var(2)})},
            3),
        rule(
            this->functor("compose", {this->var(0), this->functor("cons", {two, this->var(2)})}),
            {this->functor("add", {two, this->var(1), this->var(0)}), this->functor("compose", {this->var(1), this->var(2)})},
            3),
    });
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {this->var(0)}), {this->functor("compose", {two, this->var(0)})}, 1),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    std::set<pud_global_node_id> unfoldable;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id offspring : change.offspring) {
            live.insert(offspring);
            if (offspring.module == 2 && !runtime.get_rule_information(offspring).body.empty())
                unfoldable.insert(offspring);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            unfoldable.erase(refuted);
        }
    }
    size_t steps = 0;
    for (; steps < 2000; ++steps) {
        if (unfoldable.empty())
            break;
        const pud_global_node_id chosen_leaf = *unfoldable.begin();
        std::optional<pud_body_goal_id> chosen_goal;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(chosen_leaf).body) {
            if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                chosen_goal = goal_id;
        }
        const pud_frontier_change change = runtime.unfold(chosen_leaf, *chosen_goal);
        live.erase(chosen_leaf);
        unfoldable.erase(chosen_leaf);
        for (pud_global_node_id child : change.offspring) {
            live.insert(child);
            if (child.module == 2 && !runtime.get_rule_information(child).body.empty())
                unfoldable.insert(child);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            unfoldable.erase(refuted);
        }
    }
    EXPECT_LT(steps, 2000u);
    std::set<pud_global_node_id> expected_unfoldable;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        if (!runtime.get_rule_information(leaf).body.empty())
            expected_unfoldable.insert(leaf);
    }
    EXPECT_EQ(unfoldable, expected_unfoldable);
    std::set<const expr*> answers;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        const pud_unnormalized_rule_information leaf_info = runtime.get_rule_information(leaf);
        if (!leaf_info.body.empty())
            continue;
        answers.insert(reference_pud_normalize_with_bindings(leaf_info.bindings, this->exprs, leaf_info.head));
    }
    EXPECT_EQ(answers, (std::set<const expr*>{
        this->functor("q", {this->functor("cons", {two, nil})}),
        this->functor("q", {this->functor("cons", {one, this->functor("cons", {one, nil})})}),
    }));
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesOrderedCompositionsOfEight) {
    const expr* zero = this->atom("zero");
    const expr* one = zero;
    for (int peano_index = 0; peano_index < 1; ++peano_index)
        one = this->functor("suc", {one});
    const expr* two = one;
    for (int peano_index = 0; peano_index < 1; ++peano_index)
        two = this->functor("suc", {two});
    const expr* three = two;
    for (int peano_index = 0; peano_index < 1; ++peano_index)
        three = this->functor("suc", {three});
    const expr* four = three;
    for (int peano_index = 0; peano_index < 1; ++peano_index)
        four = this->functor("suc", {four});
    const expr* five = four;
    for (int peano_index = 0; peano_index < 1; ++peano_index)
        five = this->functor("suc", {five});
    const expr* six = five;
    for (int peano_index = 0; peano_index < 1; ++peano_index)
        six = this->functor("suc", {six});
    const expr* seven = six;
    for (int peano_index = 0; peano_index < 1; ++peano_index)
        seven = this->functor("suc", {seven});
    const expr* eight = seven;
    for (int peano_index = 0; peano_index < 1; ++peano_index)
        eight = this->functor("suc", {eight});
    const expr* nil = this->atom("nil");
    std::set<const expr*> expected;
    std::stack<std::pair<int, const expr*>> composition_stack;
    composition_stack.emplace(8, nil);
    while (!composition_stack.empty()) {
        const std::pair<int, const expr*> frame = composition_stack.top();
        composition_stack.pop();
        const int remaining = frame.first;
        const expr* list = frame.second;
        if (remaining == 0) {
            expected.insert(this->functor("q", {list}));
            continue;
        }
        for (int part = 1; part <= remaining; ++part) {
            const expr* part_term = zero;
            for (int peano_index = 0; peano_index < part; ++peano_index)
                part_term = this->functor("suc", {part_term});
            composition_stack.emplace(remaining - part, this->functor("cons", {part_term, list}));
        }
    }
    ASSERT_EQ(expected.size(), 128u);
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {zero}), {}, 0),
        rule(this->functor("compose", {zero, nil}), {}, 0),
        rule(
            this->functor("nat", {this->functor("suc", {this->var(0)})}),
            {this->functor("nat", {this->var(0)})},
            1),
        rule(
            this->functor("add", {zero, this->var(0), this->var(0)}),
            {this->functor("nat", {this->var(0)})},
            1),
        rule(
            this->functor("add", {this->functor("suc", {this->var(0)}), this->var(1), this->functor("suc", {this->var(2)})}),
            {this->functor("add", {this->var(0), this->var(1), this->var(2)})},
            3),
        rule(
            this->functor("compose", {this->var(0), this->functor("cons", {one, this->var(2)})}),
            {this->functor("add", {one, this->var(1), this->var(0)}), this->functor("compose", {this->var(1), this->var(2)})},
            3),
        rule(
            this->functor("compose", {this->var(0), this->functor("cons", {two, this->var(2)})}),
            {this->functor("add", {two, this->var(1), this->var(0)}), this->functor("compose", {this->var(1), this->var(2)})},
            3),
        rule(
            this->functor("compose", {this->var(0), this->functor("cons", {three, this->var(2)})}),
            {this->functor("add", {three, this->var(1), this->var(0)}), this->functor("compose", {this->var(1), this->var(2)})},
            3),
        rule(
            this->functor("compose", {this->var(0), this->functor("cons", {four, this->var(2)})}),
            {this->functor("add", {four, this->var(1), this->var(0)}), this->functor("compose", {this->var(1), this->var(2)})},
            3),
        rule(
            this->functor("compose", {this->var(0), this->functor("cons", {five, this->var(2)})}),
            {this->functor("add", {five, this->var(1), this->var(0)}), this->functor("compose", {this->var(1), this->var(2)})},
            3),
        rule(
            this->functor("compose", {this->var(0), this->functor("cons", {six, this->var(2)})}),
            {this->functor("add", {six, this->var(1), this->var(0)}), this->functor("compose", {this->var(1), this->var(2)})},
            3),
        rule(
            this->functor("compose", {this->var(0), this->functor("cons", {seven, this->var(2)})}),
            {this->functor("add", {seven, this->var(1), this->var(0)}), this->functor("compose", {this->var(1), this->var(2)})},
            3),
        rule(
            this->functor("compose", {this->var(0), this->functor("cons", {eight, this->var(2)})}),
            {this->functor("add", {eight, this->var(1), this->var(0)}), this->functor("compose", {this->var(1), this->var(2)})},
            3),
    });
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {this->var(0)}), {this->functor("compose", {eight, this->var(0)})}, 1),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    std::set<pud_global_node_id> unfoldable;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id offspring : change.offspring) {
            live.insert(offspring);
            if (offspring.module == 2 && !runtime.get_rule_information(offspring).body.empty())
                unfoldable.insert(offspring);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            unfoldable.erase(refuted);
        }
    }
    const uint32_t add_functor = this->functors.id("add");
    const uint32_t compose_functor = this->functors.id("compose");
    const uint32_t nat_functor = this->functors.id("nat");
    size_t steps = 0;
    // Finite program: unfold add before compose before nat (goal ids alone are not predicate order).
    for (; steps < 100000; ++steps) {
        if (unfoldable.empty())
            break;
        const pud_global_node_id chosen_leaf = *unfoldable.begin();
        std::optional<pud_body_goal_id> chosen_goal;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(chosen_leaf).body) {
            const expr::functor& goal_head = std::get<expr::functor>(goal->content);
            int goal_rank = 3;
            if (goal_head.id == add_functor)
                goal_rank = 0;
            else if (goal_head.id == compose_functor)
                goal_rank = 1;
            else if (goal_head.id == nat_functor)
                goal_rank = 2;
            if (!chosen_goal.has_value()) {
                chosen_goal = goal_id;
                continue;
            }
            const expr* chosen_expr = runtime.get_rule_information(chosen_leaf).body.at(*chosen_goal);
            const expr::functor& chosen_head = std::get<expr::functor>(chosen_expr->content);
            int chosen_rank = 3;
            if (chosen_head.id == add_functor)
                chosen_rank = 0;
            else if (chosen_head.id == compose_functor)
                chosen_rank = 1;
            else if (chosen_head.id == nat_functor)
                chosen_rank = 2;
            if (goal_rank < chosen_rank || (goal_rank == chosen_rank && goal_id < *chosen_goal))
                chosen_goal = goal_id;
        }
        const pud_frontier_change change = runtime.unfold(chosen_leaf, *chosen_goal);
        live.erase(chosen_leaf);
        unfoldable.erase(chosen_leaf);
        for (pud_global_node_id child : change.offspring) {
            live.insert(child);
            if (child.module == 2 && !runtime.get_rule_information(child).body.empty())
                unfoldable.insert(child);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            unfoldable.erase(refuted);
        }
    }
    EXPECT_LT(steps, 100000u);
    std::set<pud_global_node_id> expected_unfoldable;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        if (!runtime.get_rule_information(leaf).body.empty())
            expected_unfoldable.insert(leaf);
    }
    EXPECT_EQ(unfoldable, expected_unfoldable);
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
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesOrderedCompositionsOfEightInEveryUnfoldOrder) {
    const expr* zero = this->atom("zero");
    const expr* one = zero;
    for (int peano_index = 0; peano_index < 1; ++peano_index)
        one = this->functor("suc", {one});
    const expr* two = one;
    for (int peano_index = 0; peano_index < 1; ++peano_index)
        two = this->functor("suc", {two});
    const expr* three = two;
    for (int peano_index = 0; peano_index < 1; ++peano_index)
        three = this->functor("suc", {three});
    const expr* four = three;
    for (int peano_index = 0; peano_index < 1; ++peano_index)
        four = this->functor("suc", {four});
    const expr* five = four;
    for (int peano_index = 0; peano_index < 1; ++peano_index)
        five = this->functor("suc", {five});
    const expr* six = five;
    for (int peano_index = 0; peano_index < 1; ++peano_index)
        six = this->functor("suc", {six});
    const expr* seven = six;
    for (int peano_index = 0; peano_index < 1; ++peano_index)
        seven = this->functor("suc", {seven});
    const expr* eight = seven;
    for (int peano_index = 0; peano_index < 1; ++peano_index)
        eight = this->functor("suc", {eight});
    const expr* nil = this->atom("nil");
    std::set<const expr*> expected;
    std::stack<std::pair<int, const expr*>> composition_stack;
    composition_stack.emplace(8, nil);
    while (!composition_stack.empty()) {
        const std::pair<int, const expr*> frame = composition_stack.top();
        composition_stack.pop();
        const int remaining = frame.first;
        const expr* list = frame.second;
        if (remaining == 0) {
            expected.insert(this->functor("q", {list}));
            continue;
        }
        for (int part = 1; part <= remaining; ++part) {
            const expr* part_term = zero;
            for (int peano_index = 0; peano_index < part; ++peano_index)
                part_term = this->functor("suc", {part_term});
            composition_stack.emplace(remaining - part, this->functor("cons", {part_term, list}));
        }
    }
    ASSERT_EQ(expected.size(), 128u);
    for (uint32_t seed = 0; seed < 20; ++seed) {
        std::mt19937 rng(seed);
        std::deque<pud_module> modules;
        modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {zero}), {}, 0),
        rule(this->functor("compose", {zero, nil}), {}, 0),
        rule(
            this->functor("nat", {this->functor("suc", {this->var(0)})}),
            {this->functor("nat", {this->var(0)})},
            1),
        rule(
            this->functor("add", {zero, this->var(0), this->var(0)}),
            {this->functor("nat", {this->var(0)})},
            1),
        rule(
            this->functor("add", {this->functor("suc", {this->var(0)}), this->var(1), this->functor("suc", {this->var(2)})}),
            {this->functor("add", {this->var(0), this->var(1), this->var(2)})},
            3),
        rule(
            this->functor("compose", {this->var(0), this->functor("cons", {one, this->var(2)})}),
            {this->functor("add", {one, this->var(1), this->var(0)}), this->functor("compose", {this->var(1), this->var(2)})},
            3),
        rule(
            this->functor("compose", {this->var(0), this->functor("cons", {two, this->var(2)})}),
            {this->functor("add", {two, this->var(1), this->var(0)}), this->functor("compose", {this->var(1), this->var(2)})},
            3),
        rule(
            this->functor("compose", {this->var(0), this->functor("cons", {three, this->var(2)})}),
            {this->functor("add", {three, this->var(1), this->var(0)}), this->functor("compose", {this->var(1), this->var(2)})},
            3),
        rule(
            this->functor("compose", {this->var(0), this->functor("cons", {four, this->var(2)})}),
            {this->functor("add", {four, this->var(1), this->var(0)}), this->functor("compose", {this->var(1), this->var(2)})},
            3),
        rule(
            this->functor("compose", {this->var(0), this->functor("cons", {five, this->var(2)})}),
            {this->functor("add", {five, this->var(1), this->var(0)}), this->functor("compose", {this->var(1), this->var(2)})},
            3),
        rule(
            this->functor("compose", {this->var(0), this->functor("cons", {six, this->var(2)})}),
            {this->functor("add", {six, this->var(1), this->var(0)}), this->functor("compose", {this->var(1), this->var(2)})},
            3),
        rule(
            this->functor("compose", {this->var(0), this->functor("cons", {seven, this->var(2)})}),
            {this->functor("add", {seven, this->var(1), this->var(0)}), this->functor("compose", {this->var(1), this->var(2)})},
            3),
        rule(
            this->functor("compose", {this->var(0), this->functor("cons", {eight, this->var(2)})}),
            {this->functor("add", {eight, this->var(1), this->var(0)}), this->functor("compose", {this->var(1), this->var(2)})},
            3),
        });
        modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
            rule(this->functor("q", {this->var(0)}), {this->functor("compose", {eight, this->var(0)})}, 1),
        });
        TypeParam runtime(this->exprs);
        std::set<pud_global_node_id> live;
            std::set<pud_global_node_id> unfoldable;
        for (pud_module& module : modules) {
            install_shared_functor_names(module, this->functors);
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id offspring : change.offspring) {
                live.insert(offspring);
            if (offspring.module == 2 && !runtime.get_rule_information(offspring).body.empty())
                    unfoldable.insert(offspring);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                unfoldable.erase(refuted);
            }
        }
        const uint32_t add_functor = this->functors.id("add");
        const uint32_t compose_functor = this->functors.id("compose");
        const uint32_t nat_functor = this->functors.id("nat");
        size_t steps = 0;
        for (; steps < 100000; ++steps) {
            if (unfoldable.empty())
                break;
            const pud_global_node_id leaf = *std::next(unfoldable.begin(), rng() % unfoldable.size());
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == leaf.module)
                    leaf_module = &module;
            }
            std::optional<pud_body_goal_id> chosen_goal;
            for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body) {
                const expr::functor& goal_head = std::get<expr::functor>(goal->content);
                int goal_rank = 3;
                if (goal_head.id == add_functor)
                    goal_rank = 0;
                else if (goal_head.id == compose_functor)
                    goal_rank = 1;
                else if (goal_head.id == nat_functor)
                    goal_rank = 2;
                if (!chosen_goal.has_value()) {
                    chosen_goal = goal_id;
                    continue;
                }
                const expr* chosen_expr = runtime.get_rule_information(leaf).body.at(*chosen_goal);
                const expr::functor& chosen_head = std::get<expr::functor>(chosen_expr->content);
                int chosen_rank = 3;
                if (chosen_head.id == add_functor)
                    chosen_rank = 0;
                else if (chosen_head.id == compose_functor)
                    chosen_rank = 1;
                else if (chosen_head.id == nat_functor)
                    chosen_rank = 2;
                if (goal_rank < chosen_rank || (goal_rank == chosen_rank && goal_id < *chosen_goal))
                    chosen_goal = goal_id;
            }
            const pud_body_goal_id goal = *chosen_goal;
            const pud_frontier_change change = runtime.unfold(leaf, goal);
            live.erase(leaf);
            unfoldable.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
            live.insert(child);
            if (child.module == 2 && !runtime.get_rule_information(child).body.empty())
                unfoldable.insert(child);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                unfoldable.erase(refuted);
            }
        }
        EXPECT_LT(steps, 100000u);
        std::set<pud_global_node_id> expected_unfoldable;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            if (!runtime.get_rule_information(leaf).body.empty())
                expected_unfoldable.insert(leaf);
        }
        EXPECT_EQ(unfoldable, expected_unfoldable);
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
        EXPECT_EQ(live, reference_pud_frontier(modules));
        EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    }
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesBinaryStringsNoConsecutiveOnesLengthTen) {
    const expr* zero = this->atom("zero");
    const expr* nil = this->atom("nil");
    const expr* b0 = this->atom("b0");
    const expr* b1 = this->atom("b1");
    const expr* ten = zero;
    for (int peano_index = 0; peano_index < 10; ++peano_index)
        ten = this->functor("suc", {ten});
    std::set<const expr*> expected;
    struct string_frame {
        int remaining;
        bool prev_one;
        const expr* list;
    };
    std::stack<string_frame> string_stack;
    string_stack.push({10, false, nil});
    while (!string_stack.empty()) {
        const string_frame frame = string_stack.top();
        string_stack.pop();
        if (frame.remaining == 0) {
            expected.insert(this->functor("q", {frame.list}));
            continue;
        }
        string_stack.push({frame.remaining - 1, false, this->functor("cons", {b0, frame.list})});
        if (!frame.prev_one)
            string_stack.push({frame.remaining - 1, true, this->functor("cons", {b1, frame.list})});
    }
    ASSERT_EQ(expected.size(), 144u);
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("good", {nil}), {}, 0),
        rule(this->functor("len", {nil, zero}), {}, 0),
        rule(
            this->functor("good", {this->functor("cons", {b0, this->var(0)})}),
            {this->functor("good", {this->var(0)})},
            1),
        rule(
            this->functor("good", {this->functor("cons", {b1, this->functor("cons", {b0, this->var(0)})})}),
            {this->functor("good", {this->var(0)})},
            1),
        rule(this->functor("good", {this->functor("cons", {b1, nil})}), {}, 0),
        rule(
            this->functor("len", {this->functor("cons", {this->var(0), this->var(1)}), this->functor("suc", {this->var(3)})}),
            {this->functor("len", {this->var(1), this->var(3)})},
            4),    });
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(
            this->functor("q", {this->var(0)}),
            {this->functor("good", {this->var(0)}), this->functor("len", {this->var(0), ten})},
            1),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    std::set<pud_global_node_id> unfoldable;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id offspring : change.offspring) {
            live.insert(offspring);
            if (offspring.module == 2 && !runtime.get_rule_information(offspring).body.empty())
                unfoldable.insert(offspring);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            unfoldable.erase(refuted);
        }
    }
    size_t steps = 0;
    for (; steps < 50000; ++steps) {
        if (unfoldable.empty())
            break;
        const pud_global_node_id chosen_leaf = *unfoldable.begin();
        std::optional<pud_body_goal_id> chosen_goal;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(chosen_leaf).body) {
            if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                chosen_goal = goal_id;
        }
        const pud_frontier_change change = runtime.unfold(chosen_leaf, *chosen_goal);
        live.erase(chosen_leaf);
        unfoldable.erase(chosen_leaf);
        for (pud_global_node_id child : change.offspring) {
            live.insert(child);
            if (child.module == 2 && !runtime.get_rule_information(child).body.empty())
                unfoldable.insert(child);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            unfoldable.erase(refuted);
        }
    }
    EXPECT_LT(steps, 50000u);
    std::set<pud_global_node_id> expected_unfoldable;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        if (!runtime.get_rule_information(leaf).body.empty())
            expected_unfoldable.insert(leaf);
    }
    EXPECT_EQ(unfoldable, expected_unfoldable);
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
    EXPECT_EQ(answers, expected);
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesBinaryStringsNoConsecutiveOnesLengthTenInEveryUnfoldOrder) {
    // Random goal order on len/good leaves can revisit the same partial strings forever; smallest goal id
    // matches the deterministic drive and still exhausts this finite program.
    const expr* zero = this->atom("zero");
    const expr* nil = this->atom("nil");
    const expr* b0 = this->atom("b0");
    const expr* b1 = this->atom("b1");
    const expr* ten = zero;
    for (int peano_index = 0; peano_index < 10; ++peano_index)
        ten = this->functor("suc", {ten});
    std::set<const expr*> expected;
    struct string_frame {
        int remaining;
        bool prev_one;
        const expr* list;
    };
    std::stack<string_frame> string_stack;
    string_stack.push({10, false, nil});
    while (!string_stack.empty()) {
        const string_frame frame = string_stack.top();
        string_stack.pop();
        if (frame.remaining == 0) {
            expected.insert(this->functor("q", {frame.list}));
            continue;
        }
        string_stack.push({frame.remaining - 1, false, this->functor("cons", {b0, frame.list})});
        if (!frame.prev_one)
            string_stack.push({frame.remaining - 1, true, this->functor("cons", {b1, frame.list})});
    }
    ASSERT_EQ(expected.size(), 144u);
    for (uint32_t seed = 0; seed < 20; ++seed) {
        std::mt19937 rng(seed);
        std::deque<pud_module> modules;
        modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("good", {nil}), {}, 0),
        rule(this->functor("len", {nil, zero}), {}, 0),
        rule(
            this->functor("good", {this->functor("cons", {b0, this->var(0)})}),
            {this->functor("good", {this->var(0)})},
            1),
        rule(
            this->functor("good", {this->functor("cons", {b1, this->functor("cons", {b0, this->var(0)})})}),
            {this->functor("good", {this->var(0)})},
            1),
        rule(this->functor("good", {this->functor("cons", {b1, nil})}), {}, 0),
        rule(
            this->functor("len", {this->functor("cons", {this->var(0), this->var(1)}), this->functor("suc", {this->var(3)})}),
            {this->functor("len", {this->var(1), this->var(3)})},
            4),        });
        modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
            rule(
                this->functor("q", {this->var(0)}),
                {this->functor("good", {this->var(0)}), this->functor("len", {this->var(0), ten})},
                1),
        });
        TypeParam runtime(this->exprs);
        std::set<pud_global_node_id> live;
            std::set<pud_global_node_id> unfoldable;
        for (pud_module& module : modules) {
            install_shared_functor_names(module, this->functors);
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id offspring : change.offspring) {
                live.insert(offspring);
            if (offspring.module == 2 && !runtime.get_rule_information(offspring).body.empty())
                    unfoldable.insert(offspring);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                unfoldable.erase(refuted);
            }
        }
        size_t steps = 0;
        for (; steps < 50000; ++steps) {
            if (unfoldable.empty())
                break;
            const pud_global_node_id leaf = *std::next(unfoldable.begin(), rng() % unfoldable.size());
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == leaf.module)
                    leaf_module = &module;
            }
            std::optional<pud_body_goal_id> chosen_goal;
            for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body) {
                if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                    chosen_goal = goal_id;
            }
            const pud_body_goal_id goal = *chosen_goal;
            const pud_frontier_change change = runtime.unfold(leaf, goal);
            live.erase(leaf);
            unfoldable.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
            live.insert(child);
            if (child.module == 2 && !runtime.get_rule_information(child).body.empty())
                unfoldable.insert(child);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                unfoldable.erase(refuted);
            }
        }
        EXPECT_LT(steps, 50000u);
        std::set<pud_global_node_id> expected_unfoldable;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            if (!runtime.get_rule_information(leaf).body.empty())
                expected_unfoldable.insert(leaf);
        }
        EXPECT_EQ(unfoldable, expected_unfoldable);
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
        EXPECT_EQ(live, reference_pud_frontier(modules));
        EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    }
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesStaircasePathsOneOrTwoSummingToTen) {
    const expr* zero = this->atom("zero");
    const expr* one = this->functor("suc", {zero});
    const expr* two = this->functor("suc", {one});
    const expr* nil = this->atom("nil");
    const expr* ten = zero;
    for (int peano_index = 0; peano_index < 10; ++peano_index)
        ten = this->functor("suc", {ten});
    std::set<const expr*> expected;
    struct stair_frame {
        int remaining;
        const expr* list;
    };
    std::stack<stair_frame> stair_stack;
    stair_stack.push({10, nil});
    while (!stair_stack.empty()) {
        const stair_frame frame = stair_stack.top();
        stair_stack.pop();
        if (frame.remaining == 0) {
            expected.insert(this->functor("q", {frame.list}));
            continue;
        }
        if (frame.remaining >= 1)
            stair_stack.push({frame.remaining - 1, this->functor("cons", {one, frame.list})});
        if (frame.remaining >= 2)
            stair_stack.push({frame.remaining - 2, this->functor("cons", {two, frame.list})});
    }
    ASSERT_EQ(expected.size(), 89u);
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {zero}), {}, 0),
        rule(this->functor("steps", {zero, nil}), {}, 0),
        rule(
            this->functor("nat", {this->functor("suc", {this->var(0)})}),
            {this->functor("nat", {this->var(0)})},
            1),
        rule(
            this->functor("add", {zero, this->var(0), this->var(0)}),
            {this->functor("nat", {this->var(0)})},
            1),
        rule(
            this->functor("add", {this->functor("suc", {this->var(0)}), this->var(1), this->functor("suc", {this->var(2)})}),
            {this->functor("add", {this->var(0), this->var(1), this->var(2)})},
            3),
        rule(
            this->functor("steps", {this->var(0), this->functor("cons", {one, this->var(2)})}),
            {this->functor("add", {one, this->var(1), this->var(0)}), this->functor("steps", {this->var(1), this->var(2)})},
            3),
        rule(
            this->functor("steps", {this->var(0), this->functor("cons", {two, this->var(2)})}),
            {this->functor("add", {two, this->var(1), this->var(0)}), this->functor("steps", {this->var(1), this->var(2)})},
            3),    });
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {this->var(0)}), {this->functor("steps", {ten, this->var(0)})}, 1),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    std::set<pud_global_node_id> unfoldable;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id offspring : change.offspring) {
            live.insert(offspring);
            if (offspring.module == 2 && !runtime.get_rule_information(offspring).body.empty())
                unfoldable.insert(offspring);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            unfoldable.erase(refuted);
        }
    }
    size_t steps = 0;
    for (; steps < 50000; ++steps) {
        if (unfoldable.empty())
            break;
        const pud_global_node_id chosen_leaf = *unfoldable.begin();
        std::optional<pud_body_goal_id> chosen_goal;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(chosen_leaf).body) {
            if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                chosen_goal = goal_id;
        }
        const pud_frontier_change change = runtime.unfold(chosen_leaf, *chosen_goal);
        live.erase(chosen_leaf);
        unfoldable.erase(chosen_leaf);
        for (pud_global_node_id child : change.offspring) {
            live.insert(child);
            if (child.module == 2 && !runtime.get_rule_information(child).body.empty())
                unfoldable.insert(child);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            unfoldable.erase(refuted);
        }
    }
    EXPECT_LT(steps, 50000u);
    std::set<pud_global_node_id> expected_unfoldable;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        if (!runtime.get_rule_information(leaf).body.empty())
            expected_unfoldable.insert(leaf);
    }
    EXPECT_EQ(unfoldable, expected_unfoldable);
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
    EXPECT_EQ(answers, expected);
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesStaircasePathsOneOrTwoSummingToTenInEveryUnfoldOrder) {
    // Random goal order on add/steps leaves can extend Peano add chains without finishing steps(ten,_);
    // smallest goal id (add before steps in each clause) terminates like the deterministic drive.
    const expr* zero = this->atom("zero");
    const expr* one = this->functor("suc", {zero});
    const expr* two = this->functor("suc", {one});
    const expr* nil = this->atom("nil");
    const expr* ten = zero;
    for (int peano_index = 0; peano_index < 10; ++peano_index)
        ten = this->functor("suc", {ten});
    std::set<const expr*> expected;
    struct stair_frame {
        int remaining;
        const expr* list;
    };
    std::stack<stair_frame> stair_stack;
    stair_stack.push({10, nil});
    while (!stair_stack.empty()) {
        const stair_frame frame = stair_stack.top();
        stair_stack.pop();
        if (frame.remaining == 0) {
            expected.insert(this->functor("q", {frame.list}));
            continue;
        }
        if (frame.remaining >= 1)
            stair_stack.push({frame.remaining - 1, this->functor("cons", {one, frame.list})});
        if (frame.remaining >= 2)
            stair_stack.push({frame.remaining - 2, this->functor("cons", {two, frame.list})});
    }
    ASSERT_EQ(expected.size(), 89u);
    for (uint32_t seed = 0; seed < 20; ++seed) {
        std::mt19937 rng(seed);
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {zero}), {}, 0),
        rule(this->functor("steps", {zero, nil}), {}, 0),
        rule(
            this->functor("nat", {this->functor("suc", {this->var(0)})}),
            {this->functor("nat", {this->var(0)})},
            1),
        rule(
            this->functor("add", {zero, this->var(0), this->var(0)}),
            {this->functor("nat", {this->var(0)})},
            1),
        rule(
            this->functor("add", {this->functor("suc", {this->var(0)}), this->var(1), this->functor("suc", {this->var(2)})}),
            {this->functor("add", {this->var(0), this->var(1), this->var(2)})},
            3),
        rule(
            this->functor("steps", {this->var(0), this->functor("cons", {one, this->var(2)})}),
            {this->functor("add", {one, this->var(1), this->var(0)}), this->functor("steps", {this->var(1), this->var(2)})},
            3),
        rule(
            this->functor("steps", {this->var(0), this->functor("cons", {two, this->var(2)})}),
            {this->functor("add", {two, this->var(1), this->var(0)}), this->functor("steps", {this->var(1), this->var(2)})},
            3),    });
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {this->var(0)}), {this->functor("steps", {ten, this->var(0)})}, 1),
    });
        TypeParam runtime(this->exprs);
        std::set<pud_global_node_id> live;
            std::set<pud_global_node_id> unfoldable;
        for (pud_module& module : modules) {
            install_shared_functor_names(module, this->functors);
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id offspring : change.offspring) {
                live.insert(offspring);
            if (offspring.module == 2 && !runtime.get_rule_information(offspring).body.empty())
                    unfoldable.insert(offspring);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                unfoldable.erase(refuted);
            }
        }
        size_t steps = 0;
        for (; steps < 50000; ++steps) {
            if (unfoldable.empty())
                break;
            const pud_global_node_id leaf = *std::next(unfoldable.begin(), rng() % unfoldable.size());
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == leaf.module)
                    leaf_module = &module;
            }
            std::optional<pud_body_goal_id> chosen_goal;
            for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body) {
                if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                    chosen_goal = goal_id;
            }
            const pud_body_goal_id goal = *chosen_goal;
            const pud_frontier_change change = runtime.unfold(leaf, goal);
            live.erase(leaf);
            unfoldable.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
            live.insert(child);
            if (child.module == 2 && !runtime.get_rule_information(child).body.empty())
                unfoldable.insert(child);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                unfoldable.erase(refuted);
            }
        }
        EXPECT_LT(steps, 50000u);
        std::set<pud_global_node_id> expected_unfoldable;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            if (!runtime.get_rule_information(leaf).body.empty())
                expected_unfoldable.insert(leaf);
        }
        EXPECT_EQ(unfoldable, expected_unfoldable);
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
        EXPECT_EQ(live, reference_pud_frontier(modules));
        EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    }
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesFibIndexPairsWithSumBelowThirty) {
    const expr* zero = this->atom("zero");
    const expr* one = this->functor("suc", {zero});
    const expr* three = zero;
    for (int peano_index = 0; peano_index < 3; ++peano_index)
        three = this->functor("suc", {three});
    std::set<const expr*> expected;
    for (int i = 0; i <= 3; ++i) {
        for (int j = 0; j <= 3; ++j) {
            int fi = i <= 1 ? i : 0;
            int fj = j <= 1 ? j : 0;
            if (i > 1) {
                int a = 0;
                int b = 1;
                for (int k = 2; k <= i; ++k) {
                    const int c = a + b;
                    a = b;
                    b = c;
                }
                fi = b;
            }
            if (j > 1) {
                int a = 0;
                int b = 1;
                for (int k = 2; k <= j; ++k) {
                    const int c = a + b;
                    a = b;
                    b = c;
                }
                fj = b;
            }
            if (fi + fj >= 3)
                continue;
            const expr* i_term = zero;
            for (int peano_index = 0; peano_index < i; ++peano_index)
                i_term = this->functor("suc", {i_term});
            const expr* j_term = zero;
            for (int peano_index = 0; peano_index < j; ++peano_index)
                j_term = this->functor("suc", {j_term});
            expected.insert(this->functor("q", {i_term, j_term}));
        }
    }
    ASSERT_EQ(expected.size(), 11u);
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {zero}), {}, 0),
        rule(this->functor("fib", {zero, zero}), {}, 0),
        rule(this->functor("fib", {one, one}), {}, 0),
        rule(
            this->functor("nat", {this->functor("suc", {this->var(0)})}),
            {this->functor("nat", {this->var(0)})},
            1),
        rule(
            this->functor("add", {zero, this->var(0), this->var(0)}),
            {this->functor("nat", {this->var(0)})},
            1),
        rule(
            this->functor("add", {this->functor("suc", {this->var(0)}), this->var(1), this->functor("suc", {this->var(2)})}),
            {this->functor("add", {this->var(0), this->var(1), this->var(2)})},
            3),
        rule(
            this->functor("fib", {this->functor("suc", {this->functor("suc", {this->var(0)})}), this->var(3)}),
            {this->functor("fib", {this->var(0), this->var(1)}),
                this->functor("fib", {this->functor("suc", {this->var(0)}), this->var(2)}),
                this->functor("add", {this->var(1), this->var(2), this->var(3)})},
            4),
        rule(
            this->functor("lt", {zero, this->functor("suc", {this->var(0)})}),
            {this->functor("nat", {this->var(0)})},
            1),
        rule(
            this->functor("lt", {this->functor("suc", {this->var(0)}), this->functor("suc", {this->var(1)})}),
            {this->functor("lt", {this->var(0), this->var(1)})},
            2),    });
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(
            this->functor("q", {this->var(0), this->var(1)}),
            {this->functor("nat", {this->var(0)}),
                this->functor("nat", {this->var(1)}),
                this->functor("fib", {this->var(0), this->var(2)}),
                this->functor("fib", {this->var(1), this->var(3)}),
                this->functor("add", {this->var(2), this->var(3), this->var(4)}),
                this->functor("lt", {this->var(4), three})},
            5),
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    std::set<pud_global_node_id> unfoldable;
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id offspring : change.offspring) {
            live.insert(offspring);
            if (offspring.module == 2 && !runtime.get_rule_information(offspring).body.empty())
                unfoldable.insert(offspring);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            unfoldable.erase(refuted);
        }
    }
    // Derivation never exhausts: fib(N,V) with V unbound admits infinitely many indices N for any value bound.
    // kMaxSteps is sufficient: all 11 expected pairs first appear by step 1599 and the set stays
    // identical through 4x this cap (measured in scratch).
    static constexpr size_t kMaxSteps = 20000;
    size_t steps = 0;
    for (; steps < kMaxSteps; ++steps) {
        if (unfoldable.empty())
            break;
        const pud_global_node_id chosen_leaf = *unfoldable.begin();
        std::optional<pud_body_goal_id> chosen_goal;
        for (const auto& [goal_id, goal] : runtime.get_rule_information(chosen_leaf).body) {
            if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                chosen_goal = goal_id;
        }
        const pud_frontier_change change = runtime.unfold(chosen_leaf, *chosen_goal);
        live.erase(chosen_leaf);
        unfoldable.erase(chosen_leaf);
        for (pud_global_node_id child : change.offspring) {
            live.insert(child);
            if (child.module == 2 && !runtime.get_rule_information(child).body.empty())
                unfoldable.insert(child);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            unfoldable.erase(refuted);
        }
    }
    std::set<pud_global_node_id> expected_unfoldable;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        if (!runtime.get_rule_information(leaf).body.empty())
            expected_unfoldable.insert(leaf);
    }
    EXPECT_EQ(unfoldable, expected_unfoldable);
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
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuiteIntegrationTest, EnumeratesFibIndexPairsWithSumBelowThirtyInEveryUnfoldOrder) {
    const expr* zero = this->atom("zero");
    const expr* one = this->functor("suc", {zero});
    const expr* three = zero;
    for (int peano_index = 0; peano_index < 3; ++peano_index)
        three = this->functor("suc", {three});
    std::set<const expr*> expected;
    for (int i = 0; i <= 3; ++i) {
        for (int j = 0; j <= 3; ++j) {
            int fi = i <= 1 ? i : 0;
            int fj = j <= 1 ? j : 0;
            if (i > 1) {
                int a = 0;
                int b = 1;
                for (int k = 2; k <= i; ++k) {
                    const int c = a + b;
                    a = b;
                    b = c;
                }
                fi = b;
            }
            if (j > 1) {
                int a = 0;
                int b = 1;
                for (int k = 2; k <= j; ++k) {
                    const int c = a + b;
                    a = b;
                    b = c;
                }
                fj = b;
            }
            if (fi + fj >= 3)
                continue;
            const expr* i_term = zero;
            for (int peano_index = 0; peano_index < i; ++peano_index)
                i_term = this->functor("suc", {i_term});
            const expr* j_term = zero;
            for (int peano_index = 0; peano_index < j; ++peano_index)
                j_term = this->functor("suc", {j_term});
            expected.insert(this->functor("q", {i_term, j_term}));
        }
    }
    ASSERT_EQ(expected.size(), 11u);
    // Per-seed exhaustion is impossible (same infinite fib/nat tree as the deterministic test). Each seed
    // checks soundness; the union across seeds must still cover every expected pair.
    std::set<const expr*> found_across_seeds;
    static constexpr size_t kMaxStepsPerSeed = 3000;
    for (uint32_t seed = 0; seed < 20; ++seed) {
        std::mt19937 rng(seed);
        std::deque<pud_module> modules;
        modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(this->functor("nat", {zero}), {}, 0),
        rule(this->functor("fib", {zero, zero}), {}, 0),
        rule(this->functor("fib", {one, one}), {}, 0),
        rule(
            this->functor("nat", {this->functor("suc", {this->var(0)})}),
            {this->functor("nat", {this->var(0)})},
            1),
        rule(
            this->functor("add", {zero, this->var(0), this->var(0)}),
            {this->functor("nat", {this->var(0)})},
            1),
        rule(
            this->functor("add", {this->functor("suc", {this->var(0)}), this->var(1), this->functor("suc", {this->var(2)})}),
            {this->functor("add", {this->var(0), this->var(1), this->var(2)})},
            3),
        rule(
            this->functor("fib", {this->functor("suc", {this->functor("suc", {this->var(0)})}), this->var(3)}),
            {this->functor("fib", {this->var(0), this->var(1)}),
                this->functor("fib", {this->functor("suc", {this->var(0)}), this->var(2)}),
                this->functor("add", {this->var(1), this->var(2), this->var(3)})},
            4),
        rule(
            this->functor("lt", {zero, this->functor("suc", {this->var(0)})}),
            {this->functor("nat", {this->var(0)})},
            1),
        rule(
            this->functor("lt", {this->functor("suc", {this->var(0)}), this->functor("suc", {this->var(1)})}),
            {this->functor("lt", {this->var(0), this->var(1)})},
            2),    });
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(
            this->functor("q", {this->var(0), this->var(1)}),
            {this->functor("nat", {this->var(0)}),
                this->functor("nat", {this->var(1)}),
                this->functor("fib", {this->var(0), this->var(2)}),
                this->functor("fib", {this->var(1), this->var(3)}),
                this->functor("add", {this->var(2), this->var(3), this->var(4)}),
                this->functor("lt", {this->var(4), three})},
            5),
    });
        TypeParam runtime(this->exprs);
        std::set<pud_global_node_id> live;
            std::set<pud_global_node_id> unfoldable;
        for (pud_module& module : modules) {
            install_shared_functor_names(module, this->functors);
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id offspring : change.offspring) {
                live.insert(offspring);
            if (offspring.module == 2 && !runtime.get_rule_information(offspring).body.empty())
                    unfoldable.insert(offspring);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                unfoldable.erase(refuted);
            }
        }
        size_t steps = 0;
        for (; steps < kMaxStepsPerSeed; ++steps) {
            if (unfoldable.empty())
                break;
            const pud_global_node_id leaf = *std::next(unfoldable.begin(), rng() % unfoldable.size());
            std::vector<pud_body_goal_id> goal_ids;
            for (const auto& [goal_id, goal] : runtime.get_rule_information(leaf).body)
                goal_ids.push_back(goal_id);
            const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
            const pud_frontier_change change = runtime.unfold(leaf, goal);
            live.erase(leaf);
            unfoldable.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
            live.insert(child);
            if (child.module == 2 && !runtime.get_rule_information(child).body.empty())
                unfoldable.insert(child);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                unfoldable.erase(refuted);
            }
        }
        std::set<pud_global_node_id> expected_unfoldable;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            if (!runtime.get_rule_information(leaf).body.empty())
                expected_unfoldable.insert(leaf);
        }
        EXPECT_EQ(unfoldable, expected_unfoldable);
        std::set<const expr*> answers;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            const pud_unnormalized_rule_information leaf_info = runtime.get_rule_information(leaf);
            if (!leaf_info.body.empty())
                continue;
            answers.insert(reference_pud_normalize_with_bindings(leaf_info.bindings, this->exprs, leaf_info.head));
        }
        for (const expr* answer : answers) {
            EXPECT_TRUE(expected.contains(answer))
                << "seed " << seed << " produced an answer not in the expected set";
            found_across_seeds.insert(answer);
        }
    }
    EXPECT_EQ(found_across_seeds, expected);
}
