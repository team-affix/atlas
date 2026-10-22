#ifndef PUD_STRUCTURE_WATCH_DESCENT_HPP
#define PUD_STRUCTURE_WATCH_DESCENT_HPP

#include <cstdint>
#include <optional>
#include <unordered_set>
#include <variant>
#include <vector>
#include <immer/map.hpp>
#include <immer/set.hpp>
#include "infrastructure/pud_transient_ubs_state.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/pud_watch_head_id.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_structure_watch_result.hpp"
#include "value_objects/pud_substitution.hpp"
#include "value_objects/pud_ubs_state.hpp"

template<typename IGetNodeAddedSubstitutions>
struct pud_structure_watch_descent {
    pud_structure_watch_descent(
        IGetNodeAddedSubstitutions& get_node_added_substitutions,
        const std::vector<const expr*>& watched);
    pud_structure_watch_result<pud_structure_watch_descent> descend(
        pud_node_id child,
        const std::vector<pud_watch_head_id>& unwatch,
        const std::vector<const expr*>& watch,
        const pud_ubs_state& child_ubs_state) const;
private:
    template<typename BindMap>
    pud_watch_head_id watch(const expr* watched, BindMap& bind_map);
    void unwatch(pud_watch_head_id head_id);
    std::vector<pud_watch_head_id> propagate_rep_updated(uint32_t demoted_rep, uint32_t surviving_rep);
    template<typename BindMap>
    std::vector<pud_watch_head_id> propagate_rep_instantiated(uint32_t rep, const expr* value, BindMap& bind_map);
    template<typename BindMap>
    void visit_reps(const expr* visited, immer::set<uint32_t>& reps, BindMap& bind_map) const;

    IGetNodeAddedSubstitutions* get_node_added_substitutions_;
    pud_watch_head_id next_head_id_;
    immer::map<uint32_t, immer::set<pud_watch_head_id>> rep_to_heads_;
    immer::map<pud_watch_head_id, immer::set<uint32_t>> head_to_reps_;
};

#define PUD_STRUCTURE_WATCH_DESCENT_TEMPLATE                                               \
    template<typename IGetNodeAddedSubstitutions>
#define PUD_STRUCTURE_WATCH_DESCENT                                                        \
    pud_structure_watch_descent<IGetNodeAddedSubstitutions>

PUD_STRUCTURE_WATCH_DESCENT_TEMPLATE
PUD_STRUCTURE_WATCH_DESCENT::pud_structure_watch_descent(
    IGetNodeAddedSubstitutions& get_node_added_substitutions,
    const std::vector<const expr*>& watched)
    : get_node_added_substitutions_(&get_node_added_substitutions)
    , next_head_id_(0)
    , rep_to_heads_()
    , head_to_reps_() {
    pud_transient_ubs_state empty_ubs_state{pud_ubs_state{}};
    auto bind_map = empty_ubs_state.make_bind_map();
    for (const expr* watched_expr : watched)
        watch(watched_expr, bind_map);
}

PUD_STRUCTURE_WATCH_DESCENT_TEMPLATE
pud_structure_watch_result<PUD_STRUCTURE_WATCH_DESCENT> PUD_STRUCTURE_WATCH_DESCENT::descend(
    pud_node_id child,
    const std::vector<pud_watch_head_id>& unwatch,
    const std::vector<const expr*>& watch,
    const pud_ubs_state& child_ubs_state) const {
    pud_transient_ubs_state transient_ubs_state{child_ubs_state};
    auto bind_map = transient_ubs_state.make_bind_map();
    PUD_STRUCTURE_WATCH_DESCENT child_descent = *this;
    for (pud_watch_head_id head_id : unwatch)
        child_descent.unwatch(head_id);

    std::vector<pud_watch_head_id> structure_changes;
    std::unordered_set<pud_watch_head_id> changed_once;
    for (const pud_substitution& substitution : get_node_added_substitutions_->get(child)) {
        const expr::var* surviving = std::get_if<expr::var>(&substitution.value->content);
        const std::optional<framed_expr> surviving_value =
            surviving ? bind_map.find_value(surviving->index) : std::nullopt;
        const std::vector<pud_watch_head_id> changed = !surviving
            ? child_descent.propagate_rep_instantiated(substitution.var_idx, substitution.value, bind_map)
            : surviving_value.has_value()
            ? child_descent.propagate_rep_instantiated(substitution.var_idx, surviving_value->skeleton, bind_map)
            : child_descent.propagate_rep_updated(substitution.var_idx, surviving->index);
        for (pud_watch_head_id head_id : changed) {
            const bool first_change = changed_once.insert(head_id).second;
            if (!first_change)
                continue;
            structure_changes.push_back(head_id);
        }
    }

    std::vector<pud_watch_head_id> added_head_ids;
    added_head_ids.reserve(watch.size());
    for (const expr* watched_expr : watch)
        added_head_ids.push_back(child_descent.watch(watched_expr, bind_map));

    return pud_structure_watch_result<PUD_STRUCTURE_WATCH_DESCENT>{
        .structure_changes = structure_changes,
        .added_head_ids = added_head_ids,
        .child_descent = child_descent,
    };
}

PUD_STRUCTURE_WATCH_DESCENT_TEMPLATE
template<typename BindMap>
pud_watch_head_id PUD_STRUCTURE_WATCH_DESCENT::watch(const expr* watched, BindMap& bind_map) {
    immer::set<uint32_t> reps;
    visit_reps(watched, reps, bind_map);
    const pud_watch_head_id head_id = next_head_id_++;
    head_to_reps_ = head_to_reps_.set(head_id, reps);
    for (uint32_t rep : reps)
        rep_to_heads_ = rep_to_heads_.set(rep, rep_to_heads_[rep].insert(head_id));
    return head_id;
}

PUD_STRUCTURE_WATCH_DESCENT_TEMPLATE
void PUD_STRUCTURE_WATCH_DESCENT::unwatch(pud_watch_head_id head_id) {
    const immer::set<uint32_t> reps = head_to_reps_[head_id];
    for (uint32_t rep : reps) {
        const immer::set<pud_watch_head_id> heads = rep_to_heads_[rep].erase(head_id);
        rep_to_heads_ = heads.empty() ? rep_to_heads_.erase(rep) : rep_to_heads_.set(rep, heads);
    }
    head_to_reps_ = head_to_reps_.erase(head_id);
}

PUD_STRUCTURE_WATCH_DESCENT_TEMPLATE
std::vector<pud_watch_head_id> PUD_STRUCTURE_WATCH_DESCENT::propagate_rep_updated(
    uint32_t demoted_rep,
    uint32_t surviving_rep) {
    std::vector<pud_watch_head_id> changed;
    const immer::set<pud_watch_head_id> demoted_heads = rep_to_heads_[demoted_rep];
    for (pud_watch_head_id head_id : demoted_heads) {
        const immer::set<uint32_t> reps = head_to_reps_[head_id];
        const bool collapsed = reps.count(surviving_rep) != 0;
        head_to_reps_ = head_to_reps_.set(head_id, reps.erase(demoted_rep).insert(surviving_rep));
        rep_to_heads_ = rep_to_heads_.set(surviving_rep, rep_to_heads_[surviving_rep].insert(head_id));
        if (!collapsed)
            continue;
        changed.push_back(head_id);
    }
    rep_to_heads_ = rep_to_heads_.erase(demoted_rep);
    return changed;
}

PUD_STRUCTURE_WATCH_DESCENT_TEMPLATE
template<typename BindMap>
std::vector<pud_watch_head_id> PUD_STRUCTURE_WATCH_DESCENT::propagate_rep_instantiated(
    uint32_t rep,
    const expr* value,
    BindMap& bind_map) {
    immer::set<uint32_t> introduced_reps;
    visit_reps(value, introduced_reps, bind_map);
    std::vector<pud_watch_head_id> changed;
    const immer::set<pud_watch_head_id> instantiated_heads = rep_to_heads_[rep];
    for (pud_watch_head_id head_id : instantiated_heads) {
        immer::set<uint32_t> reps = head_to_reps_[head_id].erase(rep);
        for (uint32_t introduced_rep : introduced_reps) {
            reps = reps.insert(introduced_rep);
            rep_to_heads_ = rep_to_heads_.set(introduced_rep, rep_to_heads_[introduced_rep].insert(head_id));
        }
        head_to_reps_ = head_to_reps_.set(head_id, reps);
        changed.push_back(head_id);
    }
    rep_to_heads_ = rep_to_heads_.erase(rep);
    return changed;
}

PUD_STRUCTURE_WATCH_DESCENT_TEMPLATE
template<typename BindMap>
void PUD_STRUCTURE_WATCH_DESCENT::visit_reps(
    const expr* visited, immer::set<uint32_t>& reps, BindMap& bind_map) const {
    std::vector<const expr*> pending{visited};
    std::unordered_set<uint32_t> expanded_roots;
    while (!pending.empty()) {
        const expr* next = pending.back();
        pending.pop_back();
        if (const expr::var* var = std::get_if<expr::var>(&next->content)) {
            const std::optional<framed_expr> value = bind_map.find_value(var->index);
            if (!value.has_value()) {
                reps = reps.insert(bind_map.find_rep(var->index));
                continue;
            }
            if (expanded_roots.insert(bind_map.find(var->index)).second)
                pending.push_back(value->skeleton);
            continue;
        }
        for (const expr* arg : std::get<expr::functor>(next->content).args)
            pending.push_back(arg);
    }
}

#undef PUD_STRUCTURE_WATCH_DESCENT
#undef PUD_STRUCTURE_WATCH_DESCENT_TEMPLATE

#endif
