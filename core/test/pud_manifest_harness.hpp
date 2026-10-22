#ifndef PUD_MANIFEST_HARNESS_HPP
#define PUD_MANIFEST_HARNESS_HPP

#include <algorithm>
#include <map>
#include <cstdint>
#include <optional>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>
#include <gtest/gtest.h>
#include "functor_fixture.hpp"
#include "infrastructure/pud_manifest.hpp"
#include "infrastructure/pud_random_rule_chooser.hpp"
#include "infrastructure/pud_random_subgoal_chooser.hpp"
#include "infrastructure/pud_random_unit_solver_manifest.hpp"
#include "value_objects/pud_unfold.hpp"
#include "value_objects/rule.hpp"

enum struct pud_harness_scope { every_open_leaf, conjecture_leaves };

template<typename Manifest>
struct pud_manifest_harness {
    pud_manifest_harness(uint32_t random_seed, pud_harness_scope scope)
        : manifest(make_manifest(random_seed))
        , scope(scope)
        , rng(random_seed)
        , rule_chooser(rng)
        , subgoal_chooser(manifest.node_descents_, rng) {}

    static Manifest make_manifest(uint32_t random_seed) {
        if constexpr (std::is_constructible_v<Manifest, uint32_t>)
            return Manifest(random_seed);
        else
            return Manifest();
    }

    Manifest manifest;
    pud_harness_scope scope;
    std::mt19937 rng;
    pud_random_rule_chooser rule_chooser;
    pud_random_subgoal_chooser<typename Manifest::node_descents_t> subgoal_chooser;
    test_functors functors;
    std::vector<pud_node_id> started_roots;
    std::map<pud_node_id, const expr*> answer_templates;
    std::optional<coroutine<pud_unfold, void>> unfolding;
    std::map<pud_node_id, std::set<pud_node_id>> reported_solutions;

    const expr* var(uint32_t index) { return manifest.expr_pool_.make_var(index); }

    const expr* functor(const std::string& name, std::vector<const expr*> args) {
        return manifest.expr_pool_.make_functor(functors.id(name), args);
    }

    const expr* atom(const std::string& name) { return functor(name, {}); }

    const expr* list(std::vector<const expr*> items) {
        const expr* tail = atom("nil");
        for (auto item = items.rbegin(); item != items.rend(); ++item)
            tail = functor("cons", {*item, tail});
        return tail;
    }

    const expr* peano_on(uint32_t count, const std::string& base) {
        const expr* number = atom(base);
        for (uint32_t step = 0; step < count; ++step)
            number = functor("s", {number});
        return number;
    }

    const expr* peano(uint32_t count) { return peano_on(count, "z"); }

    pud_node_id axiom(const expr* head, std::vector<const expr*> body, uint32_t var_count) {
        const pud_node_id root = manifest.axiom_initializer_.initialize_axiom(rule(head, std::move(body), var_count));
        answer_templates.emplace(root, head);
        return root;
    }

    void run_base_descent() {
        for (pud_node_id root_node : manifest.root_nodes_.get()) {
            if (std::find(started_roots.begin(), started_roots.end(), root_node) != started_roots.end())
                continue;
            manifest.base_descent_runner_.run(root_node);
            started_roots.push_back(root_node);
        }
    }

    pud_node_id start_conjecture(const expr* answer_template, std::vector<const expr*> goals, uint32_t var_count) {
        const pud_node_id root = manifest.conjecture_initializer_.initialize_conjecture(answer_template, goals, var_count);
        answer_templates.emplace(root, answer_template);
        run_base_descent();
        return root;
    }

    void add_colors() {
        axiom(functor("color", {atom("red")}), {}, 0);
        axiom(functor("color", {atom("blue")}), {}, 0);
        axiom(functor("warm", {atom("red")}), {}, 0);
    }

    pud_node_id add_nat() {
        axiom(functor("nat", {atom("z")}), {}, 0);
        return axiom(functor("nat", {functor("s", {var(0)})}), {functor("nat", {var(0)})}, 1);
    }

    void add_append() {
        axiom(functor("app", {atom("nil"), var(0), var(0)}), {}, 1);
        axiom(
            functor("app", {functor("cons", {var(0), var(1)}), var(2), functor("cons", {var(0), var(3)})}),
            {functor("app", {var(1), var(2), var(3)})},
            4);
    }

    void add_member() {
        axiom(functor("mem", {var(0), functor("cons", {var(0), var(1)})}), {}, 2);
        axiom(functor("mem", {var(0), functor("cons", {var(1), var(2)})}), {functor("mem", {var(0), var(2)})}, 3);
    }

    void add_peano_add() {
        axiom(functor("add", {atom("z"), var(0), var(0)}), {}, 1);
        axiom(
            functor("add", {functor("s", {var(0)}), var(1), functor("s", {var(2)})}),
            {functor("add", {var(0), var(1), var(2)})},
            3);
    }

    void add_graph() {
        axiom(functor("parent", {atom("a"), atom("b")}), {}, 0);
        axiom(functor("parent", {atom("b"), atom("c")}), {}, 0);
        axiom(functor("anc", {var(0), var(1)}), {functor("parent", {var(0), var(1)})}, 2);
        axiom(
            functor("anc", {var(0), var(1)}),
            {functor("parent", {var(0), var(2)}), functor("anc", {var(2), var(1)})},
            3);
    }

    pud_node_id grow_along_open_goals(pud_node_id root, uint32_t depth) {
        pud_node_id frontier = root;
        for (uint32_t level = 0; level < depth; ++level) {
            manifest.unfolder_.unfold(frontier, first_open_goal(frontier));
            std::optional<pud_node_id> next;
            for (pud_node_id child : children_of(frontier)) {
                if (refuted(child) || open_goals(child).empty())
                    continue;
                next = child;
            }
            if (!next.has_value())
                return frontier;
            frontier = next.value();
        }
        return frontier;
    }

    coroutine<pud_unfold, void>& unfold_loop() {
        run_base_descent();
        const bool ended = !unfolding.has_value() || unfolding->done();
        if (ended)
            unfolding.emplace(manifest.unfold_loop_.drive_unfold_loop());
        return unfolding.value();
    }

    std::set<pud_node_id> drivable_leaves() {
        if (scope == pud_harness_scope::every_open_leaf)
            return manifest.open_leaves_.get();
        std::set<pud_node_id> leaves;
        for (pud_node_id leaf_node : manifest.open_leaves_.get()) {
            if (!manifest.axioms_.check_axiom(root_of(leaf_node)))
                leaves.insert(leaf_node);
        }
        return leaves;
    }

    std::optional<pud_unfold> next_unfold() {
        if constexpr (requires (Manifest& any) { any.unfold_loop_; }) {
            coroutine<pud_unfold, void>& running = unfold_loop();
            running.resume();
            if (!running.has_yield())
                return std::nullopt;
            return running.consume_yield();
        } else {
            run_base_descent();
            const std::set<pud_node_id> leaves = drivable_leaves();
            if (leaves.empty())
                return std::nullopt;
            const pud_node_id leaf_node = rule_chooser.choose_rule(leaves);
            const pud_body_goal_id goal = subgoal_chooser.choose_subgoal(leaf_node);
            manifest.unfolder_.unfold(leaf_node, goal);
            return pud_unfold{.leaf = leaf_node, .goal = goal};
        }
    }

    bool advance_unfold_loop() { return next_unfold().has_value(); }

    bool unfold_once(pud_node_id root) {
        run_base_descent();
        if (!manifest.axioms_.check_axiom(root))
            return advance_unfold_loop();
        for (pud_node_id leaf_node : manifest.open_leaves_.get()) {
            if (root_of(leaf_node) != root)
                continue;
            manifest.unfolder_.unfold(leaf_node, first_open_goal(leaf_node));
            return true;
        }
        return false;
    }

    size_t unfold_until_closed(size_t max_unfolds) {
        size_t unfolds = 0;
        while (unfolds < max_unfolds && advance_unfold_loop())
            ++unfolds;
        return unfolds;
    }

    size_t unfold_while_small(size_t max_unfolds, size_t max_goals) {
        size_t unfolds = 0;
        while (unfolds < max_unfolds && most_open_goals() <= max_goals && advance_unfold_loop())
            ++unfolds;
        return unfolds;
    }

    size_t most_open_goals() {
        size_t most = 0;
        for (pud_node_id leaf_node : manifest.open_leaves_.get())
            most = std::max(most, open_goals(leaf_node).size());
        return most;
    }

    std::set<const expr*> proven_heads() {
        std::set<const expr*> proven;
        for (pud_node_id axiom_root : manifest.axioms_.get()) {
            if (!started(axiom_root))
                continue;
            for (pud_node_id leaf_node : manifest.live_leaf_finder_.find_live_leaves(axiom_root)) {
                if (manifest.open_leaves_.check_leaf_open(leaf_node) || !has_descent(leaf_node))
                    continue;
                proven.insert(normalize_in(env_at_leaf(leaf_node), manifest.node_heads_.get(axiom_root)));
            }
        }
        return proven;
    }

    std::optional<pud_node_id> find_unreported_solution(pud_node_id root) {
        const std::set<pud_node_id>& reported = reported_solutions[root];
        std::optional<pud_node_id> found;
        for (pud_node_id leaf_node : manifest.live_leaf_finder_.find_live_leaves(root)) {
            if (manifest.open_leaves_.check_leaf_open(leaf_node) || reported.contains(leaf_node))
                continue;
            if (!found.has_value() || leaf_node < found.value())
                found = leaf_node;
        }
        return found;
    }

    std::optional<pud_node_id> next_solution(pud_node_id root, size_t max_steps) {
        for (size_t resumed = 0;; ++resumed) {
            const std::optional<pud_node_id> solution = find_unreported_solution(root);
            if (solution.has_value()) {
                reported_solutions[root].insert(solution.value());
                return solution;
            }
            if (resumed == max_steps || !unfold_once(root))
                return std::nullopt;
        }
    }

    bool exhausted(pud_node_id root) {
        const std::set<pud_node_id>& reported = reported_solutions[root];
        for (pud_node_id leaf_node : manifest.live_leaf_finder_.find_live_leaves(root)) {
            if (manifest.open_leaves_.check_leaf_open(leaf_node) || !reported.contains(leaf_node))
                return false;
        }
        return true;
    }

    const expr* answer(pud_node_id root, pud_node_id solution) {
        return normalize_in(env_at_leaf(solution), answer_templates.at(root));
    }

    std::vector<const expr*> enumerate(pud_node_id root, size_t max_answers, size_t max_steps) {
        std::vector<const expr*> answers;
        while (answers.size() < max_answers) {
            const std::optional<pud_node_id> solution = next_solution(root, max_steps);
            if (!solution.has_value())
                break;
            answers.push_back(answer(root, solution.value()));
        }
        return answers;
    }

    bool step(pud_node_id root, std::vector<const expr*>& answers) {
        const std::optional<pud_node_id> solution = find_unreported_solution(root);
        if (solution.has_value()) {
            reported_solutions[root].insert(solution.value());
            answers.push_back(answer(root, solution.value()));
            return true;
        }
        if (exhausted(root))
            return false;
        return unfold_once(root);
    }

    bool refuted(pud_node_id node) { return manifest.refuted_node_checker_.check_refuted(node); }

    bool leaf(pud_node_id node) { return manifest.leaf_nodes_.check_leaf(node); }

    bool root(pud_node_id node) { return manifest.root_nodes_.check_root(node); }

    bool alive(pud_mhq_head_id head) { return manifest.mhq_.check_head(head); }

    bool expanded(pud_node_id node) { return manifest.call_sites_.check_call_site(node); }

    bool has_descent(pud_node_id node) { return manifest.node_descents_.check_descent(node); }

    std::optional<pud_mhq_head_id> query_of(pud_node_id leaf_node, pud_body_goal_id goal) {
        return manifest.node_descents_.get(leaf_node).body_goal_pq().find_query_head(goal);
    }

    std::vector<pud_node_id> children_of(pud_node_id node) { return manifest.children_.get(node); }

    std::vector<std::pair<pud_body_goal_id, const expr*>> open_goals(pud_node_id leaf_node) {
        std::vector<std::pair<pud_body_goal_id, const expr*>> goals;
        for (const auto& [goal, goal_expr] : manifest.node_descents_.get(leaf_node).body_goal_expr().body_goal_exprs())
            goals.emplace_back(goal, goal_expr);
        std::sort(goals.begin(), goals.end());
        return goals;
    }

    pud_body_goal_id first_open_goal(pud_node_id leaf_node) { return open_goals(leaf_node).front().first; }

    std::vector<pud_node_id> all_nodes() {
        std::vector<pud_node_id> nodes;
        for (pud_node_id node = 0; node < manifest.node_id_sequencer_.peek(); ++node)
            nodes.push_back(node);
        return nodes;
    }

    std::vector<pud_node_id> path_to(pud_node_id node) {
        std::vector<pud_node_id> path{node};
        while (!root(path.back()))
            path.push_back(manifest.node_parents_.get(path.back()));
        std::reverse(path.begin(), path.end());
        return path;
    }

    pud_node_id root_of(pud_node_id node) { return path_to(node).front(); }

    bool started(pud_node_id node) {
        return std::find(started_roots.begin(), started_roots.end(), root_of(node)) != started_roots.end();
    }

    std::vector<pud_node_id> live_leaves_with_descents() {
        std::vector<pud_node_id> leaves;
        for (pud_node_id node : all_nodes()) {
            if (has_descent(node))
                leaves.push_back(node);
        }
        return leaves;
    }

    std::vector<pud_mhq_head_id> heads_held_by_leaves() {
        std::vector<pud_mhq_head_id> heads;
        for (pud_node_id leaf_node : live_leaves_with_descents()) {
            for (const auto& [goal, goal_expr] : open_goals(leaf_node)) {
                const std::optional<pud_mhq_head_id> head = query_of(leaf_node, goal);
                if (head.has_value())
                    heads.push_back(head.value());
            }
        }
        std::sort(heads.begin(), heads.end());
        heads.erase(std::unique(heads.begin(), heads.end()), heads.end());
        return heads;
    }

    uint32_t lvc_of(pud_node_id node) {
        uint32_t lvc = 0;
        for (pud_node_id on_path : path_to(node))
            lvc += manifest.node_added_var_count_.get(on_path);
        return lvc;
    }

    pud_ubs_state env_of(pud_node_id node) {
        const std::vector<pud_node_id> path = path_to(node);
        typename Manifest::binding_descent_t binding{manifest.node_added_substitutions_};
        for (size_t index = 1; index < path.size(); ++index)
            binding = binding.descend(path.at(index));
        return binding.ubs_state();
    }

    pud_ubs_state env_at_leaf(pud_node_id leaf_node) {
        return manifest.node_descents_.get(leaf_node).binding().ubs_state();
    }

    const expr* resolve_in(const pud_ubs_state& env, uint32_t index) {
        pud_transient_ubs_state transient_ubs_state{env};
        auto bind_map = transient_ubs_state.make_bind_map();
        if (const std::optional<framed_expr> value = bind_map.find_value(index))
            return value->skeleton;
        return var(bind_map.find_rep(index));
    }

    template<typename BindMap>
    const expr* resolve_through(BindMap& bind_map, uint32_t index) {
        if (const std::optional<framed_expr> value = bind_map.find_value(index))
            return value->skeleton;
        return var(bind_map.find_rep(index));
    }

    const expr* normalize_in(const pud_ubs_state& env, const expr* skeleton) {
        pud_transient_ubs_state transient_ubs_state{env};
        auto bind_map = transient_ubs_state.make_bind_map();
        auto normalizer = manifest.normalizer_factory_.make(bind_map);
        return normalizer.normalize(framed_expr{skeleton, 0});
    }

    static std::vector<uint32_t> vars_in(const expr* skeleton) {
        std::vector<uint32_t> vars;
        std::vector<const expr*> pending{skeleton};
        while (!pending.empty()) {
            const expr* next = pending.back();
            pending.pop_back();
            if (const expr::var* variable = std::get_if<expr::var>(&next->content)) {
                vars.push_back(variable->index);
                continue;
            }
            for (const expr* arg : std::get<expr::functor>(next->content).args)
                pending.push_back(arg);
        }
        return vars;
    }

    void check_live_leaves_are_exactly_the_stored_descents() {
        for (pud_node_id node : all_nodes()) {
            const bool should_have_descent = started(node) && leaf(node) && !refuted(node);
            EXPECT_EQ(has_descent(node), should_have_descent) << "node " << node;
        }
    }

    void check_open_leaves_are_exactly_the_live_leaves_with_open_goals() {
        for (pud_node_id node : all_nodes()) {
            const bool should_be_open = has_descent(node) && !open_goals(node).empty();
            EXPECT_EQ(manifest.open_leaves_.check_leaf_open(node), should_be_open) << "node " << node;
        }
    }

    void check_open_conjecture_leaves_are_the_open_leaves_under_conjectures() {
        for (pud_node_id node : all_nodes()) {
            const bool under_conjecture = !manifest.axioms_.check_axiom(root_of(node));
            const bool should_be_listed = manifest.open_leaves_.check_leaf_open(node) && under_conjecture;
            EXPECT_EQ(manifest.open_conjecture_leaves_.get().contains(node), should_be_listed) << "node " << node;
        }
    }

    void check_leaf_flag_matches_having_children() {
        for (pud_node_id node : all_nodes()) {
            EXPECT_EQ(leaf(node), !expanded(node)) << "node " << node;
            if (leaf(node)) {
                EXPECT_TRUE(children_of(node).empty()) << "leaf " << node << " has children";
                continue;
            }
            const bool childless = children_of(node).empty();
            if (childless)
                EXPECT_EQ(manifest.node_live_child_counts_.get(node), 0u) << "childless interior " << node;
        }
    }

    void check_parents_and_children_agree() {
        for (pud_node_id node : all_nodes()) {
            for (pud_node_id child : children_of(node))
                EXPECT_EQ(manifest.node_parents_.get(child), node) << "child " << child;
            if (root(node)) {
                EXPECT_THROW(manifest.node_parents_.get(node), std::out_of_range) << "root " << node;
                continue;
            }
            const std::vector<pud_node_id> siblings = children_of(manifest.node_parents_.get(node));
            EXPECT_NE(std::find(siblings.begin(), siblings.end(), node), siblings.end()) << "orphan " << node;
        }
    }

    void check_live_child_counts_match_unrefuted_children() {
        for (pud_node_id node : all_nodes()) {
            if (leaf(node))
                continue;
            size_t live_children = 0;
            for (pud_node_id child : children_of(node))
                live_children += refuted(child) ? 0 : 1;
            EXPECT_EQ(manifest.node_live_child_counts_.get(node), live_children) << "node " << node;
        }
    }

    void check_refutation_is_closed_upward() {
        for (pud_node_id node : all_nodes()) {
            if (leaf(node))
                continue;
            bool every_child_refuted = true;
            for (pud_node_id child : children_of(node))
                every_child_refuted = every_child_refuted && refuted(child);
            EXPECT_EQ(refuted(node), every_child_refuted) << "node " << node;
        }
    }

    void check_refutation_is_closed_downward() {
        for (pud_node_id node : all_nodes()) {
            if (!refuted(node))
                continue;
            for (pud_node_id child : children_of(node))
                EXPECT_TRUE(refuted(child)) << "live child " << child << " of refuted " << node;
        }
    }

    void check_leaf_lvc_is_the_sum_of_var_counts_on_its_path() {
        for (pud_node_id node : live_leaves_with_descents())
            EXPECT_EQ(manifest.node_descents_.get(node).lvc().lvc(), lvc_of(node)) << "leaf " << node;
    }

    void check_leaf_binding_matches_its_replayed_substitutions() {
        for (pud_node_id node : live_leaves_with_descents())
            EXPECT_EQ(env_at_leaf(node), env_of(node)) << "leaf " << node;
    }

    void check_stored_goals_are_in_their_nodes_representatives() {
        for (pud_node_id node : all_nodes()) {
            const pud_ubs_state env = env_of(node);
            const uint32_t lvc = lvc_of(node);
            for (const expr* goal : manifest.node_added_body_goals_.get(node)) {
                for (uint32_t index : vars_in(goal)) {
                    EXPECT_LT(index, lvc) << "node " << node;
                    EXPECT_EQ(resolve_in(env, index), var(index)) << "node " << node << " var " << index;
                }
            }
        }
    }

    void check_substitutions_meet_the_watchers_preconditions() {
        for (pud_node_id node : all_nodes()) {
            if (root(node))
                continue;
            const pud_node_id parent = manifest.node_parents_.get(node);
            const uint32_t parent_lvc = lvc_of(parent);
            const uint32_t node_lvc = lvc_of(node);
            pud_transient_ubs_state transient_ubs_state{env_of(parent)};
            auto bind_map = transient_ubs_state.make_bind_map();
            for (const pud_substitution& substitution : manifest.node_added_substitutions_.get(node)) {
                EXPECT_LT(substitution.var_idx, parent_lvc) << "node " << node;
                EXPECT_EQ(resolve_through(bind_map, substitution.var_idx), var(substitution.var_idx))
                    << "node " << node << " substitutes non-representative " << substitution.var_idx;
                if (const expr::var* surviving = std::get_if<expr::var>(&substitution.value->content)) {
                    EXPECT_LT(surviving->index, substitution.var_idx) << "node " << node;
                    EXPECT_EQ(resolve_through(bind_map, surviving->index), var(surviving->index)) << "node " << node;
                    bind_map.equate(substitution.var_idx, surviving->index);
                    continue;
                }
                bind_map.instantiate(substitution.var_idx, framed_expr{substitution.value, 0});
            }
            const pud_ubs_state env = env_of(node);
            for (const pud_substitution& substitution : manifest.node_added_substitutions_.get(node)) {
                EXPECT_EQ(normalize_in(env, substitution.value), substitution.value)
                    << "node " << node << " stores an unnormalized value for " << substitution.var_idx;
                for (uint32_t index : vars_in(substitution.value))
                    EXPECT_LT(index, node_lvc) << "node " << node;
            }
        }
    }

    void check_fresh_variables_are_dense_from_the_parents_lvc() {
        for (pud_node_id node : all_nodes()) {
            if (root(node))
                continue;
            const uint32_t parent_lvc = lvc_of(manifest.node_parents_.get(node));
            const uint32_t added = manifest.node_added_var_count_.get(node);
            std::set<uint32_t> fresh;
            for (const pud_substitution& substitution : manifest.node_added_substitutions_.get(node)) {
                for (uint32_t index : vars_in(substitution.value)) {
                    if (index >= parent_lvc)
                        fresh.insert(index);
                }
            }
            for (const expr* goal : manifest.node_added_body_goals_.get(node)) {
                for (uint32_t index : vars_in(goal)) {
                    if (index >= parent_lvc)
                        fresh.insert(index);
                }
            }
            std::set<uint32_t> expected;
            for (uint32_t index = parent_lvc; index < parent_lvc + added; ++index)
                expected.insert(index);
            EXPECT_EQ(fresh, expected) << "node " << node;
        }
    }

    void check_every_open_goal_has_a_live_query() {
        for (pud_node_id node : live_leaves_with_descents()) {
            for (const auto& [goal, goal_expr] : open_goals(node)) {
                const std::optional<pud_mhq_head_id> head = query_of(node, goal);
                EXPECT_TRUE(head.has_value()) << "leaf " << node << " goal " << goal;
                if (head.has_value())
                    EXPECT_TRUE(alive(head.value())) << "leaf " << node << " goal " << goal;
            }
        }
    }

    void check_query_head_node_is_an_ancestor_of_every_leaf_holding_it() {
        for (pud_node_id node : live_leaves_with_descents()) {
            const std::vector<pud_node_id> path = path_to(node);
            for (const auto& [goal, goal_expr] : open_goals(node)) {
                const std::optional<pud_mhq_head_id> head = query_of(node, goal);
                if (!head.has_value())
                    continue;
                const pud_node_id opener = manifest.query_head_nodes_.get(head.value());
                EXPECT_NE(std::find(path.begin(), path.end(), opener), path.end())
                    << "leaf " << node << " holds a query opened at " << opener;
            }
        }
    }

    void check_every_candidate_path_is_a_real_root_first_chain() {
        for (pud_mhq_head_id head : heads_held_by_leaves()) {
            if (!alive(head))
                continue;
            for (pud_mhcs_head_id candidate : manifest.mhq_.get_candidates(head)) {
                const std::vector<pud_node_id> path = manifest.mhcs_.get_path(candidate);
                ASSERT_FALSE(path.empty());
                EXPECT_TRUE(root(path.front())) << "candidate " << candidate;
                for (size_t index = 1; index < path.size(); ++index)
                    EXPECT_EQ(manifest.node_parents_.get(path.at(index)), path.at(index - 1)) << "candidate " << candidate;
                for (pud_node_id on_path : path)
                    EXPECT_FALSE(refuted(on_path)) << "candidate " << candidate << " passes refuted " << on_path;
            }
        }
    }

    void check_every_candidate_is_acceptable_at_every_leaf_holding_the_query() {
        for (pud_node_id node : live_leaves_with_descents()) {
            const auto& descent = manifest.node_descents_.get(node);
            for (const auto& [goal, goal_expr] : open_goals(node)) {
                const std::optional<pud_mhq_head_id> head = query_of(node, goal);
                if (!head.has_value() || !alive(head.value()))
                    continue;
                for (pud_mhcs_head_id candidate : manifest.mhq_.get_candidates(head.value())) {
                    const std::vector<pud_node_id> path = manifest.mhcs_.get_path(candidate);
                    EXPECT_NO_THROW(manifest.acceptor_.accept(
                        descent.binding().ubs_state(), descent.lvc().lvc(), goal_expr, path))
                        << "leaf " << node << " goal " << goal << " candidate ending at " << path.back();
                }
            }
        }
    }

    std::map<pud_mhq_head_id, std::map<pud_node_id, pud_body_goal_id>> scan_query_holders() {
        std::map<pud_mhq_head_id, std::map<pud_node_id, pud_body_goal_id>> holders;
        for (pud_mhq_head_id head = 0;; ++head) {
            try {
                manifest.query_head_nodes_.get(head);
            } catch (const std::out_of_range&) {
                break;
            }
            holders[head];
        }
        for (pud_node_id leaf_node : live_leaves_with_descents()) {
            for (const auto& [goal, head] : manifest.node_descents_.get(leaf_node).body_goal_pq().get_query_heads())
                holders[head].emplace(leaf_node, goal);
        }
        return holders;
    }

    void check_query_holders_match_the_stored_descents() {
        if constexpr (requires (Manifest& any) { any.query_holders_; }) {
            for (const auto& [head, scanned] : scan_query_holders()) {
                std::set<pud_node_id> scanned_leaves;
                for (const auto& [leaf_node, goal] : scanned) {
                    scanned_leaves.insert(leaf_node);
                    EXPECT_EQ(goal, scanned.begin()->second) << "head " << head << " is held under two goals";
                    EXPECT_EQ(manifest.node_descents_.get(leaf_node).body_goal_pq().find_goal(head), goal)
                        << "head " << head << " leaf " << leaf_node;
                }
                EXPECT_EQ(manifest.query_holders_.get(head), scanned_leaves) << "head " << head;
            }
        }
    }

    void check_all() {
        check_live_leaves_are_exactly_the_stored_descents();
        check_open_leaves_are_exactly_the_live_leaves_with_open_goals();
        if constexpr (requires (Manifest& any) { any.open_conjecture_leaves_; })
            check_open_conjecture_leaves_are_the_open_leaves_under_conjectures();
        check_leaf_flag_matches_having_children();
        check_parents_and_children_agree();
        check_live_child_counts_match_unrefuted_children();
        check_refutation_is_closed_upward();
        check_refutation_is_closed_downward();
        check_leaf_lvc_is_the_sum_of_var_counts_on_its_path();
        check_leaf_binding_matches_its_replayed_substitutions();
        check_stored_goals_are_in_their_nodes_representatives();
        check_substitutions_meet_the_watchers_preconditions();
        check_fresh_variables_are_dense_from_the_parents_lvc();
        check_every_open_goal_has_a_live_query();
        check_query_head_node_is_an_ancestor_of_every_leaf_holding_it();
        check_every_candidate_path_is_a_real_root_first_chain();
        check_every_candidate_is_acceptable_at_every_leaf_holding_the_query();
        check_query_holders_match_the_stored_descents();
    }
};

using pud_random_unit_solver_manifest_harness = pud_manifest_harness<pud_random_unit_solver_manifest>;

#endif
