#include <cstdint>
#include <cstdio>
#include <deque>
#include <optional>
#include <random>
#include <set>
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
struct PudAnswerSetsRuntimeSuite1IntegrationTest : public ::testing::Test {
    test_functors functors;
    expr_pool exprs;

    const expr* var(uint32_t index) { return exprs.make_var(index); }
    const expr* atom(const char* name) { return exprs.make_functor(functors.id(name), {}); }
    const expr* functor(const char* name, std::vector<const expr*> args) {
        return exprs.make_functor(functors.id(name), std::move(args));
    }
};

using pud_answer_sets_runtime_suite_1_runtimes_t = ::testing::Types<pud_runtime<pud_module, expr_pool>>;
TYPED_TEST_SUITE(PudAnswerSetsRuntimeSuite1IntegrationTest, pud_answer_sets_runtime_suite_1_runtimes_t);

TYPED_TEST(PudAnswerSetsRuntimeSuite1IntegrationTest, FindsSingleUnitSolution) {
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

TYPED_TEST(PudAnswerSetsRuntimeSuite1IntegrationTest, FindsSingleUnitSolutionInEveryUnfoldOrder) {
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

TYPED_TEST(PudAnswerSetsRuntimeSuite1IntegrationTest, RefutesWhenNoCandidates) {
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{});
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {}), {this->functor("f", {})}, 0)
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
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

TYPED_TEST(PudAnswerSetsRuntimeSuite1IntegrationTest, RefutesWhenNoCandidatesInEveryUnfoldOrder) {
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

TYPED_TEST(PudAnswerSetsRuntimeSuite1IntegrationTest, FindsClauseDerivedUnitSolution) {
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

TYPED_TEST(PudAnswerSetsRuntimeSuite1IntegrationTest, FindsClauseDerivedUnitSolutionInEveryUnfoldOrder) {
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

TYPED_TEST(PudAnswerSetsRuntimeSuite1IntegrationTest, FindsSolutionWithCorrectBindings) {
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

TYPED_TEST(PudAnswerSetsRuntimeSuite1IntegrationTest, FindsSolutionWithCorrectBindingsInEveryUnfoldOrder) {
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

TYPED_TEST(PudAnswerSetsRuntimeSuite1IntegrationTest, FindsClauseBodyBindingSolution) {
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

TYPED_TEST(PudAnswerSetsRuntimeSuite1IntegrationTest, FindsClauseBodyBindingSolutionInEveryUnfoldOrder) {
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

TYPED_TEST(PudAnswerSetsRuntimeSuite1IntegrationTest, RefutesAfterCdclOnUnsatClauseBranches) {
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

TYPED_TEST(PudAnswerSetsRuntimeSuite1IntegrationTest, RefutesAfterCdclOnUnsatClauseBranchesInEveryUnfoldOrder) {
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

TYPED_TEST(PudAnswerSetsRuntimeSuite1IntegrationTest, EnumeratesTwoGroundChoiceSolutions) {
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

TYPED_TEST(PudAnswerSetsRuntimeSuite1IntegrationTest, EnumeratesTwoGroundChoiceSolutionsInEveryUnfoldOrder) {
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

TYPED_TEST(PudAnswerSetsRuntimeSuite1IntegrationTest, RefutesAfterEnumeratingAllGroundBranches) {
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

TYPED_TEST(PudAnswerSetsRuntimeSuite1IntegrationTest, RefutesAfterEnumeratingAllGroundBranchesInEveryUnfoldOrder) {
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

TYPED_TEST(PudAnswerSetsRuntimeSuite1IntegrationTest, EnumeratesTwoVarChoiceSolutions) {
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

TYPED_TEST(PudAnswerSetsRuntimeSuite1IntegrationTest, EnumeratesTwoVarChoiceSolutionsInEveryUnfoldOrder) {
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

TYPED_TEST(PudAnswerSetsRuntimeSuite1IntegrationTest, RefutesAfterEnumeratingAllVarBranches) {
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

TYPED_TEST(PudAnswerSetsRuntimeSuite1IntegrationTest, RefutesAfterEnumeratingAllVarBranchesInEveryUnfoldOrder) {
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

TYPED_TEST(PudAnswerSetsRuntimeSuite1IntegrationTest, EnumeratesTwoGoalSharedVarSolutions) {
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

TYPED_TEST(PudAnswerSetsRuntimeSuite1IntegrationTest, EnumeratesTwoGoalSharedVarSolutionsInEveryUnfoldOrder) {
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

TYPED_TEST(PudAnswerSetsRuntimeSuite1IntegrationTest, EnumeratesFourVarBindingSolutions) {
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

TYPED_TEST(PudAnswerSetsRuntimeSuite1IntegrationTest, EnumeratesFourVarBindingSolutionsInEveryUnfoldOrder) {
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

TYPED_TEST(PudAnswerSetsRuntimeSuite1IntegrationTest, EnumeratesFourClauseBodyFactChoices) {
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

TYPED_TEST(PudAnswerSetsRuntimeSuite1IntegrationTest, EnumeratesFourClauseBodyFactChoicesInEveryUnfoldOrder) {
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

TYPED_TEST(PudAnswerSetsRuntimeSuite1IntegrationTest, FindsUniqueSharedVarConjunctionThenRefutes) {
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

TYPED_TEST(PudAnswerSetsRuntimeSuite1IntegrationTest, FindsUniqueSharedVarConjunctionThenRefutesInEveryUnfoldOrder) {
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

TYPED_TEST(PudAnswerSetsRuntimeSuite1IntegrationTest, ConflictedTickNotSolved) {
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{});
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(this->functor("q", {}), {this->functor("f", {})}, 0)
    });
    TypeParam runtime(this->exprs);
    std::set<pud_global_node_id> live;
    for (pud_module& module : modules) {
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

TYPED_TEST(PudAnswerSetsRuntimeSuite1IntegrationTest, ConflictedTickNotSolvedInEveryUnfoldOrder) {
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

TYPED_TEST(PudAnswerSetsRuntimeSuite1IntegrationTest, EnumeratesTwoParentBindingsForAlice) {
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

TYPED_TEST(PudAnswerSetsRuntimeSuite1IntegrationTest, EnumeratesTwoParentBindingsForAliceInEveryUnfoldOrder) {
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

TYPED_TEST(PudAnswerSetsRuntimeSuite1IntegrationTest, EnumeratesPeanoLessThanSeven) {
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

TYPED_TEST(PudAnswerSetsRuntimeSuite1IntegrationTest, EnumeratesPeanoLessThanSevenInEveryUnfoldOrder) {

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

TYPED_TEST(PudAnswerSetsRuntimeSuite1IntegrationTest, EnumeratesSatPAndQOrR) {
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

TYPED_TEST(PudAnswerSetsRuntimeSuite1IntegrationTest, EnumeratesSatPAndQOrRInEveryUnfoldOrder) {
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

TYPED_TEST(PudAnswerSetsRuntimeSuite1IntegrationTest, EnumeratesTwoSatAssignmentsForImpliesQ) {
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

TYPED_TEST(PudAnswerSetsRuntimeSuite1IntegrationTest, EnumeratesTwoSatAssignmentsForImpliesQInEveryUnfoldOrder) {
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

