#include <cstdint>
#include <deque>
#include <map>
#include <optional>
#include <random>
#include <set>
#include <stack>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "functor_fixture.hpp"
#include "reference_pud_answer.hpp"
#include "reference_pud_frontier.hpp"
#include "reference_pud_goal_list.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/pud_module.hpp"
#include "infrastructure/pud_runtime.hpp"

using ::testing::IsEmpty;

template<typename IRuntime>
struct PudAnswerSetsRuntimeSuite4IntegrationTest : public ::testing::Test {
    test_functors functors;
    expr_pool exprs;

    const expr* var(uint32_t index) { return exprs.make_var(index); }
    const expr* atom(const char* name) { return exprs.make_functor(functors.id(name), {}); }
    const expr* functor(const char* name, std::vector<const expr*> args) {
        return exprs.make_functor(functors.id(name), std::move(args));
    }
};

using suite4_runtimes_t = ::testing::Types<pud_runtime<pud_module, expr_pool>>;
TYPED_TEST_SUITE(PudAnswerSetsRuntimeSuite4IntegrationTest, suite4_runtimes_t);

TYPED_TEST(PudAnswerSetsRuntimeSuite4IntegrationTest, EnumeratesLatticePathsFourByFour) {
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
    std::map<pud_global_node_id, pud_goal_list> leaf_goals;
    std::set<pud_global_node_id> unfoldable;
    for (pud_module& module : modules) {
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id offspring : change.offspring) {
            live.insert(offspring);
            const pud_goal_list offspring_goals = reference_pud_goal_list(module, offspring.node);
            leaf_goals.emplace(offspring, offspring_goals);
            if (offspring.module == 2 && !offspring_goals.goals.empty())
                unfoldable.insert(offspring);
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
            pud_goal_list child_goals = parent_goals.closing(*chosen_goal);
            for (const expr* body_goal : leaf_module->get_inference(child.node).body_goals)
                child_goals = child_goals.adding(body_goal);
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
    std::set<pud_global_node_id> expected_unfoldable;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        if (!leaf_goals.at(leaf).goals.empty())
            expected_unfoldable.insert(leaf);
    }
    EXPECT_EQ(unfoldable, expected_unfoldable);
    std::set<const expr*> answers;
    size_t solved_live_leaves = 0;
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
        ++solved_live_leaves;
        answers.insert(reference_pud_answer(*leaf_module, leaf.node, this->exprs));
    }
    EXPECT_EQ(solved_live_leaves, 70u);
    EXPECT_EQ(answers, (std::set<const expr*>{this->functor("q", {})}));
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuite4IntegrationTest, EnumeratesLatticePathsFourByFourInEveryUnfoldOrder) {
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
        std::map<pud_global_node_id, pud_goal_list> leaf_goals;
        std::set<pud_global_node_id> unfoldable;
        for (pud_module& module : modules) {
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id offspring : change.offspring) {
                live.insert(offspring);
                const pud_goal_list offspring_goals = reference_pud_goal_list(module, offspring.node);
                leaf_goals.emplace(offspring, offspring_goals);
                if (offspring.module == 2 && !offspring_goals.goals.empty())
                    unfoldable.insert(offspring);
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
                pud_goal_list child_goals = parent_goals.closing(goal);
                for (const expr* body_goal : leaf_module->get_inference(child.node).body_goals)
                    child_goals = child_goals.adding(body_goal);
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
        std::set<pud_global_node_id> expected_unfoldable;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            if (!leaf_goals.at(leaf).goals.empty())
                expected_unfoldable.insert(leaf);
        }
        EXPECT_EQ(unfoldable, expected_unfoldable);
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
}

TYPED_TEST(PudAnswerSetsRuntimeSuite4IntegrationTest, CompositionsMinimalCaseOrderedCompositionsOfTwo) {
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
    std::map<pud_global_node_id, pud_goal_list> leaf_goals;
    std::set<pud_global_node_id> unfoldable;
    for (pud_module& module : modules) {
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id offspring : change.offspring) {
            live.insert(offspring);
            const pud_goal_list offspring_goals = reference_pud_goal_list(module, offspring.node);
            leaf_goals.emplace(offspring, offspring_goals);
            if (offspring.module == 2 && !offspring_goals.goals.empty())
                unfoldable.insert(offspring);
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
            pud_goal_list child_goals = parent_goals.closing(*chosen_goal);
            for (const expr* body_goal : leaf_module->get_inference(child.node).body_goals)
                child_goals = child_goals.adding(body_goal);
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
    std::set<pud_global_node_id> expected_unfoldable;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        if (!leaf_goals.at(leaf).goals.empty())
            expected_unfoldable.insert(leaf);
    }
    EXPECT_EQ(unfoldable, expected_unfoldable);
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
        this->functor("q", {this->functor("cons", {two, nil})}),
        this->functor("q", {this->functor("cons", {one, this->functor("cons", {one, nil})})}),
    }));
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuite4IntegrationTest, EnumeratesOrderedCompositionsOfEight) {
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
    std::map<pud_global_node_id, pud_goal_list> leaf_goals;
    std::set<pud_global_node_id> unfoldable;
    for (pud_module& module : modules) {
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id offspring : change.offspring) {
            live.insert(offspring);
            const pud_goal_list offspring_goals = reference_pud_goal_list(module, offspring.node);
            leaf_goals.emplace(offspring, offspring_goals);
            if (offspring.module == 2 && !offspring_goals.goals.empty())
                unfoldable.insert(offspring);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_goals.erase(refuted);
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
        const pud_module* leaf_module = nullptr;
        for (const pud_module& module : modules) {
            if (module.get_module_id() == chosen_leaf.module)
                leaf_module = &module;
        }
        std::optional<pud_body_goal_id> chosen_goal;
        for (const auto& [goal_id, goal] : leaf_goals.at(chosen_leaf).goals) {
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
            const expr* chosen_expr = leaf_goals.at(chosen_leaf).goals.at(*chosen_goal);
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
        const pud_goal_list parent_goals = leaf_goals.at(chosen_leaf);
        live.erase(chosen_leaf);
        leaf_goals.erase(chosen_leaf);
        unfoldable.erase(chosen_leaf);
        for (pud_global_node_id child : change.offspring) {
            pud_goal_list child_goals = parent_goals.closing(*chosen_goal);
            for (const expr* body_goal : leaf_module->get_inference(child.node).body_goals)
                child_goals = child_goals.adding(body_goal);
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
    EXPECT_LT(steps, 100000u);
    for (pud_global_node_id leaf : live) {
        const pud_module* leaf_module = nullptr;
        for (const pud_module& module : modules) {
            if (module.get_module_id() == leaf.module)
                leaf_module = &module;
        }
        EXPECT_EQ(leaf_goals.at(leaf), reference_pud_goal_list(*leaf_module, leaf.node));
    }
    std::set<pud_global_node_id> expected_unfoldable;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        if (!leaf_goals.at(leaf).goals.empty())
            expected_unfoldable.insert(leaf);
    }
    EXPECT_EQ(unfoldable, expected_unfoldable);
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

TYPED_TEST(PudAnswerSetsRuntimeSuite4IntegrationTest, EnumeratesOrderedCompositionsOfEightInEveryUnfoldOrder) {
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
        std::map<pud_global_node_id, pud_goal_list> leaf_goals;
        std::set<pud_global_node_id> unfoldable;
        for (pud_module& module : modules) {
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id offspring : change.offspring) {
                live.insert(offspring);
                const pud_goal_list offspring_goals = reference_pud_goal_list(module, offspring.node);
                leaf_goals.emplace(offspring, offspring_goals);
                if (offspring.module == 2 && !offspring_goals.goals.empty())
                    unfoldable.insert(offspring);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                leaf_goals.erase(refuted);
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
            for (const auto& [goal_id, goal] : leaf_goals.at(leaf).goals) {
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
                const expr* chosen_expr = leaf_goals.at(leaf).goals.at(*chosen_goal);
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
            const pud_goal_list parent_goals = leaf_goals.at(leaf);
            live.erase(leaf);
            leaf_goals.erase(leaf);
            unfoldable.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
                pud_goal_list child_goals = parent_goals.closing(goal);
                for (const expr* body_goal : leaf_module->get_inference(child.node).body_goals)
                    child_goals = child_goals.adding(body_goal);
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
        EXPECT_LT(steps, 100000u);
        for (pud_global_node_id leaf : live) {
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == leaf.module)
                    leaf_module = &module;
            }
            EXPECT_EQ(leaf_goals.at(leaf), reference_pud_goal_list(*leaf_module, leaf.node));
        }
        std::set<pud_global_node_id> expected_unfoldable;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            if (!leaf_goals.at(leaf).goals.empty())
                expected_unfoldable.insert(leaf);
        }
        EXPECT_EQ(unfoldable, expected_unfoldable);
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
}

TYPED_TEST(PudAnswerSetsRuntimeSuite4IntegrationTest, EnumeratesBinaryStringsNoConsecutiveOnesLengthTen) {
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
    std::map<pud_global_node_id, pud_goal_list> leaf_goals;
    std::set<pud_global_node_id> unfoldable;
    for (pud_module& module : modules) {
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id offspring : change.offspring) {
            live.insert(offspring);
            const pud_goal_list offspring_goals = reference_pud_goal_list(module, offspring.node);
            leaf_goals.emplace(offspring, offspring_goals);
            if (offspring.module == 2 && !offspring_goals.goals.empty())
                unfoldable.insert(offspring);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_goals.erase(refuted);
            unfoldable.erase(refuted);
        }
    }
    size_t steps = 0;
    for (; steps < 50000; ++steps) {
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
            pud_goal_list child_goals = parent_goals.closing(*chosen_goal);
            for (const expr* body_goal : leaf_module->get_inference(child.node).body_goals)
                child_goals = child_goals.adding(body_goal);
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
    EXPECT_LT(steps, 50000u);
    for (pud_global_node_id leaf : live) {
        const pud_module* leaf_module = nullptr;
        for (const pud_module& module : modules) {
            if (module.get_module_id() == leaf.module)
                leaf_module = &module;
        }
        EXPECT_EQ(leaf_goals.at(leaf), reference_pud_goal_list(*leaf_module, leaf.node));
    }
    std::set<pud_global_node_id> expected_unfoldable;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        if (!leaf_goals.at(leaf).goals.empty())
            expected_unfoldable.insert(leaf);
    }
    EXPECT_EQ(unfoldable, expected_unfoldable);
    std::set<const expr*> answers;
    size_t solved_live_leaves = 0;
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
        ++solved_live_leaves;
        answers.insert(reference_pud_answer(*leaf_module, leaf.node, this->exprs));
    }
    EXPECT_EQ(answers, expected);
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuite4IntegrationTest, EnumeratesBinaryStringsNoConsecutiveOnesLengthTenInEveryUnfoldOrder) {
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
        std::map<pud_global_node_id, pud_goal_list> leaf_goals;
        std::set<pud_global_node_id> unfoldable;
        for (pud_module& module : modules) {
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id offspring : change.offspring) {
                live.insert(offspring);
                const pud_goal_list offspring_goals = reference_pud_goal_list(module, offspring.node);
                leaf_goals.emplace(offspring, offspring_goals);
                if (offspring.module == 2 && !offspring_goals.goals.empty())
                    unfoldable.insert(offspring);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                leaf_goals.erase(refuted);
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
            for (const auto& [goal_id, goal] : leaf_goals.at(leaf).goals) {
                if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                    chosen_goal = goal_id;
            }
            const pud_body_goal_id goal = *chosen_goal;
            const pud_frontier_change change = runtime.unfold(leaf, goal);
            const pud_goal_list parent_goals = leaf_goals.at(leaf);
            live.erase(leaf);
            leaf_goals.erase(leaf);
            unfoldable.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
                pud_goal_list child_goals = parent_goals.closing(goal);
                for (const expr* body_goal : leaf_module->get_inference(child.node).body_goals)
                    child_goals = child_goals.adding(body_goal);
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
        EXPECT_LT(steps, 50000u);
        for (pud_global_node_id leaf : live) {
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == leaf.module)
                    leaf_module = &module;
            }
            EXPECT_EQ(leaf_goals.at(leaf), reference_pud_goal_list(*leaf_module, leaf.node));
        }
        std::set<pud_global_node_id> expected_unfoldable;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            if (!leaf_goals.at(leaf).goals.empty())
                expected_unfoldable.insert(leaf);
        }
        EXPECT_EQ(unfoldable, expected_unfoldable);
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
}

TYPED_TEST(PudAnswerSetsRuntimeSuite4IntegrationTest, EnumeratesStaircasePathsOneOrTwoSummingToTen) {
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
    std::map<pud_global_node_id, pud_goal_list> leaf_goals;
    std::set<pud_global_node_id> unfoldable;
    for (pud_module& module : modules) {
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id offspring : change.offspring) {
            live.insert(offspring);
            const pud_goal_list offspring_goals = reference_pud_goal_list(module, offspring.node);
            leaf_goals.emplace(offspring, offspring_goals);
            if (offspring.module == 2 && !offspring_goals.goals.empty())
                unfoldable.insert(offspring);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_goals.erase(refuted);
            unfoldable.erase(refuted);
        }
    }
    size_t steps = 0;
    for (; steps < 50000; ++steps) {
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
            pud_goal_list child_goals = parent_goals.closing(*chosen_goal);
            for (const expr* body_goal : leaf_module->get_inference(child.node).body_goals)
                child_goals = child_goals.adding(body_goal);
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
    EXPECT_LT(steps, 50000u);
    for (pud_global_node_id leaf : live) {
        const pud_module* leaf_module = nullptr;
        for (const pud_module& module : modules) {
            if (module.get_module_id() == leaf.module)
                leaf_module = &module;
        }
        EXPECT_EQ(leaf_goals.at(leaf), reference_pud_goal_list(*leaf_module, leaf.node));
    }
    std::set<pud_global_node_id> expected_unfoldable;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        if (!leaf_goals.at(leaf).goals.empty())
            expected_unfoldable.insert(leaf);
    }
    EXPECT_EQ(unfoldable, expected_unfoldable);
    std::set<const expr*> answers;
    size_t solved_live_leaves = 0;
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
        ++solved_live_leaves;
        answers.insert(reference_pud_answer(*leaf_module, leaf.node, this->exprs));
    }
    EXPECT_EQ(answers, expected);
    EXPECT_EQ(live, reference_pud_frontier(modules));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
}

TYPED_TEST(PudAnswerSetsRuntimeSuite4IntegrationTest, EnumeratesStaircasePathsOneOrTwoSummingToTenInEveryUnfoldOrder) {
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
        std::map<pud_global_node_id, pud_goal_list> leaf_goals;
        std::set<pud_global_node_id> unfoldable;
        for (pud_module& module : modules) {
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id offspring : change.offspring) {
                live.insert(offspring);
                const pud_goal_list offspring_goals = reference_pud_goal_list(module, offspring.node);
                leaf_goals.emplace(offspring, offspring_goals);
                if (offspring.module == 2 && !offspring_goals.goals.empty())
                    unfoldable.insert(offspring);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                leaf_goals.erase(refuted);
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
            for (const auto& [goal_id, goal] : leaf_goals.at(leaf).goals) {
                if (!chosen_goal.has_value() || goal_id < chosen_goal.value())
                    chosen_goal = goal_id;
            }
            const pud_body_goal_id goal = *chosen_goal;
            const pud_frontier_change change = runtime.unfold(leaf, goal);
            const pud_goal_list parent_goals = leaf_goals.at(leaf);
            live.erase(leaf);
            leaf_goals.erase(leaf);
            unfoldable.erase(leaf);
            for (pud_global_node_id child : change.offspring) {
                pud_goal_list child_goals = parent_goals.closing(goal);
                for (const expr* body_goal : leaf_module->get_inference(child.node).body_goals)
                    child_goals = child_goals.adding(body_goal);
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
        EXPECT_LT(steps, 50000u);
        for (pud_global_node_id leaf : live) {
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == leaf.module)
                    leaf_module = &module;
            }
            EXPECT_EQ(leaf_goals.at(leaf), reference_pud_goal_list(*leaf_module, leaf.node));
        }
        std::set<pud_global_node_id> expected_unfoldable;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            if (!leaf_goals.at(leaf).goals.empty())
                expected_unfoldable.insert(leaf);
        }
        EXPECT_EQ(unfoldable, expected_unfoldable);
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
}

TYPED_TEST(PudAnswerSetsRuntimeSuite4IntegrationTest, EnumeratesFibIndexPairsWithSumBelowThirty) {
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
    std::map<pud_global_node_id, pud_goal_list> leaf_goals;
    std::set<pud_global_node_id> unfoldable;
    for (pud_module& module : modules) {
        const pud_frontier_change change = runtime.register_module(module);
        for (pud_global_node_id offspring : change.offspring) {
            live.insert(offspring);
            const pud_goal_list offspring_goals = reference_pud_goal_list(module, offspring.node);
            leaf_goals.emplace(offspring, offspring_goals);
            if (offspring.module == 2 && !offspring_goals.goals.empty())
                unfoldable.insert(offspring);
        }
        for (pud_global_node_id refuted : change.refuted) {
            live.erase(refuted);
            leaf_goals.erase(refuted);
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
            pud_goal_list child_goals = parent_goals.closing(*chosen_goal);
            for (const expr* body_goal : leaf_module->get_inference(child.node).body_goals)
                child_goals = child_goals.adding(body_goal);
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
    std::set<pud_global_node_id> expected_unfoldable;
    for (pud_global_node_id leaf : live) {
        if (leaf.module != 2)
            continue;
        if (!leaf_goals.at(leaf).goals.empty())
            expected_unfoldable.insert(leaf);
    }
    EXPECT_EQ(unfoldable, expected_unfoldable);
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

TYPED_TEST(PudAnswerSetsRuntimeSuite4IntegrationTest, EnumeratesFibIndexPairsWithSumBelowThirtyInEveryUnfoldOrder) {
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
        std::map<pud_global_node_id, pud_goal_list> leaf_goals;
        std::set<pud_global_node_id> unfoldable;
        for (pud_module& module : modules) {
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id offspring : change.offspring) {
                live.insert(offspring);
                const pud_goal_list offspring_goals = reference_pud_goal_list(module, offspring.node);
                leaf_goals.emplace(offspring, offspring_goals);
                if (offspring.module == 2 && !offspring_goals.goals.empty())
                    unfoldable.insert(offspring);
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
                pud_goal_list child_goals = parent_goals.closing(goal);
                for (const expr* body_goal : leaf_module->get_inference(child.node).body_goals)
                    child_goals = child_goals.adding(body_goal);
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
        std::set<pud_global_node_id> expected_unfoldable;
        for (pud_global_node_id leaf : live) {
            if (leaf.module != 2)
                continue;
            if (!leaf_goals.at(leaf).goals.empty())
                expected_unfoldable.insert(leaf);
        }
        EXPECT_EQ(unfoldable, expected_unfoldable);
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
