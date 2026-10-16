#ifndef PUD_STRUCTURE_WATCH_DESCENT_HPP
#define PUD_STRUCTURE_WATCH_DESCENT_HPP

#include <cstdint>
#include <unordered_set>
#include <variant>
#include <vector>
#include <immer/map.hpp>
#include <immer/set.hpp>
#include "value_objects/expr.hpp"
#include "value_objects/pud_mhsw_head_id.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_substitution.hpp"

template<typename IGetNodeAddedSubstitutions>
struct pud_structure_watch_descent {
    struct descend_result {
        std::vector<pud_mhsw_head_id> structure_changes;
        std::vector<pud_mhsw_head_id> added_head_ids;
        pud_structure_watch_descent child_descent;
    };

    pud_structure_watch_descent(
        IGetNodeAddedSubstitutions& get_node_added_substitutions,
        const std::vector<const expr*>& watched);
    descend_result descend(
        pud_node_id child,
        const std::vector<pud_mhsw_head_id>& unwatch,
        const std::vector<const expr*>& watch) const;
private:
    using head_set_t = immer::set<pud_mhsw_head_id>;
    using rep_set_t = immer::set<uint32_t>;

    pud_mhsw_head_id watch(const expr* watched);
    void unwatch(pud_mhsw_head_id head_id);
    std::vector<pud_mhsw_head_id> propagate_rep_updated(uint32_t demoted_rep, uint32_t surviving_rep);
    std::vector<pud_mhsw_head_id> propagate_rep_instantiated(uint32_t rep, const expr* value);
    void visit_reps(const expr* visited, rep_set_t& reps) const;

    IGetNodeAddedSubstitutions* get_node_added_substitutions_;
    pud_mhsw_head_id next_head_id_;
    immer::map<uint32_t, head_set_t> rep_to_heads_;
    immer::map<pud_mhsw_head_id, rep_set_t> head_to_reps_;
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
    for (const expr* watched_expr : watched)
        watch(watched_expr);
}

PUD_STRUCTURE_WATCH_DESCENT_TEMPLATE
typename PUD_STRUCTURE_WATCH_DESCENT::descend_result PUD_STRUCTURE_WATCH_DESCENT::descend(
    pud_node_id child,
    const std::vector<pud_mhsw_head_id>& unwatch,
    const std::vector<const expr*>& watch) const {
    PUD_STRUCTURE_WATCH_DESCENT child_descent = *this;
    for (pud_mhsw_head_id head_id : unwatch)
        child_descent.unwatch(head_id);

    std::vector<pud_mhsw_head_id> structure_changes;
    std::unordered_set<pud_mhsw_head_id> changed_once;
    for (const pud_substitution& substitution : get_node_added_substitutions_->get(child)) {
        const expr::var* surviving = std::get_if<expr::var>(&substitution.value->content);
        const std::vector<pud_mhsw_head_id> changed = surviving
            ? child_descent.propagate_rep_updated(substitution.var_idx, surviving->index)
            : child_descent.propagate_rep_instantiated(substitution.var_idx, substitution.value);
        for (pud_mhsw_head_id head_id : changed) {
            const bool first_change = changed_once.insert(head_id).second;
            if (!first_change)
                continue;
            structure_changes.push_back(head_id);
        }
    }

    std::vector<pud_mhsw_head_id> added_head_ids;
    added_head_ids.reserve(watch.size());
    for (const expr* watched_expr : watch)
        added_head_ids.push_back(child_descent.watch(watched_expr));

    return descend_result{
        .structure_changes = structure_changes,
        .added_head_ids = added_head_ids,
        .child_descent = child_descent,
    };
}

PUD_STRUCTURE_WATCH_DESCENT_TEMPLATE
pud_mhsw_head_id PUD_STRUCTURE_WATCH_DESCENT::watch(const expr* watched) {
    rep_set_t reps;
    visit_reps(watched, reps);
    const pud_mhsw_head_id head_id = next_head_id_++;
    head_to_reps_ = head_to_reps_.set(head_id, reps);
    for (uint32_t rep : reps)
        rep_to_heads_ = rep_to_heads_.set(rep, rep_to_heads_[rep].insert(head_id));
    return head_id;
}

PUD_STRUCTURE_WATCH_DESCENT_TEMPLATE
void PUD_STRUCTURE_WATCH_DESCENT::unwatch(pud_mhsw_head_id head_id) {
    const rep_set_t reps = head_to_reps_[head_id];
    for (uint32_t rep : reps) {
        const head_set_t heads = rep_to_heads_[rep].erase(head_id);
        rep_to_heads_ = heads.empty() ? rep_to_heads_.erase(rep) : rep_to_heads_.set(rep, heads);
    }
    head_to_reps_ = head_to_reps_.erase(head_id);
}

PUD_STRUCTURE_WATCH_DESCENT_TEMPLATE
std::vector<pud_mhsw_head_id> PUD_STRUCTURE_WATCH_DESCENT::propagate_rep_updated(
    uint32_t demoted_rep,
    uint32_t surviving_rep) {
    std::vector<pud_mhsw_head_id> changed;
    const head_set_t demoted_heads = rep_to_heads_[demoted_rep];
    for (pud_mhsw_head_id head_id : demoted_heads) {
        const rep_set_t reps = head_to_reps_[head_id];
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
std::vector<pud_mhsw_head_id> PUD_STRUCTURE_WATCH_DESCENT::propagate_rep_instantiated(
    uint32_t rep,
    const expr* value) {
    rep_set_t introduced_reps;
    visit_reps(value, introduced_reps);
    std::vector<pud_mhsw_head_id> changed;
    const head_set_t instantiated_heads = rep_to_heads_[rep];
    for (pud_mhsw_head_id head_id : instantiated_heads) {
        rep_set_t reps = head_to_reps_[head_id].erase(rep);
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
void PUD_STRUCTURE_WATCH_DESCENT::visit_reps(const expr* visited, rep_set_t& reps) const {
    std::vector<const expr*> pending{visited};
    while (!pending.empty()) {
        const expr* next = pending.back();
        pending.pop_back();
        if (const expr::var* var = std::get_if<expr::var>(&next->content)) {
            reps = reps.insert(var->index);
            continue;
        }
        for (const expr* arg : std::get<expr::functor>(next->content).args)
            pending.push_back(arg);
    }
}

#undef PUD_STRUCTURE_WATCH_DESCENT
#undef PUD_STRUCTURE_WATCH_DESCENT_TEMPLATE

#endif
