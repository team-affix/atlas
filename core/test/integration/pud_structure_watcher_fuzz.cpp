#include <cstdint>
#include <deque>
#include <random>
#include <iterator>
#include <set>
#include <variant>
#include <vector>
#include <gtest/gtest.h>
#include "functor_fixture.hpp"
#include "reference_pud_canonical.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/pud_binder.hpp"
#include "infrastructure/pud_module.hpp"
#include "infrastructure/pud_modules.hpp"
#include "infrastructure/pud_goal_resolver.hpp"
#include "infrastructure/pud_initial_goal_opener.hpp"
#include "infrastructure/pud_structure_watcher.hpp"
#include "infrastructure/pud_term_factory.hpp"
#include "infrastructure/ubs_bind_map.hpp"
#include "value_objects/pud_goal_list.hpp"

// Random trees checked against an oracle: a goal changes structure exactly when its canonical form,
// with unbound classes numbered by first occurrence, differs between parent and child.
struct PudStructureWatcherFuzzTest : public ::testing::Test {
    using term_factory_t = pud_term_factory<globalizer, expr_pool, expr_pool>;
    using modules_t = pud_modules<pud_module>;

    struct level {
        pud_ubs_state bindings;
        pud_goal_list goals;
        pud_structure_watch watch;
        pud_node_id node;
        uint32_t depth;
    };

    static constexpr pud_module_id k_module = 0;
    static constexpr uint32_t k_var_count = 14;
    static constexpr uint32_t k_root_count = 140;

    test_functors functors;
    expr_pool exprs;
    globalizer globalize;
    term_factory_t terms{globalize, exprs, exprs};
    std::deque<pud_module> module_data;
    modules_t modules;
    pud_binder<term_factory_t, modules_t> binder{terms, modules};
    pud_structure_watcher<term_factory_t, modules_t> watcher{terms, modules};
    pud_goal_resolver goal_resolver;
    pud_initial_goal_opener initial_goal_opener;
    std::mt19937 rng{0};
    uint32_t checked_nodes = 0;
    uint32_t observed_changes = 0;

    void SetUp() override {
        std::vector<rule> roots;
        for (uint32_t root = 0; root < k_root_count; ++root)
            roots.push_back(rule(exprs.make_functor(functors.id("dummy"), {}), {}, 0));
        modules.register_module(module_data.emplace_back(k_module, std::vector<pud_module_id>{}, roots));
    }

    uint32_t pick(uint32_t bound) {
        return std::uniform_int_distribution<uint32_t>{0, bound - 1}(rng);
    }

    bool coin(uint32_t percent) {
        return pick(100) < percent;
    }

    const expr* constant() {
        static const char* names[] = {"a", "b", "c"};
        return exprs.make_functor(functors.id(names[pick(3)]), {});
    }

    std::vector<uint32_t> unbound_reps(const ubs_bind_map& bind_map) {
        std::vector<uint32_t> reps;
        for (uint32_t var_index = 0; var_index < k_var_count; ++var_index) {
            const ubs_class var_class = bind_map.find(var_index);
            if (!var_class.value.has_value() && var_class.rep == var_index)
                reps.push_back(var_index);
        }
        return reps;
    }

    const expr* random_expr(const std::vector<uint32_t>& reps, uint32_t depth) {
        const bool leaf = depth == 0 || coin(40);
        if (leaf && !reps.empty() && coin(75))
            return exprs.make_var(reps.at(pick(reps.size())));
        if (leaf)
            return constant();
        static const char* names[] = {"f", "g", "h"};
        const uint32_t arity = 1 + pick(3);
        std::vector<const expr*> args;
        for (uint32_t arg_index = 0; arg_index < arity; ++arg_index)
            args.push_back(random_expr(reps, depth - 1));
        return exprs.make_functor(functors.id(names[pick(3)]), args);
    }

    std::vector<pud_substitution> random_substitutions(const pud_ubs_state& parent_bindings) {
        ubs_bind_map bind_map{parent_bindings};
        std::vector<pud_substitution> substitutions;
        const uint32_t count = pick(4);
        for (uint32_t substitution_index = 0; substitution_index < count; ++substitution_index) {
            const std::vector<uint32_t> reps = unbound_reps(bind_map);
            if (reps.empty())
                break;
            if (reps.size() >= 2 && coin(55)) {
                const uint32_t first = reps.at(pick(reps.size()));
                uint32_t second = reps.at(pick(reps.size()));
                while (second == first)
                    second = reps.at(pick(reps.size()));
                const uint32_t demoted = std::max(first, second);
                const uint32_t surviving = std::min(first, second);
                substitutions.push_back({.var_idx = demoted, .value = exprs.make_var(surviving)});
                bind_map.equate(bind_map.find(demoted), bind_map.find(surviving));
                continue;
            }
            const uint32_t rep = reps.at(pick(reps.size()));
            std::vector<uint32_t> others;
            for (uint32_t other : reps)
                if (other != rep)
                    others.push_back(other);
            const expr* value = random_expr(others, 2);
            if (std::holds_alternative<expr::var>(value->content))
                value = exprs.make_functor(functors.id("w"), {value});
            substitutions.push_back({.var_idx = rep, .value = value});
            bind_map.instantiate(bind_map.find(rep), framed_expr{value, 0});
        }
        return substitutions;
    }

    std::vector<const expr*> random_goals(const pud_ubs_state& bindings, uint32_t count) {
        const std::vector<uint32_t> reps = unbound_reps(ubs_bind_map{bindings});
        std::vector<const expr*> goals;
        for (uint32_t goal_index = 0; goal_index < count; ++goal_index)
            goals.push_back(random_expr(reps, 3));
        return goals;
    }

    level make_root(pud_node_id root) {
        const std::vector<const expr*> opened_goals = random_goals({}, 1 + pick(4));
        const pud_goal_list goals = initial_goal_opener.open_goals(opened_goals);
        return level{.bindings = {}, .goals = goals, .watch = watcher.watch_root({}, goals), .node = root, .depth = 0};
    }

    level descend_and_check(const level& parent, pud_body_goal_id call_site) {
        const pud_global_node_id child = modules.store_inference(k_module, pud_inference{
            .parent = parent.node,
            .call_site = call_site,
            .substitutions = random_substitutions(parent.bindings),
            .body_goals = {},
            .var_count = 0,
        });
        const pud_ubs_state child_bindings = binder.bind(parent.bindings, child);
        const std::vector<const expr*> added_goals = random_goals(child_bindings, pick(4));
        const pud_goal_list goals = goal_resolver.resolve_goal(parent.goals, call_site, added_goals);

        std::set<pud_body_goal_id> expected;
        for (const auto& [inherited_id, inherited_goal] : parent.goals.goals) {
            if (inherited_id == call_site)
                continue;
            const bool changed = reference_pud_canonical(parent.bindings, inherited_goal)
                != reference_pud_canonical(child_bindings, inherited_goal);
            if (changed)
                expected.insert(inherited_id);
        }

        const pud_structure_watch_change change = watcher.descend(parent.watch, child_bindings, child, call_site, goals);
        const std::set<pud_body_goal_id> actual(change.changed_goals.begin(), change.changed_goals.end());
        EXPECT_EQ(actual.size(), change.changed_goals.size()) << "a goal was reported twice at node " << child.node;
        EXPECT_EQ(actual, expected) << "at node " << child.node << " depth " << parent.depth + 1;

        ++checked_nodes;
        observed_changes += expected.size();
        return level{
            .bindings = child_bindings, .goals = goals, .watch = change.watch, .node = child.node, .depth = parent.depth + 1};
    }

    void explore(const level& at, uint32_t max_depth) {
        if (at.depth == max_depth || at.goals.goals.empty() || ::testing::Test::HasFailure())
            return;
        const pud_body_goal_id call_site = std::next(at.goals.goals.begin(), pick(at.goals.goals.size()))->first;
        const uint32_t branching = 1 + pick(3);
        for (uint32_t branch = 0; branch < branching; ++branch)
            explore(descend_and_check(at, call_site), max_depth);
    }
};

TEST_F(PudStructureWatcherFuzzTest, MatchesCanonicalFormOracleOnRandomTrees) {
    for (uint32_t seed = 0; seed < 60; ++seed) {
        rng.seed(seed);
        explore(make_root(seed), 5);
        ASSERT_FALSE(HasFailure()) << "seed " << seed;
    }
    EXPECT_GT(checked_nodes, 1000u);
    EXPECT_GT(observed_changes, 100u);
}

TEST_F(PudStructureWatcherFuzzTest, MatchesCanonicalFormOracleOnDeepPaths) {
    for (uint32_t seed = 100; seed < 140; ++seed) {
        rng.seed(seed);
        level at = make_root(seed);
        for (uint32_t step = 0; step < 40 && !at.goals.goals.empty() && !HasFailure(); ++step) {
            const pud_body_goal_id call_site = std::next(at.goals.goals.begin(), pick(at.goals.goals.size()))->first;
            at = descend_and_check(at, call_site);
        }
        ASSERT_FALSE(HasFailure()) << "seed " << seed;
    }
}
