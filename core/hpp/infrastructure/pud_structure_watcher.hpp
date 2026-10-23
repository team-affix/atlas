#ifndef PUD_STRUCTURE_WATCHER_HPP
#define PUD_STRUCTURE_WATCHER_HPP

#include <cstdint>
#include <optional>
#include <unordered_set>
#include <variant>
#include <vector>
#include <immer/set.hpp>
#include "value_objects/expr.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/pud_body_goal.hpp"
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_goal_list.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_structure_watch.hpp"
#include "value_objects/pud_structure_watch_change.hpp"
#include "value_objects/pud_substitution.hpp"
#include "value_objects/pud_ubs_state.hpp"
#include "value_objects/ubs_class.hpp"

// Tells which inherited goals of a child changed structure under the child's substitutions. A goal
// changes when one of its representatives is instantiated or merged with another of its own; merging
// it with a representative it does not mention only renames it.
template<typename IMakeBindMap, typename IGetInference>
struct pud_structure_watcher {
    pud_structure_watcher(IMakeBindMap& make_bind_map, IGetInference& get_inference);
    pud_structure_watch watch_root(const pud_ubs_state& root_bindings, const pud_goal_list& goals) const;
    pud_structure_watch_change descend(
        const pud_structure_watch& parent_watch,
        const pud_ubs_state& child_bindings,
        pud_node_id child,
        pud_body_goal_id closed_goal,
        const pud_goal_list& child_goals) const;
private:
    template<typename IFind>
    void watch(pud_structure_watch& watched, const pud_body_goal& goal, IFind& bind_map) const;
    void unwatch(pud_structure_watch& watched, pud_body_goal_id goal) const;
    std::vector<pud_body_goal_id> propagate_rep_updated(
        pud_structure_watch& watched, uint32_t demoted_rep, uint32_t surviving_rep) const;
    template<typename IFind>
    std::vector<pud_body_goal_id> propagate_rep_instantiated(
        pud_structure_watch& watched, uint32_t rep, const expr* value, IFind& bind_map) const;
    template<typename IFind>
    immer::set<uint32_t> visit_reps(const expr* visited, IFind& bind_map) const;

    IMakeBindMap& make_bind_map_;
    IGetInference& get_inference_;
};

#define PUD_STRUCTURE_WATCHER_TEMPLATE template<typename IMakeBindMap, typename IGetInference>
#define PUD_STRUCTURE_WATCHER pud_structure_watcher<IMakeBindMap, IGetInference>

PUD_STRUCTURE_WATCHER_TEMPLATE
PUD_STRUCTURE_WATCHER::pud_structure_watcher(
    IMakeBindMap& make_bind_map, IGetInference& get_inference)
    : make_bind_map_(make_bind_map)
    , get_inference_(get_inference) {}

PUD_STRUCTURE_WATCHER_TEMPLATE
pud_structure_watch PUD_STRUCTURE_WATCHER::watch_root(const pud_ubs_state& root_bindings, const pud_goal_list& goals) const {
    auto&& bind_map = make_bind_map_.make_bind_map(root_bindings);
    pud_structure_watch watched{.rep_to_goals = {}, .goal_to_reps = {}};
    for (const pud_body_goal& goal : goals.goals)
        watch(watched, goal, bind_map);
    return watched;
}

// A substitution onto a variable still unbound in the child only renames a representative; any other
// substitution instantiates one. The child's own goals are watched after its substitutions.
PUD_STRUCTURE_WATCHER_TEMPLATE
pud_structure_watch_change PUD_STRUCTURE_WATCHER::descend(
    const pud_structure_watch& parent_watch,
    const pud_ubs_state& child_bindings,
    pud_node_id child,
    pud_body_goal_id closed_goal,
    const pud_goal_list& child_goals) const {
    auto&& bind_map = make_bind_map_.make_bind_map(child_bindings);
    pud_structure_watch_change change{.watch = parent_watch, .changed_goals = {}};
    unwatch(change.watch, closed_goal);
    std::unordered_set<pud_body_goal_id> changed_once;
    for (const pud_substitution& substitution : get_inference_.get_inference(child).substitutions) {
        const expr* value = substitution.value;
        std::optional<uint32_t> surviving_rep;
        if (const expr::var* surviving = std::get_if<expr::var>(&value->content)) {
            const ubs_class surviving_class = bind_map.find(surviving->index);
            if (surviving_class.value.has_value())
                value = surviving_class.value->skeleton;
            else
                surviving_rep = surviving->index;
        }
        const std::vector<pud_body_goal_id> changed = surviving_rep.has_value()
            ? propagate_rep_updated(change.watch, substitution.var_idx, surviving_rep.value())
            : propagate_rep_instantiated(change.watch, substitution.var_idx, value, bind_map);
        for (pud_body_goal_id goal : changed) {
            const bool first_change = changed_once.insert(goal).second;
            if (!first_change)
                continue;
            change.changed_goals.push_back(goal);
        }
    }
    for (const pud_body_goal& goal : child_goals.goals) {
        const bool added = change.watch.goal_to_reps.find(goal.id) == nullptr;
        if (!added)
            continue;
        watch(change.watch, goal, bind_map);
    }
    return change;
}

PUD_STRUCTURE_WATCHER_TEMPLATE
template<typename IFind>
void PUD_STRUCTURE_WATCHER::watch(pud_structure_watch& watched, const pud_body_goal& goal, IFind& bind_map) const {
    const immer::set<uint32_t> reps = visit_reps(goal.goal, bind_map);
    watched.goal_to_reps = watched.goal_to_reps.set(goal.id, reps);
    for (uint32_t rep : reps)
        watched.rep_to_goals = watched.rep_to_goals.set(rep, watched.rep_to_goals[rep].insert(goal.id));
}

PUD_STRUCTURE_WATCHER_TEMPLATE
void PUD_STRUCTURE_WATCHER::unwatch(pud_structure_watch& watched, pud_body_goal_id goal) const {
    const immer::set<uint32_t> reps = watched.goal_to_reps[goal];
    for (uint32_t rep : reps) {
        const immer::set<pud_body_goal_id> goals = watched.rep_to_goals[rep].erase(goal);
        watched.rep_to_goals = goals.empty() ? watched.rep_to_goals.erase(rep) : watched.rep_to_goals.set(rep, goals);
    }
    watched.goal_to_reps = watched.goal_to_reps.erase(goal);
}

PUD_STRUCTURE_WATCHER_TEMPLATE
std::vector<pud_body_goal_id> PUD_STRUCTURE_WATCHER::propagate_rep_updated(
    pud_structure_watch& watched, uint32_t demoted_rep, uint32_t surviving_rep) const {
    std::vector<pud_body_goal_id> changed;
    const immer::set<pud_body_goal_id> demoted_goals = watched.rep_to_goals[demoted_rep];
    for (pud_body_goal_id goal : demoted_goals) {
        const immer::set<uint32_t> reps = watched.goal_to_reps[goal];
        const bool collapsed = reps.count(surviving_rep) != 0;
        watched.goal_to_reps = watched.goal_to_reps.set(goal, reps.erase(demoted_rep).insert(surviving_rep));
        watched.rep_to_goals = watched.rep_to_goals.set(surviving_rep, watched.rep_to_goals[surviving_rep].insert(goal));
        if (!collapsed)
            continue;
        changed.push_back(goal);
    }
    watched.rep_to_goals = watched.rep_to_goals.erase(demoted_rep);
    return changed;
}

PUD_STRUCTURE_WATCHER_TEMPLATE
template<typename IFind>
std::vector<pud_body_goal_id> PUD_STRUCTURE_WATCHER::propagate_rep_instantiated(
    pud_structure_watch& watched, uint32_t rep, const expr* value, IFind& bind_map) const {
    const immer::set<uint32_t> introduced_reps = visit_reps(value, bind_map);
    std::vector<pud_body_goal_id> changed;
    const immer::set<pud_body_goal_id> instantiated_goals = watched.rep_to_goals[rep];
    for (pud_body_goal_id goal : instantiated_goals) {
        immer::set<uint32_t> reps = watched.goal_to_reps[goal].erase(rep);
        for (uint32_t introduced_rep : introduced_reps) {
            reps = reps.insert(introduced_rep);
            watched.rep_to_goals = watched.rep_to_goals.set(introduced_rep, watched.rep_to_goals[introduced_rep].insert(goal));
        }
        watched.goal_to_reps = watched.goal_to_reps.set(goal, reps);
        changed.push_back(goal);
    }
    watched.rep_to_goals = watched.rep_to_goals.erase(rep);
    return changed;
}

// Terms are hash-consed DAGs, so each functor subterm and each bound class is visited once.
PUD_STRUCTURE_WATCHER_TEMPLATE
template<typename IFind>
immer::set<uint32_t> PUD_STRUCTURE_WATCHER::visit_reps(const expr* visited, IFind& bind_map) const {
    immer::set<uint32_t> reps;
    std::vector<const expr*> pending{visited};
    std::unordered_set<const expr*> visited_functors;
    std::unordered_set<uint32_t> expanded_roots;
    while (!pending.empty()) {
        const expr* next = pending.back();
        pending.pop_back();
        if (const expr::var* var = std::get_if<expr::var>(&next->content)) {
            const ubs_class var_class = bind_map.find(var->index);
            if (!var_class.value.has_value()) {
                reps = reps.insert(var_class.rep);
                continue;
            }
            if (expanded_roots.insert(var_class.root).second)
                pending.push_back(var_class.value->skeleton);
            continue;
        }
        if (!visited_functors.insert(next).second)
            continue;
        for (const expr* arg : std::get<expr::functor>(next->content).args)
            pending.push_back(arg);
    }
    return reps;
}

#undef PUD_STRUCTURE_WATCHER
#undef PUD_STRUCTURE_WATCHER_TEMPLATE

#endif
