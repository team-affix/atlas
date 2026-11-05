#ifndef PUD_NODE_STATE_CACHE_HPP
#define PUD_NODE_STATE_CACHE_HPP

#include <cstddef>
#include <list>
#include <optional>
#include <unordered_map>
#include <vector>
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_global_node_id_hash.hpp"
#include "value_objects/pud_goal_list.hpp"
#include "value_objects/pud_inference.hpp"
#include "value_objects/pud_node_state.hpp"
#include "value_objects/pud_ubs_state.hpp"
#include "value_objects/rule.hpp"

// The node states of the stored tree, worked out on demand and kept while they fit. A node's state is a fold along its
// path: its root axiom's goals, variables and empty bindings, then each inference's closed goal, new goals, new
// variables and substitutions. A miss folds down from the nearest kept ancestor; terms are interned, so a state worked
// out again equals the one dropped. States are dropped, least recently used first, only when trimmed: a state read in
// a search step stays valid until the step ends.
template<
    typename IGetParent,
    typename IGetAxiom,
    typename IGetInference,
    typename IBuildRootGoalList,
    typename IBuildChildGoalList,
    typename IBind>
struct pud_node_state_cache {
    pud_node_state_cache(
        IGetParent& get_parent,
        IGetAxiom& get_axiom,
        IGetInference& get_inference,
        IBuildRootGoalList& build_root_goal_list,
        IBuildChildGoalList& build_child_goal_list,
        IBind& bind,
        size_t budget_bytes);
    const pud_node_state& get(pud_global_node_id node);
    void evict(pud_global_node_id node);
    void trim();
    size_t hits() const;
    size_t misses() const;
private:
    using recency_t = std::list<pud_global_node_id>;
    struct entry {
        pud_node_state state;
        size_t bytes;
        recency_t::iterator recency;
    };

    pud_node_state fold_root(pud_global_node_id root) const;
    pud_node_state fold_child(const pud_node_state& parent, pud_global_node_id child) const;
    void keep(pud_global_node_id node, pud_node_state state);

    IGetParent& get_parent_;
    IGetAxiom& get_axiom_;
    IGetInference& get_inference_;
    IBuildRootGoalList& build_root_goal_list_;
    IBuildChildGoalList& build_child_goal_list_;
    IBind& bind_;
    size_t budget_bytes_;
    size_t kept_bytes_;
    size_t hits_;
    size_t misses_;
    recency_t recency_;
    std::unordered_map<pud_global_node_id, entry, pud_global_node_id_hash> entries_;
};

#define PUD_NODE_STATE_CACHE_TEMPLATE                                                                    \
    template<typename IGetParent, typename IGetAxiom, typename IGetInference,                            \
             typename IBuildRootGoalList, typename IBuildChildGoalList, typename IBind>
#define PUD_NODE_STATE_CACHE                                                                             \
    pud_node_state_cache<IGetParent, IGetAxiom, IGetInference, IBuildRootGoalList, IBuildChildGoalList, IBind>

PUD_NODE_STATE_CACHE_TEMPLATE
PUD_NODE_STATE_CACHE::pud_node_state_cache(
    IGetParent& get_parent,
    IGetAxiom& get_axiom,
    IGetInference& get_inference,
    IBuildRootGoalList& build_root_goal_list,
    IBuildChildGoalList& build_child_goal_list,
    IBind& bind,
    size_t budget_bytes)
    : get_parent_(get_parent)
    , get_axiom_(get_axiom)
    , get_inference_(get_inference)
    , build_root_goal_list_(build_root_goal_list)
    , build_child_goal_list_(build_child_goal_list)
    , bind_(bind)
    , budget_bytes_(budget_bytes)
    , kept_bytes_(0)
    , hits_(0)
    , misses_(0)
    , recency_()
    , entries_() {}

PUD_NODE_STATE_CACHE_TEMPLATE
const pud_node_state& PUD_NODE_STATE_CACHE::get(pud_global_node_id node) {
    const auto found = entries_.find(node);
    if (found != entries_.end()) {
        ++hits_;
        recency_.splice(recency_.begin(), recency_, found->second.recency);
        return found->second.state;
    }
    ++misses_;
    std::vector<pud_global_node_id> missing{node};
    std::optional<pud_global_node_id> above = get_parent_.get_parent(node);
    while (above.has_value() && !entries_.contains(above.value())) {
        missing.push_back(above.value());
        above = get_parent_.get_parent(above.value());
    }
    for (auto step = missing.rbegin(); step != missing.rend(); ++step) {
        const std::optional<pud_global_node_id> parent = get_parent_.get_parent(*step);
        if (!parent.has_value()) {
            keep(*step, fold_root(*step));
            continue;
        }
        keep(*step, fold_child(entries_.at(parent.value()).state, *step));
    }
    return entries_.at(node).state;
}

PUD_NODE_STATE_CACHE_TEMPLATE
void PUD_NODE_STATE_CACHE::evict(pud_global_node_id node) {
    const auto found = entries_.find(node);
    if (found == entries_.end())
        return;
    kept_bytes_ -= found->second.bytes;
    recency_.erase(found->second.recency);
    entries_.erase(found);
}

PUD_NODE_STATE_CACHE_TEMPLATE
void PUD_NODE_STATE_CACHE::trim() {
    while (kept_bytes_ > budget_bytes_ && !recency_.empty())
        evict(recency_.back());
}

PUD_NODE_STATE_CACHE_TEMPLATE
size_t PUD_NODE_STATE_CACHE::hits() const {
    return hits_;
}

PUD_NODE_STATE_CACHE_TEMPLATE
size_t PUD_NODE_STATE_CACHE::misses() const {
    return misses_;
}

PUD_NODE_STATE_CACHE_TEMPLATE
pud_node_state PUD_NODE_STATE_CACHE::fold_root(pud_global_node_id root) const {
    const rule& axiom = get_axiom_.get_axiom(root);
    return pud_node_state{
        .goals = build_root_goal_list_.build(axiom.body),
        .axiom = root,
        .depth = 0,
        .lvc = axiom.var_count,
        .bindings = pud_ubs_state{}};
}

PUD_NODE_STATE_CACHE_TEMPLATE
pud_node_state PUD_NODE_STATE_CACHE::fold_child(const pud_node_state& parent, pud_global_node_id child) const {
    const pud_inference& inference = get_inference_.get_inference(child);
    return pud_node_state{
        .goals = build_child_goal_list_.build(parent.goals, inference.call_site, inference.body_goals),
        .axiom = parent.axiom,
        .depth = parent.depth + 1,
        .lvc = parent.lvc + inference.var_count,
        .bindings = bind_.bind(parent.bindings, child)};
}

// A state's size is estimated from its map entries: each costs its value plus a share of the map's nodes.
PUD_NODE_STATE_CACHE_TEMPLATE
void PUD_NODE_STATE_CACHE::keep(pud_global_node_id node, pud_node_state state) {
    const size_t bytes = sizeof(entry) + 2 * sizeof(pud_global_node_id) + 32 * state.goals.goals.size()
                         + 16 * state.bindings.parents.size() + 40 * state.bindings.classes.size();
    recency_.push_front(node);
    entries_.emplace(node, entry{.state = std::move(state), .bytes = bytes, .recency = recency_.begin()});
    kept_bytes_ += bytes;
}

#undef PUD_NODE_STATE_CACHE
#undef PUD_NODE_STATE_CACHE_TEMPLATE

#endif
