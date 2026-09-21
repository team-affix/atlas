#ifndef PUD_QUERIES_HPP
#define PUD_QUERIES_HPP

#include <algorithm>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>
#include "value_objects/expr.hpp"
#include "value_objects/om_interval.hpp"
#include "value_objects/pud_candidate_search_context.hpp"
#include "value_objects/pud_candidate_search_result.hpp"
#include "value_objects/pud_forced_unfold.hpp"
#include "value_objects/pud_query.hpp"
#include "value_objects/pud_rule_id.hpp"
#include "value_objects/pud_unfold_site.hpp"
#include "value_objects/pud_witness_search_context.hpp"
#include "value_objects/pud_witness_search_result.hpp"
#include "debug_assert.hpp"

template<typename IGetAddedBodyGoals,
         typename IGetLvc,
         typename IGetBaseInterval,
         typename IAllocateChildInterval,
         typename IDropQueryEnv,
         typename IResumeCandidateSearch,
         typename IResumeWitnessSearch>
struct pud_queries {
    pud_queries(IGetAddedBodyGoals& get_added_body_goals,
                IGetLvc& get_lvc,
                IGetBaseInterval& get_base_interval,
                IAllocateChildInterval& allocate_child_interval,
                IDropQueryEnv& drop_query_env,
                IResumeCandidateSearch& resume_candidate_search,
                IResumeWitnessSearch& resume_witness_search);
    void adopt_axiom(const pud_rule_id* axiom);
    pud_unfold_site unfold_site(const pud_rule_id* leaf, size_t body_goal_idx);
    std::vector<pud_forced_unfold> replace_unfolded(const pud_rule_id* leaf,
                                                    size_t body_goal_idx,
                                                    const std::vector<const pud_rule_id*>& children);
private:
    using owned_t = std::vector<std::unique_ptr<pud_query>>;
    using ptrs_t = std::vector<pud_query*>;
    using query_set_t = std::unordered_set<pud_query*>;
    using witness_set_t = std::unordered_set<const pud_rule_id*>;
    using leaf_set_t = std::unordered_set<const pud_rule_id*>;
    struct rule_id_less {
        bool operator()(const pud_rule_id* a, const pud_rule_id* b) const;
    };

    void ensure_booted();
    void boot();
    void install(const pud_rule_id* leaf);
    void fork_child(const pud_rule_id* child,
                    const std::vector<pud_query>& leftover_templates);
    void clear(const pud_rule_id* leaf);
    void invalidate_leaf(const pud_rule_id* node);
    void commit_queries(const pud_rule_id* leaf, std::vector<pud_query> queries);
    void replace_owned(const pud_rule_id* leaf, std::vector<pud_query> queries);
    void resume_query(pud_query& query);
    void watch_query(pud_query& query);
    void watch_context(pud_query& query, const pud_candidate_search_context& axiom_ctx);
    void unwatch_query(pud_query* query);
    void watch(const pud_rule_id* witness, pud_query* query);
    void mark_dirty(const pud_rule_id* leaf);
    void resume_dead_witness(pud_query& query, const pud_rule_id* node);
    std::vector<const pud_rule_id*> live_cursors(pud_query& query);
    std::vector<pud_candidate_search_context> root_contexts();
    std::vector<const pud_rule_id*> take_dirty_leaves();
    const std::vector<pud_query*>& get(const pud_rule_id* leaf);
    std::vector<pud_forced_unfold> take_forced_unfolds();

    IGetAddedBodyGoals& get_added_body_goals_;
    IGetLvc& get_lvc_;
    IGetBaseInterval& get_base_interval_;
    IAllocateChildInterval& allocate_child_interval_;
    IDropQueryEnv& drop_query_env_;
    IResumeCandidateSearch& resume_candidate_search_;
    IResumeWitnessSearch& resume_witness_search_;
    std::vector<const pud_rule_id*> axioms_;
    std::unordered_map<const pud_rule_id*, owned_t> owned_;
    std::unordered_map<const pud_rule_id*, ptrs_t> ptrs_;
    std::unordered_map<pud_query*, const pud_rule_id*> query_leaf_;
    std::unordered_map<const pud_rule_id*, query_set_t> by_witness_;
    std::unordered_map<pud_query*, witness_set_t> by_query_;
    leaf_set_t dirty_leaves_;
    bool booted_;
};

template<typename IGABG, typename IGL, typename IGBI, typename IACI, typename IDQE,
         typename IRCS, typename IRWS>
bool pud_queries<IGABG, IGL, IGBI, IACI, IDQE, IRCS, IRWS>::rule_id_less::operator()(
        const pud_rule_id* a, const pud_rule_id* b) const {
    return *a < *b;
}

template<typename IGABG, typename IGL, typename IGBI, typename IACI, typename IDQE,
         typename IRCS, typename IRWS>
pud_queries<IGABG, IGL, IGBI, IACI, IDQE, IRCS, IRWS>::pud_queries(
        IGABG& get_added_body_goals,
        IGL& get_lvc,
        IGBI& get_base_interval,
        IACI& allocate_child_interval,
        IDQE& drop_query_env,
        IRCS& resume_candidate_search,
        IRWS& resume_witness_search)
    : get_added_body_goals_(get_added_body_goals)
    , get_lvc_(get_lvc)
    , get_base_interval_(get_base_interval)
    , allocate_child_interval_(allocate_child_interval)
    , drop_query_env_(drop_query_env)
    , resume_candidate_search_(resume_candidate_search)
    , resume_witness_search_(resume_witness_search)
    , axioms_()
    , owned_()
    , ptrs_()
    , query_leaf_()
    , by_witness_()
    , by_query_()
    , dirty_leaves_()
    , booted_(false) {}

template<typename IGABG, typename IGL, typename IGBI, typename IACI, typename IDQE,
         typename IRCS, typename IRWS>
std::vector<pud_candidate_search_context>
pud_queries<IGABG, IGL, IGBI, IACI, IDQE, IRCS, IRWS>::root_contexts() {
    std::vector<pud_candidate_search_context> axiom_contexts;
    for (const pud_rule_id* axiom : axioms_) {
        axiom_contexts.push_back(pud_candidate_search_context{
            axiom, {}, get_added_body_goals_.get(axiom)});
    }
    return axiom_contexts;
}

template<typename IGABG, typename IGL, typename IGBI, typename IACI, typename IDQE,
         typename IRCS, typename IRWS>
void pud_queries<IGABG, IGL, IGBI, IACI, IDQE, IRCS, IRWS>::replace_owned(
        const pud_rule_id* leaf, std::vector<pud_query> queries) {
    DEBUG_ASSERT(!owned_.contains(leaf));
    owned_t owned;
    ptrs_t ptrs;
    owned.reserve(queries.size());
    ptrs.reserve(queries.size());
    for (pud_query& query : queries) {
        owned.push_back(std::make_unique<pud_query>(std::move(query)));
        pud_query* ptr = owned.back().get();
        ptrs.push_back(ptr);
        query_leaf_[ptr] = leaf;
    }
    owned_[leaf] = std::move(owned);
    ptrs_[leaf] = std::move(ptrs);
}

template<typename IGABG, typename IGL, typename IGBI, typename IACI, typename IDQE,
         typename IRCS, typename IRWS>
void pud_queries<IGABG, IGL, IGBI, IACI, IDQE, IRCS, IRWS>::watch(
        const pud_rule_id* witness, pud_query* query) {
    by_witness_[witness].insert(query);
    by_query_[query].insert(witness);
}

template<typename IGABG, typename IGL, typename IGBI, typename IACI, typename IDQE,
         typename IRCS, typename IRWS>
void pud_queries<IGABG, IGL, IGBI, IACI, IDQE, IRCS, IRWS>::watch_context(
        pud_query& query, const pud_candidate_search_context& axiom_ctx) {
    if (axiom_ctx.live_edges.empty()) {
        watch(axiom_ctx.cursor, &query);
        return;
    }
    for (const pud_witness_search_context& edge : axiom_ctx.live_edges)
        watch(edge.current, &query);
}

template<typename IGABG, typename IGL, typename IGBI, typename IACI, typename IDQE,
         typename IRCS, typename IRWS>
void pud_queries<IGABG, IGL, IGBI, IACI, IDQE, IRCS, IRWS>::watch_query(pud_query& query) {
    for (const pud_candidate_search_context& axiom_ctx : query.axiom_contexts)
        watch_context(query, axiom_ctx);
}

template<typename IGABG, typename IGL, typename IGBI, typename IACI, typename IDQE,
         typename IRCS, typename IRWS>
void pud_queries<IGABG, IGL, IGBI, IACI, IDQE, IRCS, IRWS>::unwatch_query(pud_query* query) {
    auto query_it = by_query_.find(query);
    if (query_it == by_query_.end())
        return;
    const witness_set_t witnesses = query_it->second;
    by_query_.erase(query_it);
    for (const pud_rule_id* witness : witnesses) {
        auto witness_it = by_witness_.find(witness);
        if (witness_it == by_witness_.end())
            continue;
        witness_it->second.erase(query);
        if (witness_it->second.empty())
            by_witness_.erase(witness_it);
    }
}

template<typename IGABG, typename IGL, typename IGBI, typename IACI, typename IDQE,
         typename IRCS, typename IRWS>
void pud_queries<IGABG, IGL, IGBI, IACI, IDQE, IRCS, IRWS>::resume_query(pud_query& query) {
    for (pud_candidate_search_context& axiom_ctx : query.axiom_contexts)
        resume_candidate_search_.resume(query, axiom_ctx);
}

template<typename IGABG, typename IGL, typename IGBI, typename IACI, typename IDQE,
         typename IRCS, typename IRWS>
void pud_queries<IGABG, IGL, IGBI, IACI, IDQE, IRCS, IRWS>::mark_dirty(const pud_rule_id* leaf) {
    dirty_leaves_.insert(leaf);
}

template<typename IGABG, typename IGL, typename IGBI, typename IACI, typename IDQE,
         typename IRCS, typename IRWS>
void pud_queries<IGABG, IGL, IGBI, IACI, IDQE, IRCS, IRWS>::commit_queries(
        const pud_rule_id* leaf, std::vector<pud_query> queries) {
    replace_owned(leaf, std::move(queries));
    for (pud_query* query : ptrs_.at(leaf)) {
        resume_query(*query);
        watch_query(*query);
    }
    mark_dirty(leaf);
}

template<typename IGABG, typename IGL, typename IGBI, typename IACI, typename IDQE,
         typename IRCS, typename IRWS>
void pud_queries<IGABG, IGL, IGBI, IACI, IDQE, IRCS, IRWS>::install(const pud_rule_id* leaf) {
    const uint32_t frame_offset = get_lvc_.get(leaf);
    const om_interval leaf_interval = get_base_interval_.get(leaf);
    std::vector<pud_query> queries;
    for (const expr* goal : get_added_body_goals_.get(leaf)) {
        queries.push_back(pud_query{
            allocate_child_interval_.allocate_child_of(leaf_interval),
            goal,
            root_contexts(),
            frame_offset});
    }
    commit_queries(leaf, std::move(queries));
}

template<typename IGABG, typename IGL, typename IGBI, typename IACI, typename IDQE,
         typename IRCS, typename IRWS>
void pud_queries<IGABG, IGL, IGBI, IACI, IDQE, IRCS, IRWS>::boot() {
    for (const pud_rule_id* axiom : axioms_)
        install(axiom);
}

template<typename IGABG, typename IGL, typename IGBI, typename IACI, typename IDQE,
         typename IRCS, typename IRWS>
void pud_queries<IGABG, IGL, IGBI, IACI, IDQE, IRCS, IRWS>::ensure_booted() {
    if (booted_)
        return;
    booted_ = true;
    boot();
}

template<typename IGABG, typename IGL, typename IGBI, typename IACI, typename IDQE,
         typename IRCS, typename IRWS>
void pud_queries<IGABG, IGL, IGBI, IACI, IDQE, IRCS, IRWS>::adopt_axiom(
        const pud_rule_id* axiom) {
    DEBUG_ASSERT(!booted_);
    axioms_.push_back(axiom);
}

template<typename IGABG, typename IGL, typename IGBI, typename IACI, typename IDQE,
         typename IRCS, typename IRWS>
void pud_queries<IGABG, IGL, IGBI, IACI, IDQE, IRCS, IRWS>::fork_child(
        const pud_rule_id* child,
        const std::vector<pud_query>& leftover_templates) {
    const uint32_t child_lvc = get_lvc_.get(child);
    const om_interval child_interval = get_base_interval_.get(child);
    std::vector<pud_query> child_queries;
    for (const pud_query& leftover : leftover_templates) {
        pud_query forked = leftover;
        forked.interval = allocate_child_interval_.allocate_child_of(child_interval);
        forked.frame_offset = child_lvc;
        child_queries.push_back(std::move(forked));
    }
    for (const expr* goal : get_added_body_goals_.get(child)) {
        child_queries.push_back(pud_query{
            allocate_child_interval_.allocate_child_of(child_interval),
            goal,
            root_contexts(),
            child_lvc});
    }
    commit_queries(child, std::move(child_queries));
}

template<typename IGABG, typename IGL, typename IGBI, typename IACI, typename IDQE,
         typename IRCS, typename IRWS>
void pud_queries<IGABG, IGL, IGBI, IACI, IDQE, IRCS, IRWS>::clear(const pud_rule_id* leaf) {
    auto ptrs_it = ptrs_.find(leaf);
    if (ptrs_it != ptrs_.end()) {
        for (pud_query* query : ptrs_it->second) {
            unwatch_query(query);
            drop_query_env_.drop_query(query);
            query_leaf_.erase(query);
        }
    }
    owned_.erase(leaf);
    ptrs_.erase(leaf);
    dirty_leaves_.erase(leaf);
}

template<typename IGABG, typename IGL, typename IGBI, typename IACI, typename IDQE,
         typename IRCS, typename IRWS>
const std::vector<pud_query*>&
pud_queries<IGABG, IGL, IGBI, IACI, IDQE, IRCS, IRWS>::get(const pud_rule_id* leaf) {
    ensure_booted();
    return ptrs_.at(leaf);
}

template<typename IGABG, typename IGL, typename IGBI, typename IACI, typename IDQE,
         typename IRCS, typename IRWS>
void pud_queries<IGABG, IGL, IGBI, IACI, IDQE, IRCS, IRWS>::resume_dead_witness(
        pud_query& query, const pud_rule_id* node) {
    for (pud_candidate_search_context& axiom_ctx : query.axiom_contexts) {
        if (axiom_ctx.cursor == node && axiom_ctx.live_edges.empty()) {
            resume_candidate_search_.resume(query, axiom_ctx);
            continue;
        }
        bool lost_edge = false;
        for (pud_witness_search_context& edge : axiom_ctx.live_edges) {
            if (edge.current != node)
                continue;
            const pud_witness_search_result result =
                resume_witness_search_.resume(query, edge);
            if (std::holds_alternative<pud_witness_search_result::failed>(result.content))
                lost_edge = true;
        }
        if (!lost_edge)
            continue;
        std::vector<pud_witness_search_context> kept;
        for (const pud_witness_search_context& edge : axiom_ctx.live_edges) {
            if (edge.current != node)
                kept.push_back(edge);
        }
        axiom_ctx.live_edges = kept;
        resume_candidate_search_.resume(query, axiom_ctx);
    }
}

template<typename IGABG, typename IGL, typename IGBI, typename IACI, typename IDQE,
         typename IRCS, typename IRWS>
void pud_queries<IGABG, IGL, IGBI, IACI, IDQE, IRCS, IRWS>::invalidate_leaf(
        const pud_rule_id* node) {
    auto witness_it = by_witness_.find(node);
    if (witness_it == by_witness_.end())
        return;
    const std::vector<pud_query*> queries(witness_it->second.begin(),
                                          witness_it->second.end());
    for (pud_query* query : queries) {
        unwatch_query(query);
        resume_dead_witness(*query, node);
        watch_query(*query);
        auto leaf_it = query_leaf_.find(query);
        if (leaf_it != query_leaf_.end())
            mark_dirty(leaf_it->second);
    }
}

template<typename IGABG, typename IGL, typename IGBI, typename IACI, typename IDQE,
         typename IRCS, typename IRWS>
std::vector<const pud_rule_id*>
pud_queries<IGABG, IGL, IGBI, IACI, IDQE, IRCS, IRWS>::take_dirty_leaves() {
    std::vector<const pud_rule_id*> out(dirty_leaves_.begin(), dirty_leaves_.end());
    std::sort(out.begin(), out.end(), rule_id_less{});
    dirty_leaves_.clear();
    return out;
}

template<typename IGABG, typename IGL, typename IGBI, typename IACI, typename IDQE,
         typename IRCS, typename IRWS>
std::vector<const pud_rule_id*>
pud_queries<IGABG, IGL, IGBI, IACI, IDQE, IRCS, IRWS>::live_cursors(pud_query& query) {
    std::vector<const pud_rule_id*> callees;
    for (pud_candidate_search_context& axiom_ctx : query.axiom_contexts) {
        const pud_candidate_search_result result =
            resume_candidate_search_.resume(query, axiom_ctx);
        if (std::holds_alternative<pud_candidate_search_result::axiom_refuted>(
                result.content))
            continue;
        callees.push_back(axiom_ctx.cursor);
    }
    return callees;
}

template<typename IGABG, typename IGL, typename IGBI, typename IACI, typename IDQE,
         typename IRCS, typename IRWS>
pud_unfold_site
pud_queries<IGABG, IGL, IGBI, IACI, IDQE, IRCS, IRWS>::unfold_site(
        const pud_rule_id* leaf, size_t body_goal_idx) {
    const std::vector<pud_query*>& queries = get(leaf);
    DEBUG_ASSERT(body_goal_idx < queries.size());
    return pud_unfold_site{queries[body_goal_idx], live_cursors(*queries[body_goal_idx])};
}

template<typename IGABG, typename IGL, typename IGBI, typename IACI, typename IDQE,
         typename IRCS, typename IRWS>
std::vector<pud_forced_unfold>
pud_queries<IGABG, IGL, IGBI, IACI, IDQE, IRCS, IRWS>::replace_unfolded(
        const pud_rule_id* leaf,
        size_t body_goal_idx,
        const std::vector<const pud_rule_id*>& children) {
    const std::vector<pud_query*>& parent_queries = get(leaf);
    DEBUG_ASSERT(body_goal_idx < parent_queries.size());
    std::vector<pud_query> leftover_templates;
    for (size_t idx = 0; idx < parent_queries.size(); ++idx) {
        if (idx == body_goal_idx)
            continue;
        leftover_templates.push_back(*parent_queries[idx]);
    }
    clear(leaf);
    for (const pud_rule_id* child : children)
        fork_child(child, leftover_templates);
    invalidate_leaf(leaf);
    return take_forced_unfolds();
}

template<typename IGABG, typename IGL, typename IGBI, typename IACI, typename IDQE,
         typename IRCS, typename IRWS>
std::vector<pud_forced_unfold>
pud_queries<IGABG, IGL, IGBI, IACI, IDQE, IRCS, IRWS>::take_forced_unfolds() {
    ensure_booted();
    std::vector<pud_forced_unfold> yields;
    const std::vector<const pud_rule_id*> dirty = take_dirty_leaves();
    for (const pud_rule_id* leaf : dirty) {
        const std::vector<pud_query*>& queries = get(leaf);
        bool refuted = false;
        std::vector<size_t> unit_idxs;
        for (size_t idx = 0; idx < queries.size(); ++idx) {
            const size_t live = live_cursors(*queries[idx]).size();
            if (live == 0) {
                refuted = true;
                break;
            }
            if (live == 1)
                unit_idxs.push_back(idx);
        }
        if (refuted) {
            yields.push_back(pud_forced_unfold{pud_forced_unfold::refuted{leaf}});
            continue;
        }
        for (size_t idx : unit_idxs)
            yields.push_back(pud_forced_unfold{pud_forced_unfold::unit{leaf, idx}});
    }
    return yields;
}

#endif
