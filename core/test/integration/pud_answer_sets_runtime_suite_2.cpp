#include <cstdint>
#include <cstdio>
#include <deque>
#include <map>
#include <optional>
#include <random>
#include <set>
#include <variant>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "functor_fixture.hpp"
#include "reference_pud_answer.hpp"
#include "reference_pud_frontier.hpp"
#include "reference_pud_goal_list.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/pud_module.hpp"
#include "infrastructure/pud_goal_resolver.hpp"
#include "infrastructure/pud_runtime.hpp"

using ::testing::IsEmpty;

template<typename IRuntime>
struct PudAnswerSetsRuntimeSuite2IntegrationTest : public ::testing::Test {
    test_functors functors;
    expr_pool exprs;
    pud_goal_resolver goal_resolver;

    const expr* var(uint32_t index) { return exprs.make_var(index); }
    const expr* atom(const char* name) { return exprs.make_functor(functors.id(name), {}); }
    const expr* functor(const char* name, std::vector<const expr*> args) {
        return exprs.make_functor(functors.id(name), std::move(args));
    }
};

using pud_answer_sets_runtime_suite_2_runtimes_t = ::testing::Types<pud_runtime<pud_module, expr_pool>>;
TYPED_TEST_SUITE(PudAnswerSetsRuntimeSuite2IntegrationTest, pud_answer_sets_runtime_suite_2_runtimes_t);

TYPED_TEST(PudAnswerSetsRuntimeSuite2IntegrationTest, EnumeratesTwoPathTwoColorings) {
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
    std::map<pud_global_node_id, pud_goal_list> leaf_goals;
    std::set<pud_global_node_id> unfoldable;
    for (pud_module& module : modules) {
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id leaf : change.offspring) {
            live.insert(leaf);
            leaf_goals.emplace(leaf, reference_pud_goal_list(module, leaf.node));
            if (leaf.module == 2 && !leaf_goals.at(leaf).goals.empty())
                unfoldable.insert(leaf);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_goals.erase(refuted);
            unfoldable.erase(refuted);
        }
    }

    size_t steps = 0;
    for (; steps < 2000; ++steps) {
        if (unfoldable.empty())
            break;
        const pud_global_node_id chosen_leaf = *unfoldable.begin();
        const pud_module* leaf_module = nullptr;
        for (const pud_module& module : modules) {
            if (module.get_module_id() == chosen_leaf.module)
                leaf_module = &module;
        }
        std::optional<pud_body_goal_id> chosen_goal;
        for (const auto& [goal_id, goal] : leaf_goals.at(chosen_leaf).goals) {
            if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                chosen_goal = goal_id;
        }
        const pud_frontier_change change = runtime.unfold(chosen_leaf, *chosen_goal);
        const pud_goal_list parent_goals = leaf_goals.at(chosen_leaf);
        live.erase(chosen_leaf);
        leaf_goals.erase(chosen_leaf);
        unfoldable.erase(chosen_leaf);
        for (pud_global_node_id child : change.offspring) {
            const pud_goal_list child_goals = this->goal_resolver.resolve_goal(parent_goals, *chosen_goal, leaf_module->get_inference(child.node).body_goals);
            live.insert(child);
            leaf_goals.emplace(child, child_goals);
            if (child.module == 2 && !child_goals.goals.empty())
                unfoldable.insert(child);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_goals.erase(refuted);
            unfoldable.erase(refuted);
        }
    }
    EXPECT_LT(steps, 2000u);
    for (pud_global_node_id leaf : live) {
        const pud_module* leaf_module = nullptr;
        for (const pud_module& module : modules) {
            if (module.get_module_id() == leaf.module)
                leaf_module = &module;
        }
        EXPECT_EQ(leaf_goals.at(leaf), reference_pud_goal_list(*leaf_module, leaf.node));
    }
    std::set<pud_global_node_id> reference_unfoldable;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        const pud_module* leaf_module = nullptr;
        for (const pud_module& module : modules) {
            if (module.get_module_id() == leaf.module)
                leaf_module = &module;
        }
        if (!reference_pud_goal_list(*leaf_module, leaf.node).goals.empty())
            reference_unfoldable.insert(leaf);
    }
    EXPECT_EQ(unfoldable, reference_unfoldable);

    std::set<const expr*> answers;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        const pud_module* leaf_module = nullptr;
        for (const pud_module& module : modules) {
            if (module.get_module_id() == leaf.module)
                leaf_module = &module;
        }
        if (!leaf_goals.at(leaf).goals.empty())
            continue;
        answers.insert(reference_pud_answer(*leaf_module, leaf.node, this->exprs));
    }

    EXPECT_EQ(answers, (std::set<const expr*>{
        this->functor("q", {this->atom("red"), this->atom("blue"), this->atom("red")}),
        this->functor("q", {this->atom("blue"), this->atom("red"), this->atom("blue")}),
    }));
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuite2IntegrationTest, EnumeratesTwoPathTwoColoringsInEveryUnfoldOrder) {
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
        std::map<pud_global_node_id, pud_goal_list> leaf_goals;
        std::set<pud_global_node_id> unfoldable;
        for (pud_module& module : modules) {
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id leaf : change.offspring) {
                live.insert(leaf);
                leaf_goals.emplace(leaf, reference_pud_goal_list(module, leaf.node));
                if (leaf.module == 2 && !leaf_goals.at(leaf).goals.empty())
                    unfoldable.insert(leaf);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                    leaf_goals.erase(refuted);
                unfoldable.erase(refuted);
            }
        }

        size_t steps = 0;
        for (; steps < 2000; ++steps) {
            if (unfoldable.empty())
                break;
            const pud_global_node_id leaf = *std::next(unfoldable.begin(), rng() % unfoldable.size());
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == leaf.module)
                    leaf_module = &module;
            }
            std::vector<pud_body_goal_id> goal_ids;
            for (const auto& [goal_id, goal] : leaf_goals.at(leaf).goals)
                goal_ids.push_back(goal_id);
            const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
            const pud_frontier_change change = runtime.unfold(leaf, goal);
            const pud_goal_list parent_goals = leaf_goals.at(leaf);
            live.erase(leaf);
            leaf_goals.erase(leaf);
            unfoldable.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
                const pud_goal_list child_goals = this->goal_resolver.resolve_goal(parent_goals, goal, leaf_module->get_inference(child.node).body_goals);
                live.insert(child);
                leaf_goals.emplace(child, child_goals);
                if (child.module == 2 && !child_goals.goals.empty())
                    unfoldable.insert(child);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                    leaf_goals.erase(refuted);
                unfoldable.erase(refuted);
            }
        }
        EXPECT_LT(steps, 2000u);
        for (pud_global_node_id leaf : live) {
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == leaf.module)
                    leaf_module = &module;
            }
            EXPECT_EQ(leaf_goals.at(leaf), reference_pud_goal_list(*leaf_module, leaf.node));
        }
        std::set<pud_global_node_id> reference_unfoldable;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == leaf.module)
                    leaf_module = &module;
            }
            if (!reference_pud_goal_list(*leaf_module, leaf.node).goals.empty())
                reference_unfoldable.insert(leaf);
        }
        EXPECT_EQ(unfoldable, reference_unfoldable);

        std::set<const expr*> answers;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == leaf.module)
                    leaf_module = &module;
            }
            if (!leaf_goals.at(leaf).goals.empty())
                continue;
            answers.insert(reference_pud_answer(*leaf_module, leaf.node, this->exprs));
        }

        EXPECT_EQ(answers, expected);
    }
}

TYPED_TEST(PudAnswerSetsRuntimeSuite2IntegrationTest, EnumeratesK3ThreeColorings) {
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
    std::map<pud_global_node_id, pud_goal_list> leaf_goals;
    std::set<pud_global_node_id> unfoldable;
    for (pud_module& module : modules) {
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id leaf : change.offspring) {
            live.insert(leaf);
            leaf_goals.emplace(leaf, reference_pud_goal_list(module, leaf.node));
            if (leaf.module == 2 && !leaf_goals.at(leaf).goals.empty())
                unfoldable.insert(leaf);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_goals.erase(refuted);
            unfoldable.erase(refuted);
        }
    }

    size_t steps = 0;
    for (; steps < 5000; ++steps) {
        if (unfoldable.empty())
            break;
        const pud_global_node_id chosen_leaf = *unfoldable.begin();
        const pud_module* leaf_module = nullptr;
        for (const pud_module& module : modules) {
            if (module.get_module_id() == chosen_leaf.module)
                leaf_module = &module;
        }
        std::optional<pud_body_goal_id> chosen_goal;
        for (const auto& [goal_id, goal] : leaf_goals.at(chosen_leaf).goals) {
            if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                chosen_goal = goal_id;
        }
        const pud_frontier_change change = runtime.unfold(chosen_leaf, *chosen_goal);
        const pud_goal_list parent_goals = leaf_goals.at(chosen_leaf);
        live.erase(chosen_leaf);
        leaf_goals.erase(chosen_leaf);
        unfoldable.erase(chosen_leaf);
        for (pud_global_node_id child : change.offspring) {
            const pud_goal_list child_goals = this->goal_resolver.resolve_goal(parent_goals, *chosen_goal, leaf_module->get_inference(child.node).body_goals);
            live.insert(child);
            leaf_goals.emplace(child, child_goals);
            if (child.module == 2 && !child_goals.goals.empty())
                unfoldable.insert(child);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_goals.erase(refuted);
            unfoldable.erase(refuted);
        }
    }
    EXPECT_LT(steps, 5000u);
    for (pud_global_node_id leaf : live) {
        const pud_module* leaf_module = nullptr;
        for (const pud_module& module : modules) {
            if (module.get_module_id() == leaf.module)
                leaf_module = &module;
        }
        EXPECT_EQ(leaf_goals.at(leaf), reference_pud_goal_list(*leaf_module, leaf.node));
    }
    std::set<pud_global_node_id> reference_unfoldable;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        const pud_module* leaf_module = nullptr;
        for (const pud_module& module : modules) {
            if (module.get_module_id() == leaf.module)
                leaf_module = &module;
        }
        if (!reference_pud_goal_list(*leaf_module, leaf.node).goals.empty())
            reference_unfoldable.insert(leaf);
    }
    EXPECT_EQ(unfoldable, reference_unfoldable);

    std::set<const expr*> answers;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        const pud_module* leaf_module = nullptr;
        for (const pud_module& module : modules) {
            if (module.get_module_id() == leaf.module)
                leaf_module = &module;
        }
        if (!leaf_goals.at(leaf).goals.empty())
            continue;
        answers.insert(reference_pud_answer(*leaf_module, leaf.node, this->exprs));
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

TYPED_TEST(PudAnswerSetsRuntimeSuite2IntegrationTest, EnumeratesK3ThreeColoringsInEveryUnfoldOrder) {
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
        std::map<pud_global_node_id, pud_goal_list> leaf_goals;
        std::set<pud_global_node_id> unfoldable;
        for (pud_module& module : modules) {
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id leaf : change.offspring) {
                live.insert(leaf);
                leaf_goals.emplace(leaf, reference_pud_goal_list(module, leaf.node));
                if (leaf.module == 2 && !leaf_goals.at(leaf).goals.empty())
                    unfoldable.insert(leaf);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                    leaf_goals.erase(refuted);
                unfoldable.erase(refuted);
            }
        }

        size_t steps = 0;
        for (; steps < 5000; ++steps) {
            if (unfoldable.empty())
                break;
            const pud_global_node_id leaf = *std::next(unfoldable.begin(), rng() % unfoldable.size());
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == leaf.module)
                    leaf_module = &module;
            }
            std::vector<pud_body_goal_id> goal_ids;
            for (const auto& [goal_id, goal] : leaf_goals.at(leaf).goals)
                goal_ids.push_back(goal_id);
            const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
            const pud_frontier_change change = runtime.unfold(leaf, goal);
            const pud_goal_list parent_goals = leaf_goals.at(leaf);
            live.erase(leaf);
            leaf_goals.erase(leaf);
            unfoldable.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
                const pud_goal_list child_goals = this->goal_resolver.resolve_goal(parent_goals, goal, leaf_module->get_inference(child.node).body_goals);
                live.insert(child);
                leaf_goals.emplace(child, child_goals);
                if (child.module == 2 && !child_goals.goals.empty())
                    unfoldable.insert(child);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                    leaf_goals.erase(refuted);
                unfoldable.erase(refuted);
            }
        }
        EXPECT_LT(steps, 5000u);
        for (pud_global_node_id leaf : live) {
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == leaf.module)
                    leaf_module = &module;
            }
            EXPECT_EQ(leaf_goals.at(leaf), reference_pud_goal_list(*leaf_module, leaf.node));
        }
        std::set<pud_global_node_id> reference_unfoldable;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == leaf.module)
                    leaf_module = &module;
            }
            if (!reference_pud_goal_list(*leaf_module, leaf.node).goals.empty())
                reference_unfoldable.insert(leaf);
        }
        EXPECT_EQ(unfoldable, reference_unfoldable);

        std::set<const expr*> answers;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == leaf.module)
                    leaf_module = &module;
            }
            if (!leaf_goals.at(leaf).goals.empty())
                continue;
            answers.insert(reference_pud_answer(*leaf_module, leaf.node, this->exprs));
        }

        EXPECT_EQ(answers, expected);
    }
}

TYPED_TEST(PudAnswerSetsRuntimeSuite2IntegrationTest, EnumeratesK3TailFourNodeColorings) {
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
    std::map<pud_global_node_id, pud_goal_list> leaf_goals;
    std::set<pud_global_node_id> unfoldable;
    for (pud_module& module : modules) {
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id leaf : change.offspring) {
            live.insert(leaf);
            leaf_goals.emplace(leaf, reference_pud_goal_list(module, leaf.node));
            if (leaf.module == 2 && !leaf_goals.at(leaf).goals.empty())
                unfoldable.insert(leaf);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_goals.erase(refuted);
            unfoldable.erase(refuted);
        }
    }

    size_t steps = 0;
    for (; steps < 8000; ++steps) {
        if (unfoldable.empty())
            break;
        const pud_global_node_id chosen_leaf = *unfoldable.begin();
        const pud_module* leaf_module = nullptr;
        for (const pud_module& module : modules) {
            if (module.get_module_id() == chosen_leaf.module)
                leaf_module = &module;
        }
        std::optional<pud_body_goal_id> chosen_goal;
        for (const auto& [goal_id, goal] : leaf_goals.at(chosen_leaf).goals) {
            if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                chosen_goal = goal_id;
        }
        const pud_frontier_change change = runtime.unfold(chosen_leaf, *chosen_goal);
        const pud_goal_list parent_goals = leaf_goals.at(chosen_leaf);
        live.erase(chosen_leaf);
        leaf_goals.erase(chosen_leaf);
        unfoldable.erase(chosen_leaf);
        for (pud_global_node_id child : change.offspring) {
            const pud_goal_list child_goals = this->goal_resolver.resolve_goal(parent_goals, *chosen_goal, leaf_module->get_inference(child.node).body_goals);
            live.insert(child);
            leaf_goals.emplace(child, child_goals);
            if (child.module == 2 && !child_goals.goals.empty())
                unfoldable.insert(child);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_goals.erase(refuted);
            unfoldable.erase(refuted);
        }
    }
    EXPECT_LT(steps, 8000u);
    for (pud_global_node_id leaf : live) {
        const pud_module* leaf_module = nullptr;
        for (const pud_module& module : modules) {
            if (module.get_module_id() == leaf.module)
                leaf_module = &module;
        }
        EXPECT_EQ(leaf_goals.at(leaf), reference_pud_goal_list(*leaf_module, leaf.node));
    }
    std::set<pud_global_node_id> reference_unfoldable;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        const pud_module* leaf_module = nullptr;
        for (const pud_module& module : modules) {
            if (module.get_module_id() == leaf.module)
                leaf_module = &module;
        }
        if (!reference_pud_goal_list(*leaf_module, leaf.node).goals.empty())
            reference_unfoldable.insert(leaf);
    }
    EXPECT_EQ(unfoldable, reference_unfoldable);

    std::set<const expr*> answers;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        const pud_module* leaf_module = nullptr;
        for (const pud_module& module : modules) {
            if (module.get_module_id() == leaf.module)
                leaf_module = &module;
        }
        if (!leaf_goals.at(leaf).goals.empty())
            continue;
        answers.insert(reference_pud_answer(*leaf_module, leaf.node, this->exprs));
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

TYPED_TEST(PudAnswerSetsRuntimeSuite2IntegrationTest, EnumeratesK3TailFourNodeColoringsInEveryUnfoldOrder) {
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
        std::map<pud_global_node_id, pud_goal_list> leaf_goals;
        std::set<pud_global_node_id> unfoldable;
        for (pud_module& module : modules) {
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id leaf : change.offspring) {
                live.insert(leaf);
                leaf_goals.emplace(leaf, reference_pud_goal_list(module, leaf.node));
                if (leaf.module == 2 && !leaf_goals.at(leaf).goals.empty())
                    unfoldable.insert(leaf);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                    leaf_goals.erase(refuted);
                unfoldable.erase(refuted);
            }
        }

        size_t steps = 0;
        for (; steps < 8000; ++steps) {
            if (unfoldable.empty())
                break;
            const pud_global_node_id leaf = *std::next(unfoldable.begin(), rng() % unfoldable.size());
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == leaf.module)
                    leaf_module = &module;
            }
            std::vector<pud_body_goal_id> goal_ids;
            for (const auto& [goal_id, goal] : leaf_goals.at(leaf).goals)
                goal_ids.push_back(goal_id);
            const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
            const pud_frontier_change change = runtime.unfold(leaf, goal);
            const pud_goal_list parent_goals = leaf_goals.at(leaf);
            live.erase(leaf);
            leaf_goals.erase(leaf);
            unfoldable.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
                const pud_goal_list child_goals = this->goal_resolver.resolve_goal(parent_goals, goal, leaf_module->get_inference(child.node).body_goals);
                live.insert(child);
                leaf_goals.emplace(child, child_goals);
                if (child.module == 2 && !child_goals.goals.empty())
                    unfoldable.insert(child);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                    leaf_goals.erase(refuted);
                unfoldable.erase(refuted);
            }
        }
        EXPECT_LT(steps, 8000u);
        for (pud_global_node_id leaf : live) {
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == leaf.module)
                    leaf_module = &module;
            }
            EXPECT_EQ(leaf_goals.at(leaf), reference_pud_goal_list(*leaf_module, leaf.node));
        }
        std::set<pud_global_node_id> reference_unfoldable;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == leaf.module)
                    leaf_module = &module;
            }
            if (!reference_pud_goal_list(*leaf_module, leaf.node).goals.empty())
                reference_unfoldable.insert(leaf);
        }
        EXPECT_EQ(unfoldable, reference_unfoldable);

        std::set<const expr*> answers;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == leaf.module)
                    leaf_module = &module;
            }
            if (!leaf_goals.at(leaf).goals.empty())
                continue;
            answers.insert(reference_pud_answer(*leaf_module, leaf.node, this->exprs));
        }

        EXPECT_EQ(answers, expected);
    }
}

TYPED_TEST(PudAnswerSetsRuntimeSuite2IntegrationTest, EnumeratesFourVarSatThreeClauses) {
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
    std::map<pud_global_node_id, pud_goal_list> leaf_goals;
    std::set<pud_global_node_id> unfoldable;
    for (pud_module& module : modules) {
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id leaf : change.offspring) {
            live.insert(leaf);
            leaf_goals.emplace(leaf, reference_pud_goal_list(module, leaf.node));
            if (leaf.module == 2 && !leaf_goals.at(leaf).goals.empty())
                unfoldable.insert(leaf);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_goals.erase(refuted);
            unfoldable.erase(refuted);
        }
    }

    size_t steps = 0;
    for (; steps < 10000; ++steps) {
        if (unfoldable.empty())
            break;
        const pud_global_node_id chosen_leaf = *unfoldable.begin();
        const pud_module* leaf_module = nullptr;
        for (const pud_module& module : modules) {
            if (module.get_module_id() == chosen_leaf.module)
                leaf_module = &module;
        }
        std::optional<pud_body_goal_id> chosen_goal;
        for (const auto& [goal_id, goal] : leaf_goals.at(chosen_leaf).goals) {
            if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                chosen_goal = goal_id;
        }
        const pud_frontier_change change = runtime.unfold(chosen_leaf, *chosen_goal);
        const pud_goal_list parent_goals = leaf_goals.at(chosen_leaf);
        live.erase(chosen_leaf);
        leaf_goals.erase(chosen_leaf);
        unfoldable.erase(chosen_leaf);
        for (pud_global_node_id child : change.offspring) {
            const pud_goal_list child_goals = this->goal_resolver.resolve_goal(parent_goals, *chosen_goal, leaf_module->get_inference(child.node).body_goals);
            live.insert(child);
            leaf_goals.emplace(child, child_goals);
            if (child.module == 2 && !child_goals.goals.empty())
                unfoldable.insert(child);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_goals.erase(refuted);
            unfoldable.erase(refuted);
        }
    }
    EXPECT_LT(steps, 10000u);
    for (pud_global_node_id leaf : live) {
        const pud_module* leaf_module = nullptr;
        for (const pud_module& module : modules) {
            if (module.get_module_id() == leaf.module)
                leaf_module = &module;
        }
        EXPECT_EQ(leaf_goals.at(leaf), reference_pud_goal_list(*leaf_module, leaf.node));
    }
    std::set<pud_global_node_id> reference_unfoldable;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        const pud_module* leaf_module = nullptr;
        for (const pud_module& module : modules) {
            if (module.get_module_id() == leaf.module)
                leaf_module = &module;
        }
        if (!reference_pud_goal_list(*leaf_module, leaf.node).goals.empty())
            reference_unfoldable.insert(leaf);
    }
    EXPECT_EQ(unfoldable, reference_unfoldable);

    std::set<const expr*> answers;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        const pud_module* leaf_module = nullptr;
        for (const pud_module& module : modules) {
            if (module.get_module_id() == leaf.module)
                leaf_module = &module;
        }
        if (!leaf_goals.at(leaf).goals.empty())
            continue;
        answers.insert(reference_pud_answer(*leaf_module, leaf.node, this->exprs));
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

TYPED_TEST(PudAnswerSetsRuntimeSuite2IntegrationTest, EnumeratesFourVarSatThreeClausesInEveryUnfoldOrder) {
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
        std::map<pud_global_node_id, pud_goal_list> leaf_goals;
        std::set<pud_global_node_id> unfoldable;
        for (pud_module& module : modules) {
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id leaf : change.offspring) {
                live.insert(leaf);
                leaf_goals.emplace(leaf, reference_pud_goal_list(module, leaf.node));
                if (leaf.module == 2 && !leaf_goals.at(leaf).goals.empty())
                    unfoldable.insert(leaf);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                    leaf_goals.erase(refuted);
                unfoldable.erase(refuted);
            }
        }

        size_t steps = 0;
        for (; steps < 10000; ++steps) {
            if (unfoldable.empty())
                break;
            const pud_global_node_id leaf = *std::next(unfoldable.begin(), rng() % unfoldable.size());
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == leaf.module)
                    leaf_module = &module;
            }
            std::vector<pud_body_goal_id> goal_ids;
            for (const auto& [goal_id, goal] : leaf_goals.at(leaf).goals)
                goal_ids.push_back(goal_id);
            const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
            const pud_frontier_change change = runtime.unfold(leaf, goal);
            const pud_goal_list parent_goals = leaf_goals.at(leaf);
            live.erase(leaf);
            leaf_goals.erase(leaf);
            unfoldable.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
                const pud_goal_list child_goals = this->goal_resolver.resolve_goal(parent_goals, goal, leaf_module->get_inference(child.node).body_goals);
                live.insert(child);
                leaf_goals.emplace(child, child_goals);
                if (child.module == 2 && !child_goals.goals.empty())
                    unfoldable.insert(child);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                    leaf_goals.erase(refuted);
                unfoldable.erase(refuted);
            }
        }
        EXPECT_LT(steps, 10000u);
        for (pud_global_node_id leaf : live) {
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == leaf.module)
                    leaf_module = &module;
            }
            EXPECT_EQ(leaf_goals.at(leaf), reference_pud_goal_list(*leaf_module, leaf.node));
        }
        std::set<pud_global_node_id> reference_unfoldable;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == leaf.module)
                    leaf_module = &module;
            }
            if (!reference_pud_goal_list(*leaf_module, leaf.node).goals.empty())
                reference_unfoldable.insert(leaf);
        }
        EXPECT_EQ(unfoldable, reference_unfoldable);

        std::set<const expr*> answers;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == leaf.module)
                    leaf_module = &module;
            }
            if (!leaf_goals.at(leaf).goals.empty())
                continue;
            answers.insert(reference_pud_answer(*leaf_module, leaf.node, this->exprs));
        }

        EXPECT_EQ(answers, expected);
    }
}

TYPED_TEST(PudAnswerSetsRuntimeSuite2IntegrationTest, EnumeratesAddPairsSummingLessThanTen) {
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
    std::map<pud_global_node_id, pud_goal_list> leaf_goals;
    std::set<pud_global_node_id> unfoldable;
    for (pud_module& module : modules) {
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id leaf : change.offspring) {
            live.insert(leaf);
            leaf_goals.emplace(leaf, reference_pud_goal_list(module, leaf.node));
            if (leaf.module == 2 && !leaf_goals.at(leaf).goals.empty())
                unfoldable.insert(leaf);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_goals.erase(refuted);
            unfoldable.erase(refuted);
        }
    }

    size_t steps = 0;
    for (; steps < 15000; ++steps) {
        if (unfoldable.empty())
            break;
        const pud_global_node_id chosen_leaf = *unfoldable.begin();
        const pud_module* leaf_module = nullptr;
        for (const pud_module& module : modules) {
            if (module.get_module_id() == chosen_leaf.module)
                leaf_module = &module;
        }
        std::optional<pud_body_goal_id> chosen_goal;
        for (const auto& [goal_id, goal] : leaf_goals.at(chosen_leaf).goals) {
            if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                chosen_goal = goal_id;
        }
        const pud_frontier_change change = runtime.unfold(chosen_leaf, *chosen_goal);
        const pud_goal_list parent_goals = leaf_goals.at(chosen_leaf);
        live.erase(chosen_leaf);
        leaf_goals.erase(chosen_leaf);
        unfoldable.erase(chosen_leaf);
        for (pud_global_node_id child : change.offspring) {
            const pud_goal_list child_goals = this->goal_resolver.resolve_goal(parent_goals, *chosen_goal, leaf_module->get_inference(child.node).body_goals);
            live.insert(child);
            leaf_goals.emplace(child, child_goals);
            if (child.module == 2 && !child_goals.goals.empty())
                unfoldable.insert(child);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_goals.erase(refuted);
            unfoldable.erase(refuted);
        }
    }
    EXPECT_LT(steps, 15000u);
    for (pud_global_node_id leaf : live) {
        const pud_module* leaf_module = nullptr;
        for (const pud_module& module : modules) {
            if (module.get_module_id() == leaf.module)
                leaf_module = &module;
        }
        EXPECT_EQ(leaf_goals.at(leaf), reference_pud_goal_list(*leaf_module, leaf.node));
    }
    std::set<pud_global_node_id> reference_unfoldable;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        const pud_module* leaf_module = nullptr;
        for (const pud_module& module : modules) {
            if (module.get_module_id() == leaf.module)
                leaf_module = &module;
        }
        if (!reference_pud_goal_list(*leaf_module, leaf.node).goals.empty())
            reference_unfoldable.insert(leaf);
    }
    EXPECT_EQ(unfoldable, reference_unfoldable);

    std::set<const expr*> answers;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        const pud_module* leaf_module = nullptr;
        for (const pud_module& module : modules) {
            if (module.get_module_id() == leaf.module)
                leaf_module = &module;
        }
        if (!leaf_goals.at(leaf).goals.empty())
            continue;
        answers.insert(reference_pud_answer(*leaf_module, leaf.node, this->exprs));
    }

    EXPECT_EQ(answers, expected);
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuite2IntegrationTest, EnumeratesAddPairsSummingLessThanTenInEveryUnfoldOrder) {
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
        std::map<pud_global_node_id, pud_goal_list> leaf_goals;
        std::set<pud_global_node_id> unfoldable;
        for (pud_module& module : modules) {
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id leaf : change.offspring) {
                live.insert(leaf);
                leaf_goals.emplace(leaf, reference_pud_goal_list(module, leaf.node));
                if (leaf.module == 2 && !leaf_goals.at(leaf).goals.empty())
                    unfoldable.insert(leaf);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                    leaf_goals.erase(refuted);
                unfoldable.erase(refuted);
            }
        }

        size_t steps = 0;
        for (; steps < 15000; ++steps) {
            if (unfoldable.empty())
                break;
            const pud_global_node_id leaf = *std::next(unfoldable.begin(), rng() % unfoldable.size());
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == leaf.module)
                    leaf_module = &module;
            }
            std::vector<pud_body_goal_id> goal_ids;
            for (const auto& [goal_id, goal] : leaf_goals.at(leaf).goals)
                goal_ids.push_back(goal_id);
            const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
            const pud_frontier_change change = runtime.unfold(leaf, goal);
            const pud_goal_list parent_goals = leaf_goals.at(leaf);
            live.erase(leaf);
            leaf_goals.erase(leaf);
            unfoldable.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
                const pud_goal_list child_goals = this->goal_resolver.resolve_goal(parent_goals, goal, leaf_module->get_inference(child.node).body_goals);
                live.insert(child);
                leaf_goals.emplace(child, child_goals);
                if (child.module == 2 && !child_goals.goals.empty())
                    unfoldable.insert(child);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                    leaf_goals.erase(refuted);
                unfoldable.erase(refuted);
            }
        }
        EXPECT_LT(steps, 15000u);
        for (pud_global_node_id leaf : live) {
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == leaf.module)
                    leaf_module = &module;
            }
            EXPECT_EQ(leaf_goals.at(leaf), reference_pud_goal_list(*leaf_module, leaf.node));
        }
        std::set<pud_global_node_id> reference_unfoldable;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == leaf.module)
                    leaf_module = &module;
            }
            if (!reference_pud_goal_list(*leaf_module, leaf.node).goals.empty())
                reference_unfoldable.insert(leaf);
        }
        EXPECT_EQ(unfoldable, reference_unfoldable);

        std::set<const expr*> answers;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == leaf.module)
                    leaf_module = &module;
            }
            if (!leaf_goals.at(leaf).goals.empty())
                continue;
            answers.insert(reference_pud_answer(*leaf_module, leaf.node, this->exprs));
        }

        EXPECT_EQ(answers, expected);
    }
}

TYPED_TEST(PudAnswerSetsRuntimeSuite2IntegrationTest, EnumeratesAddPairsSummingExactlyTen) {
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
    std::map<pud_global_node_id, pud_goal_list> leaf_goals;
    std::set<pud_global_node_id> unfoldable;
    for (pud_module& module : modules) {
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id leaf : change.offspring) {
            live.insert(leaf);
            leaf_goals.emplace(leaf, reference_pud_goal_list(module, leaf.node));
            if (leaf.module == 2 && !leaf_goals.at(leaf).goals.empty())
                unfoldable.insert(leaf);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_goals.erase(refuted);
            unfoldable.erase(refuted);
        }
    }

    size_t steps = 0;
    for (; steps < 10000; ++steps) {
        if (unfoldable.empty())
            break;
        const pud_global_node_id chosen_leaf = *unfoldable.begin();
        const pud_module* leaf_module = nullptr;
        for (const pud_module& module : modules) {
            if (module.get_module_id() == chosen_leaf.module)
                leaf_module = &module;
        }
        std::optional<pud_body_goal_id> chosen_goal;
        for (const auto& [goal_id, goal] : leaf_goals.at(chosen_leaf).goals) {
            if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                chosen_goal = goal_id;
        }
        const pud_frontier_change change = runtime.unfold(chosen_leaf, *chosen_goal);
        const pud_goal_list parent_goals = leaf_goals.at(chosen_leaf);
        live.erase(chosen_leaf);
        leaf_goals.erase(chosen_leaf);
        unfoldable.erase(chosen_leaf);
        for (pud_global_node_id child : change.offspring) {
            const pud_goal_list child_goals = this->goal_resolver.resolve_goal(parent_goals, *chosen_goal, leaf_module->get_inference(child.node).body_goals);
            live.insert(child);
            leaf_goals.emplace(child, child_goals);
            if (child.module == 2 && !child_goals.goals.empty())
                unfoldable.insert(child);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_goals.erase(refuted);
            unfoldable.erase(refuted);
        }
    }
    EXPECT_LT(steps, 10000u);
    for (pud_global_node_id leaf : live) {
        const pud_module* leaf_module = nullptr;
        for (const pud_module& module : modules) {
            if (module.get_module_id() == leaf.module)
                leaf_module = &module;
        }
        EXPECT_EQ(leaf_goals.at(leaf), reference_pud_goal_list(*leaf_module, leaf.node));
    }
    std::set<pud_global_node_id> reference_unfoldable;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        const pud_module* leaf_module = nullptr;
        for (const pud_module& module : modules) {
            if (module.get_module_id() == leaf.module)
                leaf_module = &module;
        }
        if (!reference_pud_goal_list(*leaf_module, leaf.node).goals.empty())
            reference_unfoldable.insert(leaf);
    }
    EXPECT_EQ(unfoldable, reference_unfoldable);

    std::set<const expr*> answers;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        const pud_module* leaf_module = nullptr;
        for (const pud_module& module : modules) {
            if (module.get_module_id() == leaf.module)
                leaf_module = &module;
        }
        if (!leaf_goals.at(leaf).goals.empty())
            continue;
        answers.insert(reference_pud_answer(*leaf_module, leaf.node, this->exprs));
    }

    EXPECT_EQ(answers, expected);
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuite2IntegrationTest, EnumeratesAddPairsSummingExactlyTenInEveryUnfoldOrder) {
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
        std::map<pud_global_node_id, pud_goal_list> leaf_goals;
        std::set<pud_global_node_id> unfoldable;
        for (pud_module& module : modules) {
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id leaf : change.offspring) {
                live.insert(leaf);
                leaf_goals.emplace(leaf, reference_pud_goal_list(module, leaf.node));
                if (leaf.module == 2 && !leaf_goals.at(leaf).goals.empty())
                    unfoldable.insert(leaf);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                    leaf_goals.erase(refuted);
                unfoldable.erase(refuted);
            }
        }

        size_t steps = 0;
        for (; steps < 10000; ++steps) {
            if (unfoldable.empty())
                break;
            const pud_global_node_id leaf = *std::next(unfoldable.begin(), rng() % unfoldable.size());
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == leaf.module)
                    leaf_module = &module;
            }
            std::vector<pud_body_goal_id> goal_ids;
            for (const auto& [goal_id, goal] : leaf_goals.at(leaf).goals)
                goal_ids.push_back(goal_id);
            const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
            const pud_frontier_change change = runtime.unfold(leaf, goal);
            const pud_goal_list parent_goals = leaf_goals.at(leaf);
            live.erase(leaf);
            leaf_goals.erase(leaf);
            unfoldable.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
                const pud_goal_list child_goals = this->goal_resolver.resolve_goal(parent_goals, goal, leaf_module->get_inference(child.node).body_goals);
                live.insert(child);
                leaf_goals.emplace(child, child_goals);
                if (child.module == 2 && !child_goals.goals.empty())
                    unfoldable.insert(child);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                    leaf_goals.erase(refuted);
                unfoldable.erase(refuted);
            }
        }
        EXPECT_LT(steps, 10000u);
        for (pud_global_node_id leaf : live) {
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == leaf.module)
                    leaf_module = &module;
            }
            EXPECT_EQ(leaf_goals.at(leaf), reference_pud_goal_list(*leaf_module, leaf.node));
        }
        std::set<pud_global_node_id> reference_unfoldable;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == leaf.module)
                    leaf_module = &module;
            }
            if (!reference_pud_goal_list(*leaf_module, leaf.node).goals.empty())
                reference_unfoldable.insert(leaf);
        }
        EXPECT_EQ(unfoldable, reference_unfoldable);

        std::set<const expr*> answers;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == leaf.module)
                    leaf_module = &module;
            }
            if (!leaf_goals.at(leaf).goals.empty())
                continue;
            answers.insert(reference_pud_answer(*leaf_module, leaf.node, this->exprs));
        }

        EXPECT_EQ(answers, expected);
    }
}

TYPED_TEST(PudAnswerSetsRuntimeSuite2IntegrationTest, EnumeratesMulPairsProductEight) {
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
    std::map<pud_global_node_id, pud_goal_list> leaf_goals;
    std::set<pud_global_node_id> unfoldable;
    for (pud_module& module : modules) {
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id leaf : change.offspring) {
            live.insert(leaf);
            leaf_goals.emplace(leaf, reference_pud_goal_list(module, leaf.node));
            if (leaf.module == 2 && !leaf_goals.at(leaf).goals.empty())
                unfoldable.insert(leaf);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_goals.erase(refuted);
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
        const pud_module* leaf_module = nullptr;
        for (const pud_module& module : modules) {
            if (module.get_module_id() == chosen_leaf.module)
                leaf_module = &module;
        }
        std::optional<pud_body_goal_id> chosen_goal;
        for (const auto& [goal_id, goal] : leaf_goals.at(chosen_leaf).goals) {
            if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                chosen_goal = goal_id;
        }
        const pud_frontier_change change = runtime.unfold(chosen_leaf, *chosen_goal);
        const pud_goal_list parent_goals = leaf_goals.at(chosen_leaf);
        live.erase(chosen_leaf);
        leaf_goals.erase(chosen_leaf);
        unfoldable.erase(chosen_leaf);
        for (pud_global_node_id child : change.offspring) {
            const pud_goal_list child_goals = this->goal_resolver.resolve_goal(parent_goals, *chosen_goal, leaf_module->get_inference(child.node).body_goals);
            live.insert(child);
            leaf_goals.emplace(child, child_goals);
            if (child.module == 2 && !child_goals.goals.empty())
                unfoldable.insert(child);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_goals.erase(refuted);
            unfoldable.erase(refuted);
        }
    }
    for (pud_global_node_id leaf : live) {
        const pud_module* leaf_module = nullptr;
        for (const pud_module& module : modules) {
            if (module.get_module_id() == leaf.module)
                leaf_module = &module;
        }
        EXPECT_EQ(leaf_goals.at(leaf), reference_pud_goal_list(*leaf_module, leaf.node));
    }
    std::set<pud_global_node_id> reference_unfoldable;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        const pud_module* leaf_module = nullptr;
        for (const pud_module& module : modules) {
            if (module.get_module_id() == leaf.module)
                leaf_module = &module;
        }
        if (!reference_pud_goal_list(*leaf_module, leaf.node).goals.empty())
            reference_unfoldable.insert(leaf);
    }
    EXPECT_EQ(unfoldable, reference_unfoldable);

    std::set<const expr*> answers;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        const pud_module* leaf_module = nullptr;
        for (const pud_module& module : modules) {
            if (module.get_module_id() == leaf.module)
                leaf_module = &module;
        }
        if (!leaf_goals.at(leaf).goals.empty())
            continue;
        answers.insert(reference_pud_answer(*leaf_module, leaf.node, this->exprs));
    }
    EXPECT_EQ(answers, expected);
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuite2IntegrationTest, EnumeratesMulPairsProductEightInEveryUnfoldOrder) {
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
        std::map<pud_global_node_id, pud_goal_list> leaf_goals;
        std::set<pud_global_node_id> unfoldable;
        for (pud_module& module : modules) {
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id leaf : change.offspring) {
                live.insert(leaf);
                leaf_goals.emplace(leaf, reference_pud_goal_list(module, leaf.node));
                if (leaf.module == 2 && !leaf_goals.at(leaf).goals.empty())
                    unfoldable.insert(leaf);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                    leaf_goals.erase(refuted);
                unfoldable.erase(refuted);
            }
        }

        size_t steps = 0;
        for (; steps < kMaxStepsPerSeed; ++steps) {
            if (unfoldable.empty())
                break;
            const pud_global_node_id leaf = *std::next(unfoldable.begin(), rng() % unfoldable.size());
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == leaf.module)
                    leaf_module = &module;
            }
            std::vector<pud_body_goal_id> goal_ids;
            for (const auto& [goal_id, goal] : leaf_goals.at(leaf).goals)
                goal_ids.push_back(goal_id);
            const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
            const pud_frontier_change change = runtime.unfold(leaf, goal);
            const pud_goal_list parent_goals = leaf_goals.at(leaf);
            live.erase(leaf);
            leaf_goals.erase(leaf);
            unfoldable.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
                const pud_goal_list child_goals = this->goal_resolver.resolve_goal(parent_goals, goal, leaf_module->get_inference(child.node).body_goals);
                live.insert(child);
                leaf_goals.emplace(child, child_goals);
                if (child.module == 2 && !child_goals.goals.empty())
                    unfoldable.insert(child);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                leaf_goals.erase(refuted);
                unfoldable.erase(refuted);
            }
        }
        for (pud_global_node_id leaf : live) {
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == leaf.module)
                    leaf_module = &module;
            }
            EXPECT_EQ(leaf_goals.at(leaf), reference_pud_goal_list(*leaf_module, leaf.node));
        }
        std::set<pud_global_node_id> reference_unfoldable;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == leaf.module)
                    leaf_module = &module;
            }
            if (!reference_pud_goal_list(*leaf_module, leaf.node).goals.empty())
                reference_unfoldable.insert(leaf);
        }
        EXPECT_EQ(unfoldable, reference_unfoldable);

        std::set<const expr*> answers;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == leaf.module)
                    leaf_module = &module;
            }
            if (!leaf_goals.at(leaf).goals.empty())
                continue;
            answers.insert(reference_pud_answer(*leaf_module, leaf.node, this->exprs));
        }

        for (const expr* answer : answers) {
            EXPECT_TRUE(expected.contains(answer))
                << "seed " << seed << " produced an answer not in the expected set";
            found_across_seeds.insert(answer);
        }
    }
    EXPECT_EQ(found_across_seeds, expected);
}

TYPED_TEST(PudAnswerSetsRuntimeSuite2IntegrationTest, EnumeratesDualBoundedSharedXSums) {
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
    std::map<pud_global_node_id, pud_goal_list> leaf_goals;
    std::set<pud_global_node_id> unfoldable;
    for (pud_module& module : modules) {
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id leaf : change.offspring) {
            live.insert(leaf);
            leaf_goals.emplace(leaf, reference_pud_goal_list(module, leaf.node));
            if (leaf.module == 2 && !leaf_goals.at(leaf).goals.empty())
                unfoldable.insert(leaf);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_goals.erase(refuted);
            unfoldable.erase(refuted);
        }
    }

    size_t steps = 0;
    for (; steps < 15000; ++steps) {
        if (unfoldable.empty())
            break;
        const pud_global_node_id chosen_leaf = *unfoldable.begin();
        const pud_module* leaf_module = nullptr;
        for (const pud_module& module : modules) {
            if (module.get_module_id() == chosen_leaf.module)
                leaf_module = &module;
        }
        std::optional<pud_body_goal_id> chosen_goal;
        for (const auto& [goal_id, goal] : leaf_goals.at(chosen_leaf).goals) {
            if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                chosen_goal = goal_id;
        }
        const pud_frontier_change change = runtime.unfold(chosen_leaf, *chosen_goal);
        const pud_goal_list parent_goals = leaf_goals.at(chosen_leaf);
        live.erase(chosen_leaf);
        leaf_goals.erase(chosen_leaf);
        unfoldable.erase(chosen_leaf);
        for (pud_global_node_id child : change.offspring) {
            const pud_goal_list child_goals = this->goal_resolver.resolve_goal(parent_goals, *chosen_goal, leaf_module->get_inference(child.node).body_goals);
            live.insert(child);
            leaf_goals.emplace(child, child_goals);
            if (child.module == 2 && !child_goals.goals.empty())
                unfoldable.insert(child);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_goals.erase(refuted);
            unfoldable.erase(refuted);
        }
    }
    EXPECT_LT(steps, 15000u);
    for (pud_global_node_id leaf : live) {
        const pud_module* leaf_module = nullptr;
        for (const pud_module& module : modules) {
            if (module.get_module_id() == leaf.module)
                leaf_module = &module;
        }
        EXPECT_EQ(leaf_goals.at(leaf), reference_pud_goal_list(*leaf_module, leaf.node));
    }
    std::set<pud_global_node_id> reference_unfoldable;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        const pud_module* leaf_module = nullptr;
        for (const pud_module& module : modules) {
            if (module.get_module_id() == leaf.module)
                leaf_module = &module;
        }
        if (!reference_pud_goal_list(*leaf_module, leaf.node).goals.empty())
            reference_unfoldable.insert(leaf);
    }
    EXPECT_EQ(unfoldable, reference_unfoldable);

    std::set<const expr*> answers;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        const pud_module* leaf_module = nullptr;
        for (const pud_module& module : modules) {
            if (module.get_module_id() == leaf.module)
                leaf_module = &module;
        }
        if (!leaf_goals.at(leaf).goals.empty())
            continue;
        answers.insert(reference_pud_answer(*leaf_module, leaf.node, this->exprs));
    }

    EXPECT_EQ(answers, expected);
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuite2IntegrationTest, EnumeratesDualBoundedSharedXSumsInEveryUnfoldOrder) {
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
        std::map<pud_global_node_id, pud_goal_list> leaf_goals;
        std::set<pud_global_node_id> unfoldable;
        for (pud_module& module : modules) {
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id leaf : change.offspring) {
                live.insert(leaf);
                leaf_goals.emplace(leaf, reference_pud_goal_list(module, leaf.node));
                if (leaf.module == 2 && !leaf_goals.at(leaf).goals.empty())
                    unfoldable.insert(leaf);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                    leaf_goals.erase(refuted);
                unfoldable.erase(refuted);
            }
        }

        size_t steps = 0;
        for (; steps < 15000; ++steps) {
            if (unfoldable.empty())
                break;
            const pud_global_node_id leaf = *std::next(unfoldable.begin(), rng() % unfoldable.size());
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == leaf.module)
                    leaf_module = &module;
            }
            std::vector<pud_body_goal_id> goal_ids;
            for (const auto& [goal_id, goal] : leaf_goals.at(leaf).goals)
                goal_ids.push_back(goal_id);
            const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
            const pud_frontier_change change = runtime.unfold(leaf, goal);
            const pud_goal_list parent_goals = leaf_goals.at(leaf);
            live.erase(leaf);
            leaf_goals.erase(leaf);
            unfoldable.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
                const pud_goal_list child_goals = this->goal_resolver.resolve_goal(parent_goals, goal, leaf_module->get_inference(child.node).body_goals);
                live.insert(child);
                leaf_goals.emplace(child, child_goals);
                if (child.module == 2 && !child_goals.goals.empty())
                    unfoldable.insert(child);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                    leaf_goals.erase(refuted);
                unfoldable.erase(refuted);
            }
        }
        EXPECT_LT(steps, 15000u);
        for (pud_global_node_id leaf : live) {
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == leaf.module)
                    leaf_module = &module;
            }
            EXPECT_EQ(leaf_goals.at(leaf), reference_pud_goal_list(*leaf_module, leaf.node));
        }
        std::set<pud_global_node_id> reference_unfoldable;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == leaf.module)
                    leaf_module = &module;
            }
            if (!reference_pud_goal_list(*leaf_module, leaf.node).goals.empty())
                reference_unfoldable.insert(leaf);
        }
        EXPECT_EQ(unfoldable, reference_unfoldable);

        std::set<const expr*> answers;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == leaf.module)
                    leaf_module = &module;
            }
            if (!leaf_goals.at(leaf).goals.empty())
                continue;
            answers.insert(reference_pud_answer(*leaf_module, leaf.node, this->exprs));
        }

        EXPECT_EQ(answers, expected);
    }
}

TYPED_TEST(PudAnswerSetsRuntimeSuite2IntegrationTest, CatalanMinimalCaseOneNode) {
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
    std::map<pud_global_node_id, pud_goal_list> leaf_goals;
    std::set<pud_global_node_id> unfoldable;
    for (pud_module& module : modules) {
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id leaf : change.offspring) {
            live.insert(leaf);
            leaf_goals.emplace(leaf, reference_pud_goal_list(module, leaf.node));
            if (leaf.module == 2 && !leaf_goals.at(leaf).goals.empty())
                unfoldable.insert(leaf);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_goals.erase(refuted);
            unfoldable.erase(refuted);
        }
    }
    size_t steps = 0;
    for (; steps < 500; ++steps) {
        if (unfoldable.empty())
            break;
        const pud_global_node_id chosen_leaf = *unfoldable.begin();
        const pud_module* leaf_module = nullptr;
        for (const pud_module& module : modules) {
            if (module.get_module_id() == chosen_leaf.module)
                leaf_module = &module;
        }
        std::optional<pud_body_goal_id> chosen_goal;
        for (const auto& [goal_id, goal] : leaf_goals.at(chosen_leaf).goals) {
            if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                chosen_goal = goal_id;
        }
        const pud_frontier_change change = runtime.unfold(chosen_leaf, *chosen_goal);
        const pud_goal_list parent_goals = leaf_goals.at(chosen_leaf);
        live.erase(chosen_leaf);
        leaf_goals.erase(chosen_leaf);
        unfoldable.erase(chosen_leaf);
        for (pud_global_node_id child : change.offspring) {
            const pud_goal_list child_goals = this->goal_resolver.resolve_goal(parent_goals, *chosen_goal, leaf_module->get_inference(child.node).body_goals);
            live.insert(child);
            leaf_goals.emplace(child, child_goals);
            if (child.module == 2 && !child_goals.goals.empty())
                unfoldable.insert(child);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_goals.erase(refuted);
            unfoldable.erase(refuted);
        }
    }
    EXPECT_LT(steps, 500u);
    for (pud_global_node_id leaf : live) {
        const pud_module* leaf_module = nullptr;
        for (const pud_module& module : modules) {
            if (module.get_module_id() == leaf.module)
                leaf_module = &module;
        }
        EXPECT_EQ(leaf_goals.at(leaf), reference_pud_goal_list(*leaf_module, leaf.node));
    }
    std::set<const expr*> answers;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        const pud_module* leaf_module = nullptr;
        for (const pud_module& module : modules) {
            if (module.get_module_id() == leaf.module)
                leaf_module = &module;
        }
        if (!leaf_goals.at(leaf).goals.empty())
            continue;
        answers.insert(reference_pud_answer(*leaf_module, leaf.node, this->exprs));
    }
    EXPECT_EQ(answers, (std::set<const expr*>{this->functor("q", {lone})}));
    // No legacy RuntimeParamTest counterpart; smoke coverage for Catalan-shaped rules at n=1.
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuite2IntegrationTest, EnumeratesCatalanTreesWithFiveNodes) {
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
    std::map<pud_global_node_id, pud_goal_list> leaf_goals;
    std::set<pud_global_node_id> unfoldable;
    for (pud_module& module : modules) {
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id leaf : change.offspring) {
            live.insert(leaf);
            leaf_goals.emplace(leaf, reference_pud_goal_list(module, leaf.node));
            if (leaf.module == 2 && !leaf_goals.at(leaf).goals.empty())
                unfoldable.insert(leaf);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_goals.erase(refuted);
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
        const pud_module* leaf_module = nullptr;
        for (const pud_module& module : modules) {
            if (module.get_module_id() == chosen_leaf.module)
                leaf_module = &module;
        }
        std::optional<pud_body_goal_id> chosen_goal;
        for (const auto& [goal_id, goal] : leaf_goals.at(chosen_leaf).goals) {
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
            const expr* chosen_expr = leaf_goals.at(chosen_leaf).goals.at(*chosen_goal);
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
        const pud_goal_list parent_goals = leaf_goals.at(chosen_leaf);
        live.erase(chosen_leaf);
        leaf_goals.erase(chosen_leaf);
        unfoldable.erase(chosen_leaf);
        for (pud_global_node_id child : change.offspring) {
            const pud_goal_list child_goals = this->goal_resolver.resolve_goal(parent_goals, *chosen_goal, leaf_module->get_inference(child.node).body_goals);
            live.insert(child);
            leaf_goals.emplace(child, child_goals);
            if (child.module == 2 && !child_goals.goals.empty())
                unfoldable.insert(child);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_goals.erase(refuted);
            unfoldable.erase(refuted);
        }
    }
    EXPECT_LT(steps, 200000u);
    for (pud_global_node_id leaf : live) {
        const pud_module* leaf_module = nullptr;
        for (const pud_module& module : modules) {
            if (module.get_module_id() == leaf.module)
                leaf_module = &module;
        }
        EXPECT_EQ(leaf_goals.at(leaf), reference_pud_goal_list(*leaf_module, leaf.node));
    }
    std::set<pud_global_node_id> reference_unfoldable;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        const pud_module* leaf_module = nullptr;
        for (const pud_module& module : modules) {
            if (module.get_module_id() == leaf.module)
                leaf_module = &module;
        }
        if (!reference_pud_goal_list(*leaf_module, leaf.node).goals.empty())
            reference_unfoldable.insert(leaf);
    }
    EXPECT_EQ(unfoldable, reference_unfoldable);

    std::set<const expr*> answers;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        const pud_module* leaf_module = nullptr;
        for (const pud_module& module : modules) {
            if (module.get_module_id() == leaf.module)
                leaf_module = &module;
        }
        if (!leaf_goals.at(leaf).goals.empty())
            continue;
        answers.insert(reference_pud_answer(*leaf_module, leaf.node, this->exprs));
    }

    EXPECT_EQ(answers, expected);
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuite2IntegrationTest, EnumeratesCatalanTreesWithFiveNodesInEveryUnfoldOrder) {
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
        std::map<pud_global_node_id, pud_goal_list> leaf_goals;
        std::set<pud_global_node_id> unfoldable;
        for (pud_module& module : modules) {
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id leaf : change.offspring) {
                live.insert(leaf);
                leaf_goals.emplace(leaf, reference_pud_goal_list(module, leaf.node));
                if (leaf.module == 2 && !leaf_goals.at(leaf).goals.empty())
                    unfoldable.insert(leaf);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                leaf_goals.erase(refuted);
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
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == leaf.module)
                    leaf_module = &module;
            }
            std::optional<pud_body_goal_id> chosen_goal;
            for (const auto& [goal_id, goal] : leaf_goals.at(leaf).goals) {
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
                const expr* chosen_expr = leaf_goals.at(leaf).goals.at(*chosen_goal);
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
            const pud_goal_list parent_goals = leaf_goals.at(leaf);
            live.erase(leaf);
            leaf_goals.erase(leaf);
            unfoldable.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
                const pud_goal_list child_goals = this->goal_resolver.resolve_goal(parent_goals, goal, leaf_module->get_inference(child.node).body_goals);
                live.insert(child);
                leaf_goals.emplace(child, child_goals);
                if (child.module == 2 && !child_goals.goals.empty())
                    unfoldable.insert(child);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                    leaf_goals.erase(refuted);
                unfoldable.erase(refuted);
            }
        }
        for (pud_global_node_id leaf : live) {
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == leaf.module)
                    leaf_module = &module;
            }
            EXPECT_EQ(leaf_goals.at(leaf), reference_pud_goal_list(*leaf_module, leaf.node));
        }
        std::set<pud_global_node_id> reference_unfoldable;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == leaf.module)
                    leaf_module = &module;
            }
            if (!reference_pud_goal_list(*leaf_module, leaf.node).goals.empty())
                reference_unfoldable.insert(leaf);
        }
        EXPECT_EQ(unfoldable, reference_unfoldable);

        std::set<const expr*> answers;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == leaf.module)
                    leaf_module = &module;
            }
            if (!leaf_goals.at(leaf).goals.empty())
                continue;
            answers.insert(reference_pud_answer(*leaf_module, leaf.node, this->exprs));
        }

        for (const expr* answer : answers) {
            EXPECT_TRUE(expected.contains(answer))
                << "seed " << seed << " produced an answer not in the expected set";
            found_across_seeds.insert(answer);
        }
    }
    EXPECT_EQ(found_across_seeds.size(), 42u);
}

TYPED_TEST(PudAnswerSetsRuntimeSuite2IntegrationTest, EnumeratesFourTwoGoalGroundCombinations) {
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
    std::map<pud_global_node_id, pud_goal_list> leaf_goals;
    std::set<pud_global_node_id> unfoldable;
    for (pud_module& module : modules) {
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id leaf : change.offspring) {
            live.insert(leaf);
            leaf_goals.emplace(leaf, reference_pud_goal_list(module, leaf.node));
            if (leaf.module == 2 && !leaf_goals.at(leaf).goals.empty())
                unfoldable.insert(leaf);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_goals.erase(refuted);
            unfoldable.erase(refuted);
        }
    }

    size_t steps = 0;
    for (; steps < 2000; ++steps) {
        if (unfoldable.empty())
            break;
        const pud_global_node_id chosen_leaf = *unfoldable.begin();
        const pud_module* leaf_module = nullptr;
        for (const pud_module& module : modules) {
            if (module.get_module_id() == chosen_leaf.module)
                leaf_module = &module;
        }
        std::optional<pud_body_goal_id> chosen_goal;
        for (const auto& [goal_id, goal] : leaf_goals.at(chosen_leaf).goals) {
            if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                chosen_goal = goal_id;
        }
        const pud_frontier_change change = runtime.unfold(chosen_leaf, *chosen_goal);
        const pud_goal_list parent_goals = leaf_goals.at(chosen_leaf);
        live.erase(chosen_leaf);
        leaf_goals.erase(chosen_leaf);
        unfoldable.erase(chosen_leaf);
        for (pud_global_node_id child : change.offspring) {
            const pud_goal_list child_goals = this->goal_resolver.resolve_goal(parent_goals, *chosen_goal, leaf_module->get_inference(child.node).body_goals);
            live.insert(child);
            leaf_goals.emplace(child, child_goals);
            if (child.module == 2 && !child_goals.goals.empty())
                unfoldable.insert(child);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_goals.erase(refuted);
            unfoldable.erase(refuted);
        }
    }
    EXPECT_LT(steps, 2000u);
    for (pud_global_node_id leaf : live) {
        const pud_module* leaf_module = nullptr;
        for (const pud_module& module : modules) {
            if (module.get_module_id() == leaf.module)
                leaf_module = &module;
        }
        EXPECT_EQ(leaf_goals.at(leaf), reference_pud_goal_list(*leaf_module, leaf.node));
    }
    std::set<pud_global_node_id> reference_unfoldable;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        const pud_module* leaf_module = nullptr;
        for (const pud_module& module : modules) {
            if (module.get_module_id() == leaf.module)
                leaf_module = &module;
        }
        if (!reference_pud_goal_list(*leaf_module, leaf.node).goals.empty())
            reference_unfoldable.insert(leaf);
    }
    EXPECT_EQ(unfoldable, reference_unfoldable);

    std::set<const expr*> answers;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        const pud_module* leaf_module = nullptr;
        for (const pud_module& module : modules) {
            if (module.get_module_id() == leaf.module)
                leaf_module = &module;
        }
        if (!leaf_goals.at(leaf).goals.empty())
            continue;
        answers.insert(reference_pud_answer(*leaf_module, leaf.node, this->exprs));
    }

    EXPECT_EQ(answers, (std::set<const expr*>{this->functor("q", {})}));
    size_t solved_live_leaves = 0;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        if (!leaf_goals.at(leaf).goals.empty())
            continue;
        ++solved_live_leaves;
    }
    EXPECT_EQ(solved_live_leaves, 4u);
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuite2IntegrationTest, EnumeratesFourTwoGoalGroundCombinationsInEveryUnfoldOrder) {
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
        std::map<pud_global_node_id, pud_goal_list> leaf_goals;
        std::set<pud_global_node_id> unfoldable;
        for (pud_module& module : modules) {
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id leaf : change.offspring) {
                live.insert(leaf);
                leaf_goals.emplace(leaf, reference_pud_goal_list(module, leaf.node));
                if (leaf.module == 2 && !leaf_goals.at(leaf).goals.empty())
                    unfoldable.insert(leaf);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                    leaf_goals.erase(refuted);
                unfoldable.erase(refuted);
            }
        }

        size_t steps = 0;
        for (; steps < 2000; ++steps) {
            if (unfoldable.empty())
                break;
            const pud_global_node_id leaf = *std::next(unfoldable.begin(), rng() % unfoldable.size());
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == leaf.module)
                    leaf_module = &module;
            }
            std::vector<pud_body_goal_id> goal_ids;
            for (const auto& [goal_id, goal] : leaf_goals.at(leaf).goals)
                goal_ids.push_back(goal_id);
            const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
            const pud_frontier_change change = runtime.unfold(leaf, goal);
            const pud_goal_list parent_goals = leaf_goals.at(leaf);
            live.erase(leaf);
            leaf_goals.erase(leaf);
            unfoldable.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
                const pud_goal_list child_goals = this->goal_resolver.resolve_goal(parent_goals, goal, leaf_module->get_inference(child.node).body_goals);
                live.insert(child);
                leaf_goals.emplace(child, child_goals);
                if (child.module == 2 && !child_goals.goals.empty())
                    unfoldable.insert(child);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                    leaf_goals.erase(refuted);
                unfoldable.erase(refuted);
            }
        }
        EXPECT_LT(steps, 2000u);
        for (pud_global_node_id leaf : live) {
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == leaf.module)
                    leaf_module = &module;
            }
            EXPECT_EQ(leaf_goals.at(leaf), reference_pud_goal_list(*leaf_module, leaf.node));
        }
        std::set<pud_global_node_id> reference_unfoldable;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == leaf.module)
                    leaf_module = &module;
            }
            if (!reference_pud_goal_list(*leaf_module, leaf.node).goals.empty())
                reference_unfoldable.insert(leaf);
        }
        EXPECT_EQ(unfoldable, reference_unfoldable);

        std::set<const expr*> answers;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == leaf.module)
                    leaf_module = &module;
            }
            if (!leaf_goals.at(leaf).goals.empty())
                continue;
            answers.insert(reference_pud_answer(*leaf_module, leaf.node, this->exprs));
        }

        EXPECT_EQ(answers, expected);
        size_t solved_live_leaves = 0;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            if (!leaf_goals.at(leaf).goals.empty())
                continue;
            ++solved_live_leaves;
        }
        EXPECT_EQ(solved_live_leaves, 4u);
    }
}

TYPED_TEST(PudAnswerSetsRuntimeSuite2IntegrationTest, EnumeratesEightThreeGoalGroundCombinations) {
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
    std::map<pud_global_node_id, pud_goal_list> leaf_goals;
    std::set<pud_global_node_id> unfoldable;
    for (pud_module& module : modules) {
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id leaf : change.offspring) {
            live.insert(leaf);
            leaf_goals.emplace(leaf, reference_pud_goal_list(module, leaf.node));
            if (leaf.module == 2 && !leaf_goals.at(leaf).goals.empty())
                unfoldable.insert(leaf);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_goals.erase(refuted);
            unfoldable.erase(refuted);
        }
    }

    size_t steps = 0;
    for (; steps < 2000; ++steps) {
        if (unfoldable.empty())
            break;
        const pud_global_node_id chosen_leaf = *unfoldable.begin();
        const pud_module* leaf_module = nullptr;
        for (const pud_module& module : modules) {
            if (module.get_module_id() == chosen_leaf.module)
                leaf_module = &module;
        }
        std::optional<pud_body_goal_id> chosen_goal;
        for (const auto& [goal_id, goal] : leaf_goals.at(chosen_leaf).goals) {
            if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                chosen_goal = goal_id;
        }
        const pud_frontier_change change = runtime.unfold(chosen_leaf, *chosen_goal);
        const pud_goal_list parent_goals = leaf_goals.at(chosen_leaf);
        live.erase(chosen_leaf);
        leaf_goals.erase(chosen_leaf);
        unfoldable.erase(chosen_leaf);
        for (pud_global_node_id child : change.offspring) {
            const pud_goal_list child_goals = this->goal_resolver.resolve_goal(parent_goals, *chosen_goal, leaf_module->get_inference(child.node).body_goals);
            live.insert(child);
            leaf_goals.emplace(child, child_goals);
            if (child.module == 2 && !child_goals.goals.empty())
                unfoldable.insert(child);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_goals.erase(refuted);
            unfoldable.erase(refuted);
        }
    }
    EXPECT_LT(steps, 2000u);
    for (pud_global_node_id leaf : live) {
        const pud_module* leaf_module = nullptr;
        for (const pud_module& module : modules) {
            if (module.get_module_id() == leaf.module)
                leaf_module = &module;
        }
        EXPECT_EQ(leaf_goals.at(leaf), reference_pud_goal_list(*leaf_module, leaf.node));
    }
    std::set<pud_global_node_id> reference_unfoldable;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        const pud_module* leaf_module = nullptr;
        for (const pud_module& module : modules) {
            if (module.get_module_id() == leaf.module)
                leaf_module = &module;
        }
        if (!reference_pud_goal_list(*leaf_module, leaf.node).goals.empty())
            reference_unfoldable.insert(leaf);
    }
    EXPECT_EQ(unfoldable, reference_unfoldable);

    std::set<const expr*> answers;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        const pud_module* leaf_module = nullptr;
        for (const pud_module& module : modules) {
            if (module.get_module_id() == leaf.module)
                leaf_module = &module;
        }
        if (!leaf_goals.at(leaf).goals.empty())
            continue;
        answers.insert(reference_pud_answer(*leaf_module, leaf.node, this->exprs));
    }

    EXPECT_EQ(answers, (std::set<const expr*>{this->functor("q", {})}));
    size_t solved_live_leaves = 0;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        if (!leaf_goals.at(leaf).goals.empty())
            continue;
        ++solved_live_leaves;
    }
    EXPECT_EQ(solved_live_leaves, 8u);
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuite2IntegrationTest, EnumeratesEightThreeGoalGroundCombinationsInEveryUnfoldOrder) {
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
        std::map<pud_global_node_id, pud_goal_list> leaf_goals;
        std::set<pud_global_node_id> unfoldable;
        for (pud_module& module : modules) {
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id leaf : change.offspring) {
                live.insert(leaf);
                leaf_goals.emplace(leaf, reference_pud_goal_list(module, leaf.node));
                if (leaf.module == 2 && !leaf_goals.at(leaf).goals.empty())
                    unfoldable.insert(leaf);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                    leaf_goals.erase(refuted);
                unfoldable.erase(refuted);
            }
        }

        size_t steps = 0;
        for (; steps < 2000; ++steps) {
            if (unfoldable.empty())
                break;
            const pud_global_node_id leaf = *std::next(unfoldable.begin(), rng() % unfoldable.size());
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == leaf.module)
                    leaf_module = &module;
            }
            std::vector<pud_body_goal_id> goal_ids;
            for (const auto& [goal_id, goal] : leaf_goals.at(leaf).goals)
                goal_ids.push_back(goal_id);
            const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
            const pud_frontier_change change = runtime.unfold(leaf, goal);
            const pud_goal_list parent_goals = leaf_goals.at(leaf);
            live.erase(leaf);
            leaf_goals.erase(leaf);
            unfoldable.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
                const pud_goal_list child_goals = this->goal_resolver.resolve_goal(parent_goals, goal, leaf_module->get_inference(child.node).body_goals);
                live.insert(child);
                leaf_goals.emplace(child, child_goals);
                if (child.module == 2 && !child_goals.goals.empty())
                    unfoldable.insert(child);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                    leaf_goals.erase(refuted);
                unfoldable.erase(refuted);
            }
        }
        EXPECT_LT(steps, 2000u);
        for (pud_global_node_id leaf : live) {
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == leaf.module)
                    leaf_module = &module;
            }
            EXPECT_EQ(leaf_goals.at(leaf), reference_pud_goal_list(*leaf_module, leaf.node));
        }
        std::set<pud_global_node_id> reference_unfoldable;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == leaf.module)
                    leaf_module = &module;
            }
            if (!reference_pud_goal_list(*leaf_module, leaf.node).goals.empty())
                reference_unfoldable.insert(leaf);
        }
        EXPECT_EQ(unfoldable, reference_unfoldable);

        std::set<const expr*> answers;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == leaf.module)
                    leaf_module = &module;
            }
            if (!leaf_goals.at(leaf).goals.empty())
                continue;
            answers.insert(reference_pud_answer(*leaf_module, leaf.node, this->exprs));
        }

        EXPECT_EQ(answers, expected);
        size_t solved_live_leaves = 0;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            if (!leaf_goals.at(leaf).goals.empty())
                continue;
            ++solved_live_leaves;
        }
        EXPECT_EQ(solved_live_leaves, 8u);
    }
}

TYPED_TEST(PudAnswerSetsRuntimeSuite2IntegrationTest, EnumeratesManySharedVarGroundHeads) {
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
    std::map<pud_global_node_id, pud_goal_list> leaf_goals;
    std::set<pud_global_node_id> unfoldable;
    for (pud_module& module : modules) {
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id leaf : change.offspring) {
            live.insert(leaf);
            leaf_goals.emplace(leaf, reference_pud_goal_list(module, leaf.node));
            if (leaf.module == 2 && !leaf_goals.at(leaf).goals.empty())
                unfoldable.insert(leaf);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_goals.erase(refuted);
            unfoldable.erase(refuted);
        }
    }

    size_t steps = 0;
    for (; steps < 2000; ++steps) {
        if (unfoldable.empty())
            break;
        const pud_global_node_id chosen_leaf = *unfoldable.begin();
        const pud_module* leaf_module = nullptr;
        for (const pud_module& module : modules) {
            if (module.get_module_id() == chosen_leaf.module)
                leaf_module = &module;
        }
        std::optional<pud_body_goal_id> chosen_goal;
        for (const auto& [goal_id, goal] : leaf_goals.at(chosen_leaf).goals) {
            if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                chosen_goal = goal_id;
        }
        const pud_frontier_change change = runtime.unfold(chosen_leaf, *chosen_goal);
        const pud_goal_list parent_goals = leaf_goals.at(chosen_leaf);
        live.erase(chosen_leaf);
        leaf_goals.erase(chosen_leaf);
        unfoldable.erase(chosen_leaf);
        for (pud_global_node_id child : change.offspring) {
            const pud_goal_list child_goals = this->goal_resolver.resolve_goal(parent_goals, *chosen_goal, leaf_module->get_inference(child.node).body_goals);
            live.insert(child);
            leaf_goals.emplace(child, child_goals);
            if (child.module == 2 && !child_goals.goals.empty())
                unfoldable.insert(child);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_goals.erase(refuted);
            unfoldable.erase(refuted);
        }
    }
    EXPECT_LT(steps, 2000u);
    for (pud_global_node_id leaf : live) {
        const pud_module* leaf_module = nullptr;
        for (const pud_module& module : modules) {
            if (module.get_module_id() == leaf.module)
                leaf_module = &module;
        }
        EXPECT_EQ(leaf_goals.at(leaf), reference_pud_goal_list(*leaf_module, leaf.node));
    }
    std::set<pud_global_node_id> reference_unfoldable;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        const pud_module* leaf_module = nullptr;
        for (const pud_module& module : modules) {
            if (module.get_module_id() == leaf.module)
                leaf_module = &module;
        }
        if (!reference_pud_goal_list(*leaf_module, leaf.node).goals.empty())
            reference_unfoldable.insert(leaf);
    }
    EXPECT_EQ(unfoldable, reference_unfoldable);

    std::set<const expr*> answers;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        const pud_module* leaf_module = nullptr;
        for (const pud_module& module : modules) {
            if (module.get_module_id() == leaf.module)
                leaf_module = &module;
        }
        if (!leaf_goals.at(leaf).goals.empty())
            continue;
        answers.insert(reference_pud_answer(*leaf_module, leaf.node, this->exprs));
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

TYPED_TEST(PudAnswerSetsRuntimeSuite2IntegrationTest, EnumeratesManySharedVarGroundHeadsInEveryUnfoldOrder) {
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
        std::map<pud_global_node_id, pud_goal_list> leaf_goals;
        std::set<pud_global_node_id> unfoldable;
        for (pud_module& module : modules) {
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id leaf : change.offspring) {
                live.insert(leaf);
                leaf_goals.emplace(leaf, reference_pud_goal_list(module, leaf.node));
                if (leaf.module == 2 && !leaf_goals.at(leaf).goals.empty())
                    unfoldable.insert(leaf);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                    leaf_goals.erase(refuted);
                unfoldable.erase(refuted);
            }
        }

        size_t steps = 0;
        for (; steps < 2000; ++steps) {
            if (unfoldable.empty())
                break;
            const pud_global_node_id leaf = *std::next(unfoldable.begin(), rng() % unfoldable.size());
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == leaf.module)
                    leaf_module = &module;
            }
            std::vector<pud_body_goal_id> goal_ids;
            for (const auto& [goal_id, goal] : leaf_goals.at(leaf).goals)
                goal_ids.push_back(goal_id);
            const pud_body_goal_id goal = goal_ids.at(rng() % goal_ids.size());
            const pud_frontier_change change = runtime.unfold(leaf, goal);
            const pud_goal_list parent_goals = leaf_goals.at(leaf);
            live.erase(leaf);
            leaf_goals.erase(leaf);
            unfoldable.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
                const pud_goal_list child_goals = this->goal_resolver.resolve_goal(parent_goals, goal, leaf_module->get_inference(child.node).body_goals);
                live.insert(child);
                leaf_goals.emplace(child, child_goals);
                if (child.module == 2 && !child_goals.goals.empty())
                    unfoldable.insert(child);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                    leaf_goals.erase(refuted);
                unfoldable.erase(refuted);
            }
        }
        EXPECT_LT(steps, 2000u);
        for (pud_global_node_id leaf : live) {
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == leaf.module)
                    leaf_module = &module;
            }
            EXPECT_EQ(leaf_goals.at(leaf), reference_pud_goal_list(*leaf_module, leaf.node));
        }
        std::set<pud_global_node_id> reference_unfoldable;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == leaf.module)
                    leaf_module = &module;
            }
            if (!reference_pud_goal_list(*leaf_module, leaf.node).goals.empty())
                reference_unfoldable.insert(leaf);
        }
        EXPECT_EQ(unfoldable, reference_unfoldable);

        std::set<const expr*> answers;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == leaf.module)
                    leaf_module = &module;
            }
            if (!leaf_goals.at(leaf).goals.empty())
                continue;
            answers.insert(reference_pud_answer(*leaf_module, leaf.node, this->exprs));
        }

        EXPECT_EQ(answers, expected);
    }
}


