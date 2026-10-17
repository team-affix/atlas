#include <algorithm>
#include <cstdint>
#include <map>
#include <random>
#include <set>
#include <unordered_map>
#include <variant>
#include <vector>
#include <gtest/gtest.h>
#include "functor_fixture.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/pud_binding_descent.hpp"
#include "infrastructure/pud_node_added_substitutions.hpp"
#include "infrastructure/pud_structure_watch_descent.hpp"
#include "infrastructure/pud_transient_ubs_state.hpp"
#include "value_objects/pud_substitution.hpp"

// Oracle: a watched expression's structure changes between parent and child exactly when its
// canonical form differs, where the canonical form resolves every variable through the real
// bind map and numbers representatives by first occurrence.
struct PudStructureWatchDescentFuzzTest : public ::testing::Test {
    using bind_map_t = ubs_transient_bind_map<expr_pool, expr_pool>;
    using binding_descent_t = pud_binding_descent<
        bind_map_t,
        pud_node_added_substitutions,
        expr_pool,
        expr_pool>;
    using watch_descent_t = pud_structure_watch_descent<pud_node_added_substitutions>;

    struct level {
        binding_descent_t binding;
        watch_descent_t watch;
        std::map<pud_watch_head_id, const expr*> live_heads;
        pud_watch_head_id next_head_id;
        uint32_t depth;
    };

    static constexpr uint32_t k_var_count = 14;
    static constexpr uint32_t k_rep_token_base = 1000000;

    test_functors                  functors;
    expr_pool                      exprs;
    pud_node_added_substitutions node_specs_;
    std::mt19937                   rng{0};
    pud_node_id                    next_node_id = 1;
    uint32_t                       checked_nodes = 0;
    uint32_t                       observed_changes = 0;

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

    std::vector<uint32_t> unbound_reps(bind_map_t& bind_map) {
        std::vector<uint32_t> reps;
        for (uint32_t var_index = 0; var_index < k_var_count; ++var_index) {
            const expr::var* resolved = std::get_if<expr::var>(&bind_map.resolve(var_index)->content);
            if (resolved && resolved->index == var_index)
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

    std::vector<pud_substitution> random_substitutions(const binding_descent_t& parent) {
        pud_transient_ubs_state scratch{parent.ubs_state()};
        bind_map_t bind_map = scratch.make_bind_map(exprs, exprs);
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
                bind_map.equate(demoted, surviving);
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
            const expr::functor& functor = std::get<expr::functor>(value->content);
            substitutions.push_back({.var_idx = rep, .value = value});
            bind_map.instantiate(rep, functor.id, functor.args);
        }
        return substitutions;
    }

    void visit_canonical(
        bind_map_t& bind_map,
        const expr* visited,
        std::unordered_map<uint32_t, uint32_t>& numbering,
        std::vector<uint32_t>& tokens) {
        if (const expr::var* var = std::get_if<expr::var>(&visited->content)) {
            const expr* resolved = bind_map.resolve(var->index);
            if (const expr::var* rep = std::get_if<expr::var>(&resolved->content)) {
                const uint32_t number = numbering.emplace(rep->index, numbering.size()).first->second;
                tokens.push_back(k_rep_token_base + number);
                return;
            }
            visit_canonical(bind_map, resolved, numbering, tokens);
            return;
        }
        const expr::functor& functor = std::get<expr::functor>(visited->content);
        tokens.push_back(functor.id);
        tokens.push_back(functor.args.size());
        for (const expr* arg : functor.args)
            visit_canonical(bind_map, arg, numbering, tokens);
    }

    std::vector<uint32_t> canonical(const binding_descent_t& at, const expr* watched) {
        pud_transient_ubs_state transient{at.ubs_state()};
        bind_map_t bind_map = transient.make_bind_map(exprs, exprs);
        std::unordered_map<uint32_t, uint32_t> numbering;
        std::vector<uint32_t> tokens;
        visit_canonical(bind_map, watched, numbering, tokens);
        return tokens;
    }

    std::vector<const expr*> random_watch_list(const binding_descent_t& at) {
        pud_transient_ubs_state transient{at.ubs_state()};
        bind_map_t bind_map = transient.make_bind_map(exprs, exprs);
        const std::vector<uint32_t> reps = unbound_reps(bind_map);
        std::vector<const expr*> watched;
        const uint32_t count = pick(4);
        for (uint32_t watch_index = 0; watch_index < count; ++watch_index)
            watched.push_back(random_expr(reps, 3));
        return watched;
    }

    level make_root() {
        level root{
            .binding = binding_descent_t{node_specs_, exprs, exprs},
            .watch = watch_descent_t{node_specs_, {}},
            .live_heads = {},
            .next_head_id = 0,
            .depth = 0,
        };
        const std::vector<const expr*> watched = random_watch_list(root.binding);
        root.watch = watch_descent_t{node_specs_, watched};
        for (const expr* watched_expr : watched)
            root.live_heads.emplace(root.next_head_id++, watched_expr);
        return root;
    }

    level descend_and_check(const level& parent) {
        const pud_node_id child = next_node_id++;
        node_specs_.store(child, random_substitutions(parent.binding));
        const binding_descent_t child_binding = parent.binding.descend(child);

        std::map<pud_watch_head_id, const expr*> live_heads = parent.live_heads;
        std::vector<pud_watch_head_id> unwatch;
        for (const auto& [head_id, watched_expr] : parent.live_heads) {
            if (!coin(15))
                continue;
            unwatch.push_back(head_id);
            live_heads.erase(head_id);
        }
        const std::vector<const expr*> watch = random_watch_list(child_binding);

        std::set<pud_watch_head_id> expected;
        for (const auto& [head_id, watched_expr] : live_heads) {
            const bool changed = canonical(parent.binding, watched_expr) != canonical(child_binding, watched_expr);
            if (changed)
                expected.insert(head_id);
        }

        const pud_structure_watch_result<watch_descent_t> result = parent.watch.descend(child, unwatch, watch);
        const std::set<pud_watch_head_id> actual(result.structure_changes.begin(), result.structure_changes.end());
        EXPECT_EQ(actual.size(), result.structure_changes.size()) << "a head was reported twice at node " << child;
        EXPECT_EQ(actual, expected) << "at node " << child << " depth " << parent.depth + 1;

        std::vector<pud_watch_head_id> expected_added;
        pud_watch_head_id next_head_id = parent.next_head_id;
        for (const expr* watched_expr : watch) {
            expected_added.push_back(next_head_id);
            live_heads.emplace(next_head_id++, watched_expr);
        }
        EXPECT_EQ(result.added_head_ids, expected_added);

        ++checked_nodes;
        observed_changes += expected.size();
        return level{
            .binding = child_binding,
            .watch = result.child_descent,
            .live_heads = live_heads,
            .next_head_id = next_head_id,
            .depth = parent.depth + 1,
        };
    }

    void explore(const level& at, uint32_t max_depth) {
        if (at.depth == max_depth || ::testing::Test::HasFailure())
            return;
        const uint32_t branching = 1 + pick(3);
        for (uint32_t branch = 0; branch < branching; ++branch)
            explore(descend_and_check(at), max_depth);
    }
};

TEST_F(PudStructureWatchDescentFuzzTest, MatchesCanonicalFormOracleOnRandomTrees) {
    for (uint32_t seed = 0; seed < 60; ++seed) {
        rng.seed(seed);
        explore(make_root(), 5);
        ASSERT_FALSE(HasFailure()) << "seed " << seed;
    }
    EXPECT_GT(checked_nodes, 1000u);
    EXPECT_GT(observed_changes, 100u);
}

TEST_F(PudStructureWatchDescentFuzzTest, MatchesCanonicalFormOracleOnDeepPaths) {
    for (uint32_t seed = 100; seed < 140; ++seed) {
        rng.seed(seed);
        level at = make_root();
        for (uint32_t step = 0; step < 40 && !HasFailure(); ++step)
            at = descend_and_check(at);
        ASSERT_FALSE(HasFailure()) << "seed " << seed;
    }
}
