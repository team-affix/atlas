#ifndef PUD_QUERIES_HPP
#define PUD_QUERIES_HPP

#include <algorithm>
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
#include "value_objects/pud_db_node.hpp"
#include "value_objects/pud_query.hpp"
#include "value_objects/pud_rule_id.hpp"
#include "value_objects/pud_witness_search_context.hpp"
#include "value_objects/pud_witness_search_result.hpp"
#include "debug_assert.hpp"

template<typename IGetNode,
         typename IAllocateChildInterval,
         typename IOrderedRoots,
         typename IOrderedLeaves,
         typename IReinit,
         typename IResumeCandidateSearch,
         typename IResumeWitnessSearch>
struct pud_queries {
    pud_queries(IGetNode& get_node,
                IAllocateChildInterval& allocate_child_interval,
                IOrderedRoots& ordered_roots,
                IOrderedLeaves& ordered_leaves,
                IReinit& reinit,
                IResumeCandidateSearch& resume_candidate_search,
                IResumeWitnessSearch& resume_witness_search);
    void install(const pud_rule_id* leaf);
    void attach_axiom(const pud_rule_id* axiom);
    void fork_child(const pud_rule_id* child,
                    const std::vector<pud_query>& leftover_templates);
    void clear(const pud_rule_id* leaf);
    const std::vector<pud_query*>& get(const pud_rule_id* leaf) const;
    void invalidate_leaf(const pud_rule_id* node);
    std::vector<const pud_rule_id*> take_dirty_leaves();
private:
    using owned_t = std::vector<std::unique_ptr<pud_query>>;
    using ptrs_t = std::vector<pud_query*>;
    using query_set_t = std::unordered_set<pud_query*>;
    using witness_set_t = std::unordered_set<const pud_rule_id*>;
    using leaf_set_t = std::unordered_set<const pud_rule_id*>;
    struct rule_id_less {
        bool operator()(const pud_rule_id* a, const pud_rule_id* b) const;
    };

    void replace_owned(const pud_rule_id* leaf, std::vector<pud_query> queries);
    void resume_query(pud_query& query);
    void watch_query(pud_query& query);
    void watch_context(pud_query& query, const pud_candidate_search_context& axiom_ctx);
    void unwatch_query(pud_query* query);
    void watch(const pud_rule_id* witness, pud_query* query);
    void mark_dirty(const pud_rule_id* leaf);
    void resume_dead_witness(pud_query& query, const pud_rule_id* node);
    std::vector<pud_candidate_search_context> root_contexts();

    IGetNode& get_node_;
    IAllocateChildInterval& allocate_child_interval_;
    IOrderedRoots& ordered_roots_;
    IOrderedLeaves& ordered_leaves_;
    IReinit& reinit_;
    IResumeCandidateSearch& resume_candidate_search_;
    IResumeWitnessSearch& resume_witness_search_;
    std::unordered_map<const pud_rule_id*, owned_t> owned_;
    std::unordered_map<const pud_rule_id*, ptrs_t> ptrs_;
    std::unordered_map<pud_query*, const pud_rule_id*> query_leaf_;
    std::unordered_map<const pud_rule_id*, query_set_t> by_witness_;
    std::unordered_map<pud_query*, witness_set_t> by_query_;
    leaf_set_t dirty_leaves_;
};

template<typename IGN, typename IACI, typename IOR, typename IOL, typename IR,
         typename IRCS, typename IRWS>
bool pud_queries<IGN, IACI, IOR, IOL, IR, IRCS, IRWS>::rule_id_less::operator()(
        const pud_rule_id* a, const pud_rule_id* b) const {
    return *a < *b;
}

template<typename IGN, typename IACI, typename IOR, typename IOL, typename IR,
         typename IRCS, typename IRWS>
pud_queries<IGN, IACI, IOR, IOL, IR, IRCS, IRWS>::pud_queries(
        IGN& get_node,
        IACI& allocate_child_interval,
        IOR& ordered_roots,
        IOL& ordered_leaves,
        IR& reinit,
        IRCS& resume_candidate_search,
        IRWS& resume_witness_search)
    : get_node_(get_node)
    , allocate_child_interval_(allocate_child_interval)
    , ordered_roots_(ordered_roots)
    , ordered_leaves_(ordered_leaves)
    , reinit_(reinit)
    , resume_candidate_search_(resume_candidate_search)
    , resume_witness_search_(resume_witness_search)
    , owned_()
    , ptrs_()
    , query_leaf_()
    , by_witness_()
    , by_query_()
    , dirty_leaves_() {}

template<typename IGN, typename IACI, typename IOR, typename IOL, typename IR,
         typename IRCS, typename IRWS>
std::vector<pud_candidate_search_context>
pud_queries<IGN, IACI, IOR, IOL, IR, IRCS, IRWS>::root_contexts() {
    std::vector<pud_candidate_search_context> axiom_contexts;
    for (const pud_rule_id* root : ordered_roots_.ordered_roots())
        axiom_contexts.push_back(pud_candidate_search_context{root, {}});
    return axiom_contexts;
}

template<typename IGN, typename IACI, typename IOR, typename IOL, typename IR,
         typename IRCS, typename IRWS>
void pud_queries<IGN, IACI, IOR, IOL, IR, IRCS, IRWS>::replace_owned(
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

template<typename IGN, typename IACI, typename IOR, typename IOL, typename IR,
         typename IRCS, typename IRWS>
void pud_queries<IGN, IACI, IOR, IOL, IR, IRCS, IRWS>::watch(
        const pud_rule_id* witness, pud_query* query) {
    by_witness_[witness].insert(query);
    by_query_[query].insert(witness);
}

template<typename IGN, typename IACI, typename IOR, typename IOL, typename IR,
         typename IRCS, typename IRWS>
void pud_queries<IGN, IACI, IOR, IOL, IR, IRCS, IRWS>::watch_context(
        pud_query& query, const pud_candidate_search_context& axiom_ctx) {
    if (axiom_ctx.live_edges.empty()) {
        watch(axiom_ctx.cursor, &query);
        return;
    }
    for (const pud_witness_search_context& edge : axiom_ctx.live_edges)
        watch(edge.current, &query);
}

template<typename IGN, typename IACI, typename IOR, typename IOL, typename IR,
         typename IRCS, typename IRWS>
void pud_queries<IGN, IACI, IOR, IOL, IR, IRCS, IRWS>::watch_query(pud_query& query) {
    for (const pud_candidate_search_context& axiom_ctx : query.axiom_contexts)
        watch_context(query, axiom_ctx);
}

template<typename IGN, typename IACI, typename IOR, typename IOL, typename IR,
         typename IRCS, typename IRWS>
void pud_queries<IGN, IACI, IOR, IOL, IR, IRCS, IRWS>::unwatch_query(pud_query* query) {
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

template<typename IGN, typename IACI, typename IOR, typename IOL, typename IR,
         typename IRCS, typename IRWS>
void pud_queries<IGN, IACI, IOR, IOL, IR, IRCS, IRWS>::resume_query(pud_query& query) {
    reinit_.reinit(query);
    for (pud_candidate_search_context& axiom_ctx : query.axiom_contexts)
        resume_candidate_search_.resume(query, axiom_ctx);
}

template<typename IGN, typename IACI, typename IOR, typename IOL, typename IR,
         typename IRCS, typename IRWS>
void pud_queries<IGN, IACI, IOR, IOL, IR, IRCS, IRWS>::mark_dirty(const pud_rule_id* leaf) {
    dirty_leaves_.insert(leaf);
}

template<typename IGN, typename IACI, typename IOR, typename IOL, typename IR,
         typename IRCS, typename IRWS>
void pud_queries<IGN, IACI, IOR, IOL, IR, IRCS, IRWS>::install(const pud_rule_id* leaf) {
    const pud_db_node& node = get_node_.get_node(leaf);
    const uint32_t frame_offset = node.lvc;
    std::vector<pud_query> queries;
    for (const expr* goal : node.added_body_goals) {
        queries.push_back(pud_query{
            allocate_child_interval_.allocate_child_of(node.interval),
            goal,
            root_contexts(),
            frame_offset});
    }
    replace_owned(leaf, std::move(queries));
    for (pud_query* query : ptrs_.at(leaf)) {
        resume_query(*query);
        watch_query(*query);
    }
    mark_dirty(leaf);
}

template<typename IGN, typename IACI, typename IOR, typename IOL, typename IR,
         typename IRCS, typename IRWS>
void pud_queries<IGN, IACI, IOR, IOL, IR, IRCS, IRWS>::attach_axiom(
        const pud_rule_id* axiom) {
    for (const pud_rule_id* leaf : ordered_leaves_.ordered_leaves()) {
        if (leaf == axiom)
            continue;
        auto ptrs_it = ptrs_.find(leaf);
        if (ptrs_it == ptrs_.end())
            continue;
        for (pud_query* query : ptrs_it->second) {
            query->axiom_contexts.push_back(pud_candidate_search_context{axiom, {}});
            pud_candidate_search_context& new_ctx = query->axiom_contexts.back();
            resume_candidate_search_.resume(*query, new_ctx);
            watch_context(*query, new_ctx);
        }
        mark_dirty(leaf);
    }
}

template<typename IGN, typename IACI, typename IOR, typename IOL, typename IR,
         typename IRCS, typename IRWS>
void pud_queries<IGN, IACI, IOR, IOL, IR, IRCS, IRWS>::fork_child(
        const pud_rule_id* child,
        const std::vector<pud_query>& leftover_templates) {
    const pud_db_node& stored_child = get_node_.get_node(child);
    const uint32_t child_lvc = stored_child.lvc;
    std::vector<pud_query> child_queries;
    for (const pud_query& leftover : leftover_templates) {
        pud_query forked = leftover;
        forked.interval = allocate_child_interval_.allocate_child_of(stored_child.interval);
        forked.frame_offset = child_lvc;
        child_queries.push_back(std::move(forked));
    }
    for (const expr* goal : stored_child.added_body_goals) {
        child_queries.push_back(pud_query{
            allocate_child_interval_.allocate_child_of(stored_child.interval),
            goal,
            root_contexts(),
            child_lvc});
    }
    replace_owned(child, std::move(child_queries));
    for (pud_query* query : ptrs_.at(child)) {
        resume_query(*query);
        watch_query(*query);
    }
    mark_dirty(child);
}

template<typename IGN, typename IACI, typename IOR, typename IOL, typename IR,
         typename IRCS, typename IRWS>
void pud_queries<IGN, IACI, IOR, IOL, IR, IRCS, IRWS>::clear(const pud_rule_id* leaf) {
    auto ptrs_it = ptrs_.find(leaf);
    if (ptrs_it != ptrs_.end()) {
        for (pud_query* query : ptrs_it->second) {
            unwatch_query(query);
            query_leaf_.erase(query);
        }
    }
    owned_.erase(leaf);
    ptrs_.erase(leaf);
    dirty_leaves_.erase(leaf);
}

template<typename IGN, typename IACI, typename IOR, typename IOL, typename IR,
         typename IRCS, typename IRWS>
const std::vector<pud_query*>&
pud_queries<IGN, IACI, IOR, IOL, IR, IRCS, IRWS>::get(const pud_rule_id* leaf) const {
    return ptrs_.at(leaf);
}

template<typename IGN, typename IACI, typename IOR, typename IOL, typename IR,
         typename IRCS, typename IRWS>
void pud_queries<IGN, IACI, IOR, IOL, IR, IRCS, IRWS>::resume_dead_witness(
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

template<typename IGN, typename IACI, typename IOR, typename IOL, typename IR,
         typename IRCS, typename IRWS>
void pud_queries<IGN, IACI, IOR, IOL, IR, IRCS, IRWS>::invalidate_leaf(
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

template<typename IGN, typename IACI, typename IOR, typename IOL, typename IR,
         typename IRCS, typename IRWS>
std::vector<const pud_rule_id*>
pud_queries<IGN, IACI, IOR, IOL, IR, IRCS, IRWS>::take_dirty_leaves() {
    std::vector<const pud_rule_id*> out(dirty_leaves_.begin(), dirty_leaves_.end());
    std::sort(out.begin(), out.end(), rule_id_less{});
    dirty_leaves_.clear();
    return out;
}

#endif
