#ifndef PUD_BODY_GOAL_PQ_DESCENT_HPP
#define PUD_BODY_GOAL_PQ_DESCENT_HPP

#include <cstdint>
#include <optional>
#include <variant>
#include <vector>
#include <immer/box.hpp>
#include <immer/map.hpp>
#include "infrastructure/pud_mhq_head_owner.hpp"
#include "value_objects/body_goal_id.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_forced_query_propagation.hpp"
#include "value_objects/pud_mhq_head_id.hpp"
#include "value_objects/pud_watch_head_id.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_ubs_state.hpp"
#include "debug_assert.hpp"

// Each live body goal owns one MHQ query. A child shares its parent's query for every goal
// whose structure did not change, forks the query of every goal whose structure changed, and
// opens a fresh query for every goal it adds; every fork is recorded at the child it happened
// at. Unit and refuted goals are those forced on the step that produced this descent.
template<
    typename IOpenQuery,
    typename ITryAddQueryHead,
    typename ITryForkQueryHead,
    typename IRemoveQueryHead,
    typename IGetCallSite,
    typename IStoreQueryHeadNode,
    typename IMarkQueryForked>
struct pud_body_goal_pq_descent {
    pud_body_goal_pq_descent(
        IOpenQuery& open_query,
        ITryAddQueryHead& try_add_query_head,
        ITryForkQueryHead& try_fork_query_head,
        IRemoveQueryHead& remove_query_head,
        IGetCallSite& get_call_site,
        IStoreQueryHeadNode& store_query_head_node,
        IMarkQueryForked& mark_query_forked,
        pud_node_id root,
        const immer::map<body_goal_id, const expr*>& root_body_goal_exprs,
        const pud_ubs_state& root_ubs_state,
        uint32_t root_frame_offset,
        const std::vector<pud_watch_head_id>& root_watch_head_ids);
    pud_body_goal_pq_descent descend(
        body_goal_id next_body_goal_id,
        pud_node_id parent,
        pud_node_id child,
        const pud_ubs_state& child_ubs_state,
        uint32_t child_frame_offset,
        const immer::map<body_goal_id, const expr*>& child_body_goal_exprs,
        const std::vector<pud_watch_head_id>& structure_changes,
        const std::vector<pud_watch_head_id>& added_watch_head_ids) const;
    pud_watch_head_id find_call_site_watch_head_id(pud_node_id parent) const;
    std::optional<pud_mhq_head_id> find_query_head(body_goal_id goal) const;
    std::optional<body_goal_id> find_goal(pud_mhq_head_id head_id) const;
    const std::vector<body_goal_id>& unit_goals() const;
    const std::vector<body_goal_id>& refuted_goals() const;
private:
    using owner_t = pud_mhq_head_owner<IRemoveQueryHead>;

    void open(
        body_goal_id goal,
        pud_node_id node,
        const expr* goal_expr,
        const pud_ubs_state& ubs_state,
        uint32_t frame_offset,
        pud_watch_head_id watch_head_id);
    void fork(
        body_goal_id goal,
        pud_node_id node,
        const expr* goal_expr,
        const pud_ubs_state& ubs_state,
        uint32_t frame_offset);
    void close(body_goal_id goal);
    void record(
        body_goal_id goal,
        pud_node_id node,
        pud_mhq_head_id head_id,
        const std::optional<pud_forced_query_propagation>& forced_propagation);

    // Pointers rather than references, so the descent stays copy-assignable.
    IOpenQuery* open_query_;
    ITryAddQueryHead* try_add_query_head_;
    ITryForkQueryHead* try_fork_query_head_;
    IRemoveQueryHead* remove_query_head_;
    IGetCallSite* get_call_site_;
    IStoreQueryHeadNode* store_query_head_node_;
    IMarkQueryForked* mark_query_forked_;
    immer::map<body_goal_id, immer::box<owner_t>> body_goal_queries_;
    immer::map<body_goal_id, pud_watch_head_id> body_goal_to_watch_head_;
    immer::map<pud_watch_head_id, body_goal_id> watch_head_to_body_goal_;
    std::vector<body_goal_id> unit_goals_;
    std::vector<body_goal_id> refuted_goals_;
};

#define PUD_BODY_GOAL_PQ_DESCENT_TEMPLATE                                                  \
    template<typename IOpenQuery, typename ITryAddQueryHead, typename ITryForkQueryHead,   \
             typename IRemoveQueryHead, typename IGetCallSite, typename IStoreQueryHeadNode,  \
             typename IMarkQueryForked>
#define PUD_BODY_GOAL_PQ_DESCENT                                                           \
    pud_body_goal_pq_descent<IOpenQuery, ITryAddQueryHead, ITryForkQueryHead,              \
        IRemoveQueryHead, IGetCallSite, IStoreQueryHeadNode, IMarkQueryForked>

PUD_BODY_GOAL_PQ_DESCENT_TEMPLATE
PUD_BODY_GOAL_PQ_DESCENT::pud_body_goal_pq_descent(
    IOpenQuery& open_query,
    ITryAddQueryHead& try_add_query_head,
    ITryForkQueryHead& try_fork_query_head,
    IRemoveQueryHead& remove_query_head,
    IGetCallSite& get_call_site,
    IStoreQueryHeadNode& store_query_head_node,
    IMarkQueryForked& mark_query_forked,
    pud_node_id root,
    const immer::map<body_goal_id, const expr*>& root_body_goal_exprs,
    const pud_ubs_state& root_ubs_state,
    uint32_t root_frame_offset,
    const std::vector<pud_watch_head_id>& root_watch_head_ids)
    : open_query_(&open_query)
    , try_add_query_head_(&try_add_query_head)
    , try_fork_query_head_(&try_fork_query_head)
    , remove_query_head_(&remove_query_head)
    , get_call_site_(&get_call_site)
    , store_query_head_node_(&store_query_head_node)
    , mark_query_forked_(&mark_query_forked)
    , body_goal_queries_()
    , body_goal_to_watch_head_()
    , watch_head_to_body_goal_()
    , unit_goals_()
    , refuted_goals_() {
    for (body_goal_id goal = 0; goal < root_watch_head_ids.size(); ++goal)
        open(goal, root, root_body_goal_exprs.at(goal), root_ubs_state, root_frame_offset, root_watch_head_ids.at(goal));
}

PUD_BODY_GOAL_PQ_DESCENT_TEMPLATE
PUD_BODY_GOAL_PQ_DESCENT PUD_BODY_GOAL_PQ_DESCENT::descend(
    body_goal_id next_body_goal_id,
    pud_node_id parent,
    pud_node_id child,
    const pud_ubs_state& child_ubs_state,
    uint32_t child_frame_offset,
    const immer::map<body_goal_id, const expr*>& child_body_goal_exprs,
    const std::vector<pud_watch_head_id>& structure_changes,
    const std::vector<pud_watch_head_id>& added_watch_head_ids) const {
    PUD_BODY_GOAL_PQ_DESCENT result = *this;
    result.unit_goals_.clear();
    result.refuted_goals_.clear();
    result.close(get_call_site_->get(parent));

    for (pud_watch_head_id watch_head_id : structure_changes) {
        const body_goal_id* goal = result.watch_head_to_body_goal_.find(watch_head_id);
        if (!goal)
            continue;
        result.fork(*goal, child, child_body_goal_exprs.at(*goal), child_ubs_state, child_frame_offset);
    }

    for (size_t added_index = 0; added_index < added_watch_head_ids.size(); ++added_index) {
        const body_goal_id goal = next_body_goal_id + added_index;
        result.open(
            goal,
            child,
            child_body_goal_exprs.at(goal),
            child_ubs_state,
            child_frame_offset,
            added_watch_head_ids.at(added_index));
    }
    return result;
}

PUD_BODY_GOAL_PQ_DESCENT_TEMPLATE
pud_watch_head_id PUD_BODY_GOAL_PQ_DESCENT::find_call_site_watch_head_id(pud_node_id parent) const {
    const pud_watch_head_id* watch_head_id = body_goal_to_watch_head_.find(get_call_site_->get(parent));
    DEBUG_ASSERT(watch_head_id != nullptr);
    return *watch_head_id;
}

PUD_BODY_GOAL_PQ_DESCENT_TEMPLATE
std::optional<pud_mhq_head_id> PUD_BODY_GOAL_PQ_DESCENT::find_query_head(body_goal_id goal) const {
    const immer::box<owner_t>* query = body_goal_queries_.find(goal);
    if (!query)
        return std::nullopt;
    return query->get().head_id();
}

PUD_BODY_GOAL_PQ_DESCENT_TEMPLATE
std::optional<body_goal_id> PUD_BODY_GOAL_PQ_DESCENT::find_goal(pud_mhq_head_id head_id) const {
    for (const auto& [goal, query] : body_goal_queries_) {
        if (query.get().head_id() == head_id)
            return goal;
    }
    return std::nullopt;
}

PUD_BODY_GOAL_PQ_DESCENT_TEMPLATE
const std::vector<body_goal_id>& PUD_BODY_GOAL_PQ_DESCENT::unit_goals() const {
    return unit_goals_;
}

PUD_BODY_GOAL_PQ_DESCENT_TEMPLATE
const std::vector<body_goal_id>& PUD_BODY_GOAL_PQ_DESCENT::refuted_goals() const {
    return refuted_goals_;
}

PUD_BODY_GOAL_PQ_DESCENT_TEMPLATE
void PUD_BODY_GOAL_PQ_DESCENT::open(
    body_goal_id goal,
    pud_node_id node,
    const expr* goal_expr,
    const pud_ubs_state& ubs_state,
    uint32_t frame_offset,
    pud_watch_head_id watch_head_id) {
    body_goal_to_watch_head_ = body_goal_to_watch_head_.set(goal, watch_head_id);
    watch_head_to_body_goal_ = watch_head_to_body_goal_.set(watch_head_id, goal);
    const auto added = try_add_query_head_->try_add_head(open_query_->open(ubs_state, goal_expr, frame_offset));
    record(goal, node, added.head_id, added.forced_propagation);
}

PUD_BODY_GOAL_PQ_DESCENT_TEMPLATE
void PUD_BODY_GOAL_PQ_DESCENT::fork(
    body_goal_id goal,
    pud_node_id node,
    const expr* goal_expr,
    const pud_ubs_state& ubs_state,
    uint32_t frame_offset) {
    const immer::box<owner_t>* parent_query = body_goal_queries_.find(goal);
    if (!parent_query)
        return;
    mark_query_forked_->mark_query_forked(node, parent_query->get().head_id());
    const auto forked = try_fork_query_head_->try_fork_head(
        parent_query->get().head_id(),
        open_query_->open(ubs_state, goal_expr, frame_offset));
    record(goal, node, forked.head_id, forked.forced_propagation);
}

PUD_BODY_GOAL_PQ_DESCENT_TEMPLATE
void PUD_BODY_GOAL_PQ_DESCENT::close(body_goal_id goal) {
    body_goal_queries_ = body_goal_queries_.erase(goal);
    const pud_watch_head_id* watch_head_id = body_goal_to_watch_head_.find(goal);
    if (!watch_head_id)
        return;
    watch_head_to_body_goal_ = watch_head_to_body_goal_.erase(*watch_head_id);
    body_goal_to_watch_head_ = body_goal_to_watch_head_.erase(goal);
}

PUD_BODY_GOAL_PQ_DESCENT_TEMPLATE
void PUD_BODY_GOAL_PQ_DESCENT::record(
    body_goal_id goal,
    pud_node_id node,
    pud_mhq_head_id head_id,
    const std::optional<pud_forced_query_propagation>& forced_propagation) {
    const bool refuted =
        forced_propagation.has_value() && std::holds_alternative<pud_refuted_query>(forced_propagation.value());
    if (refuted) {
        body_goal_queries_ = body_goal_queries_.erase(goal);
        refuted_goals_.push_back(goal);
        return;
    }
    body_goal_queries_ = body_goal_queries_.set(goal, immer::box<owner_t>{*remove_query_head_, head_id});
    store_query_head_node_->store(head_id, node);
    if (!forced_propagation.has_value())
        return;
    unit_goals_.push_back(goal);
}

#undef PUD_BODY_GOAL_PQ_DESCENT
#undef PUD_BODY_GOAL_PQ_DESCENT_TEMPLATE

#endif
