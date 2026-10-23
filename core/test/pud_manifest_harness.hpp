#ifndef PUD_MANIFEST_HARNESS_HPP
#define PUD_MANIFEST_HARNESS_HPP

#include <algorithm>
#include <cstdint>
#include <deque>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <variant>
#include <vector>
#include <gtest/gtest.h>
#include "functor_fixture.hpp"
#include "infrastructure/pud_manifest.hpp"
#include "infrastructure/ubs_bind_map.hpp"
#include "value_objects/pud_frontier_change.hpp"
#include "value_objects/pud_solution.hpp"
#include "value_objects/pud_unfold.hpp"
#include "value_objects/rule.hpp"

// Which leaves the harness drives: every open leaf of the database, or only conjecture leaves, which
// the production solver drives.
enum struct pud_harness_scope { every_open_leaf, conjecture_leaves };

// Builds databases on a real manifest and drives it: conjectures through the solver, a bare
// database by expanding its open leaves, forced unfolds first. The checks recompute what the
// manifest derives from what it stores, with the production pieces, and compare.
struct pud_manifest_harness {
    pud_manifest_harness(uint32_t random_seed, pud_harness_scope scope)
        : manifest(random_seed)
        , scope(scope)
        , functors()
        , roots()
        , database_frontier()
        , solving()
        , waiting_solutions() {}

    pud_manifest manifest;
    pud_harness_scope scope;
    test_functors functors;
    std::vector<pud_node_id> roots;
    std::set<pud_node_id> database_frontier;
    std::optional<coroutine<pud_solver_step, void>> solving;
    std::map<pud_node_id, std::deque<pud_solution>> waiting_solutions;

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
        const pud_node_id root = manifest.root_opener_.add_axiom(rule(head, std::move(body), var_count));
        roots.push_back(root);
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

    pud_frontier_change open_axioms() {
        const pud_frontier_change change = manifest.root_opener_.open();
        track(change);
        return change;
    }

    // A conjecture is solved by the solver, or, when the harness drives every open leaf, opened as a
    // plain root of the database.
    pud_node_id start_conjecture(const expr* answer_template, std::vector<const expr*> goals, uint32_t var_count) {
        open_axioms();
        if (scope == pud_harness_scope::conjecture_leaves) {
            const pud_node_id conjecture = manifest.solver_.add_conjecture(answer_template, goals, var_count);
            roots.push_back(conjecture);
            return conjecture;
        }
        const pud_node_id conjecture = manifest.root_opener_.add_conjecture(answer_template, goals, var_count);
        roots.push_back(conjecture);
        open_axioms();
        return conjecture;
    }

    void track(const pud_frontier_change& change) {
        database_frontier.insert(change.opened.begin(), change.opened.end());
        for (pud_node_id leaf : change.refuted)
            database_frontier.erase(leaf);
    }

    pud_frontier_change expand(pud_node_id leaf, pud_body_goal_id goal) {
        database_frontier.erase(leaf);
        const pud_frontier_change change = manifest.expander_.expand(leaf, goal);
        track(change);
        return change;
    }

    std::optional<pud_unfold> next_unfold() {
        std::optional<pud_unfold> unfold = manifest.unit_unfold_finder_.find_unit_unfold();
        if (!unfold.has_value() && database_frontier.empty())
            return std::nullopt;
        if (!unfold.has_value())
            unfold = manifest.expansion_chooser_.choose_expansion(database_frontier);
        expand(unfold->leaf, unfold->goal);
        return unfold;
    }

    bool advance_database() { return next_unfold().has_value(); }

    // Refutes every live leaf under the node, and whatever follows from that.
    void refute(pud_node_id node) {
        std::vector<pud_node_id> leaves;
        for (pud_node_id descendant : all_nodes()) {
            if (!live_leaf(descendant))
                continue;
            const std::vector<pud_node_id> path = path_to(descendant);
            if (std::find(path.begin(), path.end(), node) != path.end())
                leaves.push_back(descendant);
        }
        for (pud_node_id refuted_leaf : manifest.frontier_retirer_.retire({}, leaves))
            database_frontier.erase(refuted_leaf);
    }

    // Drives the database, then reads the conjecture's head at each of its proven leaves.
    std::vector<const expr*> proven_answers(pud_node_id conjecture, size_t max_unfolds) {
        unfold_until_closed(max_unfolds);
        std::vector<const expr*> answers;
        for (pud_node_id leaf_node : live_leaves()) {
            if (root_of(leaf_node) != conjecture || !open_goals(leaf_node).empty())
                continue;
            answers.push_back(manifest.leaf_materializer_.materialize(leaf_node, manifest.root_heads_.get(conjecture)));
        }
        return answers;
    }

    std::optional<pud_solver_step> next_solver_step() {
        if (!solving.has_value() || solving->done())
            solving.emplace(manifest.solver_.solve());
        solving->resume();
        if (!solving->has_yield())
            return std::nullopt;
        const pud_solver_step step = solving->consume_yield();
        if (const pud_solution* solution = std::get_if<pud_solution>(&step))
            waiting_solutions[solution->conjecture].push_back(*solution);
        return step;
    }

    bool advance_solver() { return next_solver_step().has_value(); }

    bool advance_unfold_loop() {
        if (scope == pud_harness_scope::conjecture_leaves)
            return advance_solver();
        return advance_database();
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

    pud_node_id grow_along_open_goals(pud_node_id root, uint32_t depth) {
        pud_node_id frontier = root;
        for (uint32_t level = 0; level < depth; ++level) {
            expand(frontier, first_open_goal(frontier));
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

    std::optional<pud_solution> next_solution(pud_node_id conjecture, size_t max_steps) {
        for (size_t steps = 0; waiting_solutions[conjecture].empty(); ++steps) {
            if (steps == max_steps || !advance_solver())
                return std::nullopt;
        }
        const pud_solution solution = waiting_solutions[conjecture].front();
        waiting_solutions[conjecture].pop_front();
        return solution;
    }

    std::vector<const expr*> enumerate(pud_node_id conjecture, size_t max_answers, size_t max_steps) {
        std::vector<const expr*> answers;
        while (answers.size() < max_answers) {
            const std::optional<pud_solution> solution = next_solution(conjecture, max_steps);
            if (!solution.has_value())
                break;
            answers.push_back(solution->answer);
        }
        return answers;
    }

    bool step(pud_node_id conjecture, std::vector<const expr*>& answers) {
        if (!waiting_solutions[conjecture].empty()) {
            answers.push_back(waiting_solutions[conjecture].front().answer);
            waiting_solutions[conjecture].pop_front();
            return true;
        }
        if (manifest.solver_.exhausted(conjecture))
            return false;
        return advance_solver();
    }

    bool exhausted(pud_node_id conjecture) {
        return waiting_solutions[conjecture].empty() && manifest.solver_.exhausted(conjecture);
    }

    bool refuted(pud_node_id node) { return manifest.node_status_.check_refuted(node); }

    bool leaf(pud_node_id node) { return manifest.node_status_.check_leaf(node); }

    bool root(pud_node_id node) { return !manifest.node_parents_.get(node).has_value(); }

    bool live_leaf(pud_node_id node) { return leaf(node) && !refuted(node); }

    bool expanded(pud_node_id node) { return !leaf(node); }

    bool open(pud_node_id node) { return live_leaf(node) && !open_goals(node).empty(); }

    std::vector<pud_node_id> open_leaves() {
        std::vector<pud_node_id> leaves;
        for (pud_node_id node : all_nodes()) {
            if (open(node))
                leaves.push_back(node);
        }
        return leaves;
    }

    bool axiom_root(pud_node_id node) {
        return std::find(manifest.axioms_.begin(), manifest.axioms_.end(), node) != manifest.axioms_.end();
    }

    std::vector<pud_node_id> children_of(pud_node_id node) {
        if (leaf(node))
            return {};
        return manifest.children_.get(node);
    }

    std::vector<pud_body_goal> open_goals(pud_node_id node) {
        const auto& goals = manifest.node_goals_.get(node).goals;
        return std::vector<pud_body_goal>(goals.begin(), goals.end());
    }

    pud_body_goal_id first_open_goal(pud_node_id node) { return open_goals(node).front().id; }

    std::optional<pud_mhq_head_id> query_of(pud_node_id leaf_node, pud_body_goal_id goal) {
        const pud_mhq_head_id* head = manifest.leaf_states_.get(leaf_node).queries.find(goal);
        if (!head)
            return std::nullopt;
        return *head;
    }

    bool alive(pud_mhq_head_id head) { return manifest.query_holders_.find_holder(head).has_value(); }

    std::vector<pud_node_id> holders_of(pud_mhq_head_id head) { return manifest.query_holders_.get_holders(head); }

    // Every node, found by walking down from the roots.
    std::vector<pud_node_id> all_nodes() {
        std::vector<pud_node_id> nodes;
        std::vector<pud_node_id> pending(roots.rbegin(), roots.rend());
        while (!pending.empty()) {
            const pud_node_id node = pending.back();
            pending.pop_back();
            nodes.push_back(node);
            const std::vector<pud_node_id> children = children_of(node);
            pending.insert(pending.end(), children.rbegin(), children.rend());
        }
        std::sort(nodes.begin(), nodes.end());
        return nodes;
    }

    std::vector<pud_node_id> live_leaves() {
        std::vector<pud_node_id> leaves;
        for (pud_node_id node : all_nodes()) {
            if (live_leaf(node))
                leaves.push_back(node);
        }
        return leaves;
    }

    std::vector<pud_node_id> path_to(pud_node_id node) {
        std::vector<pud_node_id> path{node};
        while (const std::optional<pud_node_id> parent = manifest.node_parents_.get(path.back()))
            path.push_back(parent.value());
        std::reverse(path.begin(), path.end());
        return path;
    }

    pud_node_id root_of(pud_node_id node) { return path_to(node).front(); }

    size_t most_open_goals() {
        size_t most = 0;
        for (pud_node_id leaf_node : live_leaves())
            most = std::max(most, open_goals(leaf_node).size());
        return most;
    }

    std::set<const expr*> proven_heads() {
        std::set<const expr*> proven;
        for (pud_node_id leaf_node : live_leaves()) {
            const pud_node_id tree = root_of(leaf_node);
            if (!axiom_root(tree) || !open_goals(leaf_node).empty())
                continue;
            proven.insert(manifest.leaf_materializer_.materialize(leaf_node, manifest.root_heads_.get(tree)));
        }
        return proven;
    }

    pud_ubs_state env_of(pud_node_id node) {
        const std::vector<pud_node_id> path = path_to(node);
        pud_ubs_state env{};
        for (size_t index = 1; index < path.size(); ++index)
            env = manifest.binder_.bind(env, path.at(index));
        return env;
    }

    uint32_t rep_in(const pud_ubs_state& env, uint32_t index) { return ubs_bind_map{env}.find(index).rep; }

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

    void check_parents_and_children_agree() {
        for (pud_node_id node : all_nodes()) {
            for (pud_node_id child : children_of(node))
                EXPECT_EQ(manifest.node_parents_.get(child), node) << "child " << child;
            if (root(node))
                continue;
            const std::vector<pud_node_id> siblings = children_of(manifest.node_parents_.get(node).value());
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
            EXPECT_EQ(manifest.live_child_counts_.get(node), live_children) << "node " << node;
        }
    }

    void check_goal_ids_ascend_and_extend_the_parents() {
        for (pud_node_id node : all_nodes()) {
            const std::vector<pud_body_goal> goals = open_goals(node);
            for (size_t index = 1; index < goals.size(); ++index)
                EXPECT_LT(goals.at(index - 1).id, goals.at(index).id) << "node " << node;
            if (root(node))
                continue;
            const pud_node_id parent = manifest.node_parents_.get(node).value();
            const pud_body_goal_id call_site = manifest.call_sites_.get(parent);
            for (const pud_body_goal& goal : open_goals(parent)) {
                const bool inherited = std::find(goals.begin(), goals.end(), goal) != goals.end();
                EXPECT_EQ(inherited, goal.id != call_site) << "node " << node << " goal " << goal.id;
            }
        }
    }

    void check_leaf_binding_matches_its_replayed_substitutions() {
        for (pud_node_id node : live_leaves())
            EXPECT_EQ(manifest.leaf_states_.get(node).bindings, env_of(node)) << "leaf " << node;
    }

    // A goal is written in the representatives of the node that adds it; inherited goals keep the
    // writing of the ancestor that added them.
    void check_stored_goals_use_their_nodes_representatives() {
        for (pud_node_id node : all_nodes()) {
            const pud_ubs_state env = env_of(node);
            const uint32_t lvc = manifest.node_lvcs_.get(node);
            const pud_body_goal_id first_added = root(node) ? 0 : manifest.node_goals_.get(manifest.node_parents_.get(node).value()).next_id;
            for (const pud_body_goal& goal : open_goals(node)) {
                if (goal.id < first_added)
                    continue;
                for (uint32_t index : vars_in(goal.goal)) {
                    EXPECT_LT(index, lvc) << "node " << node;
                    EXPECT_EQ(rep_in(env, index), index) << "node " << node << " var " << index;
                }
            }
        }
    }

    void check_substitutions_meet_the_watchers_preconditions() {
        for (pud_node_id node : all_nodes()) {
            if (root(node))
                continue;
            const pud_node_id parent = manifest.node_parents_.get(node).value();
            const uint32_t parent_lvc = manifest.node_lvcs_.get(parent);
            const uint32_t node_lvc = manifest.node_lvcs_.get(node);
            ubs_bind_map bind_map{env_of(parent)};
            for (const pud_substitution& substitution : manifest.node_substitutions_.get(node)) {
                EXPECT_LT(substitution.var_idx, parent_lvc) << "node " << node;
                const ubs_class target = bind_map.find(substitution.var_idx);
                EXPECT_EQ(target.rep, substitution.var_idx) << "node " << node << " substitutes non-representative";
                EXPECT_FALSE(target.value.has_value()) << "node " << node << " substitutes a bound class";
                if (const expr::var* surviving = std::get_if<expr::var>(&substitution.value->content)) {
                    EXPECT_LT(surviving->index, substitution.var_idx) << "node " << node;
                    EXPECT_EQ(bind_map.find(surviving->index).rep, surviving->index) << "node " << node;
                    bind_map.equate(target, bind_map.find(surviving->index));
                    continue;
                }
                bind_map.instantiate(target, framed_expr{substitution.value, 0});
            }
            const pud_ubs_state env = env_of(node);
            for (const pud_substitution& substitution : manifest.node_substitutions_.get(node)) {
                for (uint32_t index : vars_in(substitution.value)) {
                    EXPECT_LT(index, node_lvc) << "node " << node;
                    EXPECT_EQ(rep_in(env, index), index) << "node " << node << " var " << index;
                }
            }
        }
    }

    void check_fresh_variables_are_dense_from_the_parents_lvc() {
        for (pud_node_id node : all_nodes()) {
            if (root(node))
                continue;
            const pud_node_id parent = manifest.node_parents_.get(node).value();
            const uint32_t parent_lvc = manifest.node_lvcs_.get(parent);
            const pud_body_goal_id first_added = manifest.node_goals_.get(parent).next_id;
            std::set<uint32_t> fresh;
            std::vector<const expr*> added_terms;
            for (const pud_substitution& substitution : manifest.node_substitutions_.get(node))
                added_terms.push_back(substitution.value);
            for (const pud_body_goal& goal : open_goals(node)) {
                if (goal.id >= first_added)
                    added_terms.push_back(goal.goal);
            }
            for (const expr* term : added_terms) {
                for (uint32_t index : vars_in(term)) {
                    if (index >= parent_lvc)
                        fresh.insert(index);
                }
            }
            std::set<uint32_t> expected;
            for (uint32_t index = parent_lvc; index < manifest.node_lvcs_.get(node); ++index)
                expected.insert(index);
            EXPECT_EQ(fresh, expected) << "node " << node;
        }
    }

    void check_every_open_goal_has_a_query_its_leaf_holds() {
        for (pud_node_id node : live_leaves()) {
            for (const pud_body_goal& goal : open_goals(node)) {
                const std::optional<pud_mhq_head_id> head = query_of(node, goal.id);
                ASSERT_TRUE(head.has_value()) << "leaf " << node << " goal " << goal.id;
                const std::vector<pud_node_id> holders = holders_of(head.value());
                EXPECT_NE(std::find(holders.begin(), holders.end(), node), holders.end())
                    << "leaf " << node << " goal " << goal.id;
                EXPECT_FALSE(manifest.mhq_.candidates(head.value()).empty()) << "leaf " << node << " goal " << goal.id;
            }
        }
    }

    void check_every_candidate_path_is_a_real_root_first_chain() {
        for (pud_node_id node : live_leaves()) {
            for (const pud_body_goal& goal : open_goals(node)) {
                for (pud_mhcs_head_id candidate : manifest.mhq_.candidates(query_of(node, goal.id).value())) {
                    const std::vector<pud_node_id>& path = manifest.mhcs_.path(candidate);
                    ASSERT_FALSE(path.empty());
                    EXPECT_TRUE(axiom_root(path.front())) << "candidate " << candidate;
                    for (size_t index = 1; index < path.size(); ++index)
                        EXPECT_EQ(manifest.node_parents_.get(path.at(index)), path.at(index - 1)) << "candidate " << candidate;
                    for (pud_node_id on_path : path)
                        EXPECT_FALSE(refuted(on_path)) << "candidate " << candidate << " passes refuted " << on_path;
                }
            }
        }
    }

    void check_every_candidate_is_acceptable_at_every_leaf_holding_the_query() {
        for (pud_node_id node : live_leaves()) {
            const pud_leaf_state& state = manifest.leaf_states_.get(node);
            for (const pud_body_goal& goal : open_goals(node)) {
                for (pud_mhcs_head_id candidate : manifest.mhq_.candidates(query_of(node, goal.id).value())) {
                    const std::vector<pud_node_id>& path = manifest.mhcs_.path(candidate);
                    EXPECT_NO_THROW(manifest.acceptor_.accept(state.bindings, manifest.node_lvcs_.get(node), goal.goal, path))
                        << "leaf " << node << " goal " << goal.id << " candidate ending at " << path.back();
                }
            }
        }
    }

    void check_all() {
        check_parents_and_children_agree();
        check_live_child_counts_match_unrefuted_children();
        check_goal_ids_ascend_and_extend_the_parents();
        check_leaf_binding_matches_its_replayed_substitutions();
        check_stored_goals_use_their_nodes_representatives();
        check_substitutions_meet_the_watchers_preconditions();
        check_fresh_variables_are_dense_from_the_parents_lvc();
        check_every_open_goal_has_a_query_its_leaf_holds();
        check_every_candidate_path_is_a_real_root_first_chain();
        check_every_candidate_is_acceptable_at_every_leaf_holding_the_query();
    }
};

#endif
