#include <cstdint>
#include <deque>
#include <optional>
#include <random>
#include <map>
#include <set>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "functor_fixture.hpp"
#include "reference_pud_goal_list.hpp"
#include "reference_pud_inference_violations.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/pud_module.hpp"
#include "infrastructure/pud_goal_resolver.hpp"
#include "infrastructure/pud_runtime.hpp"

using ::testing::IsEmpty;

template<typename IRuntime>
struct PudInferenceInvariantsIntegrationTest : public ::testing::Test {
    test_functors functors;
    expr_pool exprs;
    pud_goal_resolver goal_resolver;

    const expr* var(uint32_t index) { return exprs.make_var(index); }
    const expr* atom(const char* name) { return exprs.make_functor(functors.id(name), {}); }
    const expr* functor(const char* name, std::vector<const expr*> args) {
        return exprs.make_functor(functors.id(name), std::move(args));
    }
};

using inference_invariant_runtimes_t = ::testing::Types<pud_runtime<pud_module, expr_pool>>;
TYPED_TEST_SUITE(PudInferenceInvariantsIntegrationTest, inference_invariant_runtimes_t);

TYPED_TEST(PudInferenceInvariantsIntegrationTest, ColorsInferencesStayValidUnderRandomUnfolds) {
    for (uint32_t seed = 0; seed < 30; ++seed) {
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
        std::map<pud_global_node_id, pud_goal_list> leaf_goals;
        for (pud_module& module : modules) {
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id leaf : change.offspring) {
                live.insert(leaf);
                leaf_goals.emplace(leaf, reference_pud_goal_list(module, leaf.node));
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                leaf_goals.erase(refuted);
            }
        }
        size_t steps = 0;
        for (; steps < 150; ++steps) {
            std::vector<pud_global_node_id> unfoldable;
            for (pud_global_node_id leaf : live) {
                const pud_module* leaf_module = nullptr;
                for (const pud_module& module : modules) {
                    if (module.get_module_id() == leaf.module)
                        leaf_module = &module;
                }
                if (leaf_goals.at(leaf).goals.empty())
                    continue;
                unfoldable.push_back(leaf);
            }
            if (unfoldable.empty())
                break;
            std::uniform_int_distribution<size_t> leaf_dist(0, unfoldable.size() - 1);
            const pud_global_node_id chosen_leaf = unfoldable.at(leaf_dist(rng));
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == chosen_leaf.module)
                    leaf_module = &module;
            }
            std::vector<pud_body_goal_id> goal_ids;
            for (const auto& [goal_id, goal] : leaf_goals.at(chosen_leaf).goals)
                goal_ids.push_back(goal_id);
            std::uniform_int_distribution<size_t> goal_dist(0, goal_ids.size() - 1);
            const pud_body_goal_id chosen_goal = goal_ids.at(goal_dist(rng));
            const pud_goal_list parent_goals = leaf_goals.at(chosen_leaf);
            const pud_frontier_change change = runtime.unfold(chosen_leaf, chosen_goal);
            live.erase(chosen_leaf);
            leaf_goals.erase(chosen_leaf);
            for (pud_global_node_id child : change.offspring) {
                const pud_goal_list child_goals = this->goal_resolver.resolve_goal(
                    parent_goals,
                    chosen_goal,
                    leaf_module->get_inference(child.node).body_goals);
                live.insert(child);
                leaf_goals.emplace(child, child_goals);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                leaf_goals.erase(refuted);
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
        for (const pud_module& checked_module : modules)
            EXPECT_THAT(reference_pud_inference_violations(checked_module), IsEmpty()) << "seed " << seed;
    }
}

TYPED_TEST(PudInferenceInvariantsIntegrationTest, PeanoAdditionInferencesStayValidUnderRandomUnfolds) {
    for (uint32_t seed = 0; seed < 30; ++seed) {
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
        std::map<pud_global_node_id, pud_goal_list> leaf_goals;
        for (pud_module& module : modules) {
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id leaf : change.offspring) {
                live.insert(leaf);
                leaf_goals.emplace(leaf, reference_pud_goal_list(module, leaf.node));
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                leaf_goals.erase(refuted);
            }
        }
        size_t steps = 0;
        for (; steps < 150; ++steps) {
            std::vector<pud_global_node_id> unfoldable;
            for (pud_global_node_id leaf : live) {
                const pud_module* leaf_module = nullptr;
                for (const pud_module& module : modules) {
                    if (module.get_module_id() == leaf.module)
                        leaf_module = &module;
                }
                if (leaf_goals.at(leaf).goals.empty())
                    continue;
                unfoldable.push_back(leaf);
            }
            if (unfoldable.empty())
                break;
            std::uniform_int_distribution<size_t> leaf_dist(0, unfoldable.size() - 1);
            const pud_global_node_id chosen_leaf = unfoldable.at(leaf_dist(rng));
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == chosen_leaf.module)
                    leaf_module = &module;
            }
            std::vector<pud_body_goal_id> goal_ids;
            for (const auto& [goal_id, goal] : leaf_goals.at(chosen_leaf).goals)
                goal_ids.push_back(goal_id);
            std::uniform_int_distribution<size_t> goal_dist(0, goal_ids.size() - 1);
            const pud_body_goal_id chosen_goal = goal_ids.at(goal_dist(rng));
            const pud_goal_list parent_goals = leaf_goals.at(chosen_leaf);
            const pud_frontier_change change = runtime.unfold(chosen_leaf, chosen_goal);
            live.erase(chosen_leaf);
            leaf_goals.erase(chosen_leaf);
            for (pud_global_node_id child : change.offspring) {
                const pud_goal_list child_goals = this->goal_resolver.resolve_goal(
                    parent_goals,
                    chosen_goal,
                    leaf_module->get_inference(child.node).body_goals);
                live.insert(child);
                leaf_goals.emplace(child, child_goals);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                leaf_goals.erase(refuted);
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
        for (const pud_module& checked_module : modules)
            EXPECT_THAT(reference_pud_inference_violations(checked_module), IsEmpty()) << "seed " << seed;
    }
}

TYPED_TEST(PudInferenceInvariantsIntegrationTest, AppendSplitInferencesStayValidUnderRandomUnfolds) {
    const expr* target = this->functor(
        "cons",
        {this->atom("a"), this->functor("cons", {this->atom("b"), this->atom("nil")})});
    for (uint32_t seed = 0; seed < 30; ++seed) {
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
        std::map<pud_global_node_id, pud_goal_list> leaf_goals;
        for (pud_module& module : modules) {
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id leaf : change.offspring) {
                live.insert(leaf);
                leaf_goals.emplace(leaf, reference_pud_goal_list(module, leaf.node));
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                leaf_goals.erase(refuted);
            }
        }
        size_t steps = 0;
        for (; steps < 150; ++steps) {
            std::vector<pud_global_node_id> unfoldable;
            for (pud_global_node_id leaf : live) {
                const pud_module* leaf_module = nullptr;
                for (const pud_module& module : modules) {
                    if (module.get_module_id() == leaf.module)
                        leaf_module = &module;
                }
                if (leaf_goals.at(leaf).goals.empty())
                    continue;
                unfoldable.push_back(leaf);
            }
            if (unfoldable.empty())
                break;
            std::uniform_int_distribution<size_t> leaf_dist(0, unfoldable.size() - 1);
            const pud_global_node_id chosen_leaf = unfoldable.at(leaf_dist(rng));
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == chosen_leaf.module)
                    leaf_module = &module;
            }
            std::vector<pud_body_goal_id> goal_ids;
            for (const auto& [goal_id, goal] : leaf_goals.at(chosen_leaf).goals)
                goal_ids.push_back(goal_id);
            std::uniform_int_distribution<size_t> goal_dist(0, goal_ids.size() - 1);
            const pud_body_goal_id chosen_goal = goal_ids.at(goal_dist(rng));
            const pud_goal_list parent_goals = leaf_goals.at(chosen_leaf);
            const pud_frontier_change change = runtime.unfold(chosen_leaf, chosen_goal);
            live.erase(chosen_leaf);
            leaf_goals.erase(chosen_leaf);
            for (pud_global_node_id child : change.offspring) {
                const pud_goal_list child_goals = this->goal_resolver.resolve_goal(
                    parent_goals,
                    chosen_goal,
                    leaf_module->get_inference(child.node).body_goals);
                live.insert(child);
                leaf_goals.emplace(child, child_goals);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                leaf_goals.erase(refuted);
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
        for (const pud_module& checked_module : modules)
            EXPECT_THAT(reference_pud_inference_violations(checked_module), IsEmpty()) << "seed " << seed;
    }
}

TYPED_TEST(PudInferenceInvariantsIntegrationTest, GraphReachabilityInferencesStayValidUnderRandomUnfolds) {
    for (uint32_t seed = 0; seed < 30; ++seed) {
        std::mt19937 rng(seed);
        std::deque<pud_module> modules;
        modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
            rule(this->functor("edge", {this->atom("a"), this->atom("b")}), {}, 0),
            rule(this->functor("edge", {this->atom("b"), this->atom("c")}), {}, 0),
            rule(this->functor("edge", {this->atom("c"), this->atom("d")}), {}, 0),
            rule(this->functor("edge", {this->atom("b"), this->atom("e")}), {}, 0),
            rule(
                this->functor("path", {this->var(0), this->var(1)}),
                {this->functor("edge", {this->var(0), this->var(1)})},
                2),
            rule(
                this->functor("path", {this->var(0), this->var(2)}),
                {this->functor("edge", {this->var(0), this->var(1)}), this->functor("path", {this->var(1), this->var(2)})},
                3),
        });
        modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
            rule(
                this->functor("r", {this->var(0)}),
                {this->functor("path", {this->atom("a"), this->var(0)})},
                1),
        });
        TypeParam runtime(this->exprs);
        std::set<pud_global_node_id> live;
        std::map<pud_global_node_id, pud_goal_list> leaf_goals;
        for (pud_module& module : modules) {
            const pud_frontier_change change = runtime.register_module(module);
            for (pud_global_node_id leaf : change.offspring) {
                live.insert(leaf);
                leaf_goals.emplace(leaf, reference_pud_goal_list(module, leaf.node));
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                leaf_goals.erase(refuted);
            }
        }
        size_t steps = 0;
        for (; steps < 150; ++steps) {
            std::vector<pud_global_node_id> unfoldable;
            for (pud_global_node_id leaf : live) {
                const pud_module* leaf_module = nullptr;
                for (const pud_module& module : modules) {
                    if (module.get_module_id() == leaf.module)
                        leaf_module = &module;
                }
                if (leaf_goals.at(leaf).goals.empty())
                    continue;
                unfoldable.push_back(leaf);
            }
            if (unfoldable.empty())
                break;
            std::uniform_int_distribution<size_t> leaf_dist(0, unfoldable.size() - 1);
            const pud_global_node_id chosen_leaf = unfoldable.at(leaf_dist(rng));
            const pud_module* leaf_module = nullptr;
            for (const pud_module& module : modules) {
                if (module.get_module_id() == chosen_leaf.module)
                    leaf_module = &module;
            }
            std::vector<pud_body_goal_id> goal_ids;
            for (const auto& [goal_id, goal] : leaf_goals.at(chosen_leaf).goals)
                goal_ids.push_back(goal_id);
            std::uniform_int_distribution<size_t> goal_dist(0, goal_ids.size() - 1);
            const pud_body_goal_id chosen_goal = goal_ids.at(goal_dist(rng));
            const pud_goal_list parent_goals = leaf_goals.at(chosen_leaf);
            const pud_frontier_change change = runtime.unfold(chosen_leaf, chosen_goal);
            live.erase(chosen_leaf);
            leaf_goals.erase(chosen_leaf);
            for (pud_global_node_id child : change.offspring) {
                const pud_goal_list child_goals = this->goal_resolver.resolve_goal(
                    parent_goals,
                    chosen_goal,
                    leaf_module->get_inference(child.node).body_goals);
                live.insert(child);
                leaf_goals.emplace(child, child_goals);
            }
            for (pud_global_node_id refuted : change.refuted) {
                live.erase(refuted);
                leaf_goals.erase(refuted);
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
        for (const pud_module& checked_module : modules)
            EXPECT_THAT(reference_pud_inference_violations(checked_module), IsEmpty()) << "seed " << seed;
    }
}
