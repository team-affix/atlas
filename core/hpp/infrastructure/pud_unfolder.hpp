#ifndef PUD_UNFOLDER_HPP
#define PUD_UNFOLDER_HPP

#include <cstddef>
#include <unordered_map>
#include <variant>
#include <vector>
#include "infrastructure/coroutine.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/om_interval.hpp"
#include "value_objects/pud_candidate_search_context.hpp"
#include "value_objects/pud_candidate_search_result.hpp"
#include "value_objects/pud_db_node.hpp"
#include "value_objects/pud_forced_unfold.hpp"
#include "value_objects/pud_query.hpp"
#include "value_objects/pud_rule_id.hpp"
#include "debug_assert.hpp"

template<typename IGetNode,
         typename IBindQuery,
         typename IUnifyCallee,
         typename INormalize,
         typename IMakeVar,
         typename IAddInference,
         typename ILinkChild,
         typename IAllocateChildInterval,
         typename IGetLeafQueries,
         typename IReplaceLeafQueries,
         typename IClearLeafQueries,
         typename IReinit,
         typename IResumeCandidateSearch,
         typename IOrderedRoots,
         typename IOrderedLeaves,
         typename ITryParent,
         typename IInvalidateLeaf,
         typename IWatch,
         typename IUnwatchQuery>
struct pud_unfolder {
    pud_unfolder(IGetNode& get_node,
                 IBindQuery& bind_query,
                 IUnifyCallee& unify_callee,
                 INormalize& normalize,
                 IMakeVar& make_var,
                 IAddInference& add_inference,
                 ILinkChild& link_child,
                 IAllocateChildInterval& allocate_child_interval,
                 IGetLeafQueries& get_leaf_queries,
                 IReplaceLeafQueries& replace_leaf_queries,
                 IClearLeafQueries& clear_leaf_queries,
                 IReinit& reinit,
                 IResumeCandidateSearch& resume_candidate_search,
                 IOrderedRoots& ordered_roots,
                 IOrderedLeaves& ordered_leaves,
                 ITryParent& try_parent,
                 IInvalidateLeaf& invalidate_leaf,
                 IWatch& watch,
                 IUnwatchQuery& unwatch_query);
    coroutine<pud_forced_unfold, void> unfold(const pud_rule_id* leaf,
                                              size_t body_goal_idx,
                                              const pud_rule_id* callee);
private:
    std::vector<const expr*> effective_body(const pud_rule_id* node);
    void watch_query(pud_query& query);
    void install_queries(const pud_rule_id* leaf, std::vector<pud_query> queries);
    void resume_query(pud_query& query, uint32_t frame_offset);
    size_t live_axiom_count(pud_query& query);
    std::vector<pud_forced_unfold> collect_yields();

    IGetNode& get_node_;
    IBindQuery& bind_query_;
    IUnifyCallee& unify_callee_;
    INormalize& normalize_;
    IMakeVar& make_var_;
    IAddInference& add_inference_;
    ILinkChild& link_child_;
    IAllocateChildInterval& allocate_child_interval_;
    IGetLeafQueries& get_leaf_queries_;
    IReplaceLeafQueries& replace_leaf_queries_;
    IClearLeafQueries& clear_leaf_queries_;
    IReinit& reinit_;
    IResumeCandidateSearch& resume_candidate_search_;
    IOrderedRoots& ordered_roots_;
    IOrderedLeaves& ordered_leaves_;
    ITryParent& try_parent_;
    IInvalidateLeaf& invalidate_leaf_;
    IWatch& watch_;
    IUnwatchQuery& unwatch_query_;
};

template<typename IGN, typename IBQ, typename IUC, typename IN, typename IMV,
         typename IAI, typename ILC, typename IACI, typename IGLQ, typename IRLQ,
         typename ICLQ, typename IR, typename IRCS, typename IOR, typename IOL,
         typename ITP, typename IIL, typename IW, typename IUQ>
pud_unfolder<IGN, IBQ, IUC, IN, IMV, IAI, ILC, IACI, IGLQ, IRLQ, ICLQ, IR, IRCS,
             IOR, IOL, ITP, IIL, IW, IUQ>::pud_unfolder(
        IGN& get_node,
        IBQ& bind_query,
        IUC& unify_callee,
        IN& normalize,
        IMV& make_var,
        IAI& add_inference,
        ILC& link_child,
        IACI& allocate_child_interval,
        IGLQ& get_leaf_queries,
        IRLQ& replace_leaf_queries,
        ICLQ& clear_leaf_queries,
        IR& reinit,
        IRCS& resume_candidate_search,
        IOR& ordered_roots,
        IOL& ordered_leaves,
        ITP& try_parent,
        IIL& invalidate_leaf,
        IW& watch,
        IUQ& unwatch_query)
    : get_node_(get_node)
    , bind_query_(bind_query)
    , unify_callee_(unify_callee)
    , normalize_(normalize)
    , make_var_(make_var)
    , add_inference_(add_inference)
    , link_child_(link_child)
    , allocate_child_interval_(allocate_child_interval)
    , get_leaf_queries_(get_leaf_queries)
    , replace_leaf_queries_(replace_leaf_queries)
    , clear_leaf_queries_(clear_leaf_queries)
    , reinit_(reinit)
    , resume_candidate_search_(resume_candidate_search)
    , ordered_roots_(ordered_roots)
    , ordered_leaves_(ordered_leaves)
    , try_parent_(try_parent)
    , invalidate_leaf_(invalidate_leaf)
    , watch_(watch)
    , unwatch_query_(unwatch_query) {}

template<typename IGN, typename IBQ, typename IUC, typename IN, typename IMV,
         typename IAI, typename ILC, typename IACI, typename IGLQ, typename IRLQ,
         typename ICLQ, typename IR, typename IRCS, typename IOR, typename IOL,
         typename ITP, typename IIL, typename IW, typename IUQ>
std::vector<const expr*>
pud_unfolder<IGN, IBQ, IUC, IN, IMV, IAI, ILC, IACI, IGLQ, IRLQ, ICLQ, IR, IRCS,
             IOR, IOL, ITP, IIL, IW, IUQ>::effective_body(const pud_rule_id* node) {
    std::vector<const pud_rule_id*> path;
    const pud_rule_id* walk = node;
    while (walk != nullptr) {
        path.push_back(walk);
        walk = try_parent_.try_parent(walk);
    }
    std::vector<const expr*> body;
    for (size_t idx = path.size(); idx > 0; --idx) {
        const pud_rule_id* step = path[idx - 1];
        const bool has_parent = (idx != path.size());
        if (has_parent) {
            const pud_rule_id::inference& inf = std::get<pud_rule_id::inference>(step->content);
            DEBUG_ASSERT(inf.call_site < body.size());
            body.erase(body.begin() + static_cast<std::ptrdiff_t>(inf.call_site));
        }
        const std::vector<const expr*>& added = get_node_.get_node(step).added_body_goals;
        body.insert(body.end(), added.begin(), added.end());
    }
    return body;
}

template<typename IGN, typename IBQ, typename IUC, typename IN, typename IMV,
         typename IAI, typename ILC, typename IACI, typename IGLQ, typename IRLQ,
         typename ICLQ, typename IR, typename IRCS, typename IOR, typename IOL,
         typename ITP, typename IIL, typename IW, typename IUQ>
void pud_unfolder<IGN, IBQ, IUC, IN, IMV, IAI, ILC, IACI, IGLQ, IRLQ, ICLQ, IR, IRCS,
                  IOR, IOL, ITP, IIL, IW, IUQ>::watch_query(pud_query& query) {
    for (const pud_candidate_search_context& axiom_ctx : query.axiom_contexts) {
        if (axiom_ctx.live_edges.empty()) {
            watch_.watch(axiom_ctx.cursor, &query);
            continue;
        }
        for (const pud_witness_search_context& edge : axiom_ctx.live_edges)
            watch_.watch(edge.current, &query);
    }
}

template<typename IGN, typename IBQ, typename IUC, typename IN, typename IMV,
         typename IAI, typename ILC, typename IACI, typename IGLQ, typename IRLQ,
         typename ICLQ, typename IR, typename IRCS, typename IOR, typename IOL,
         typename ITP, typename IIL, typename IW, typename IUQ>
void pud_unfolder<IGN, IBQ, IUC, IN, IMV, IAI, ILC, IACI, IGLQ, IRLQ, ICLQ, IR, IRCS,
                  IOR, IOL, ITP, IIL, IW, IUQ>::install_queries(
        const pud_rule_id* leaf, std::vector<pud_query> queries) {
    replace_leaf_queries_.replace_leaf_queries(leaf, std::move(queries));
}

template<typename IGN, typename IBQ, typename IUC, typename IN, typename IMV,
         typename IAI, typename ILC, typename IACI, typename IGLQ, typename IRLQ,
         typename ICLQ, typename IR, typename IRCS, typename IOR, typename IOL,
         typename ITP, typename IIL, typename IW, typename IUQ>
void pud_unfolder<IGN, IBQ, IUC, IN, IMV, IAI, ILC, IACI, IGLQ, IRLQ, ICLQ, IR, IRCS,
                  IOR, IOL, ITP, IIL, IW, IUQ>::resume_query(
        pud_query& query, uint32_t frame_offset) {
    bind_query_.bind_query(query, frame_offset);
    reinit_.reinit(query, frame_offset);
    for (pud_candidate_search_context& axiom_ctx : query.axiom_contexts)
        resume_candidate_search_.resume(axiom_ctx);
}

template<typename IGN, typename IBQ, typename IUC, typename IN, typename IMV,
         typename IAI, typename ILC, typename IACI, typename IGLQ, typename IRLQ,
         typename ICLQ, typename IR, typename IRCS, typename IOR, typename IOL,
         typename ITP, typename IIL, typename IW, typename IUQ>
size_t pud_unfolder<IGN, IBQ, IUC, IN, IMV, IAI, ILC, IACI, IGLQ, IRLQ, ICLQ, IR, IRCS,
                    IOR, IOL, ITP, IIL, IW, IUQ>::live_axiom_count(pud_query& query) {
    size_t live = 0;
    for (pud_candidate_search_context& axiom_ctx : query.axiom_contexts) {
        const pud_candidate_search_result result = resume_candidate_search_.resume(axiom_ctx);
        if (!std::holds_alternative<pud_candidate_search_result::axiom_refuted>(result.content))
            ++live;
    }
    return live;
}

template<typename IGN, typename IBQ, typename IUC, typename IN, typename IMV,
         typename IAI, typename ILC, typename IACI, typename IGLQ, typename IRLQ,
         typename ICLQ, typename IR, typename IRCS, typename IOR, typename IOL,
         typename ITP, typename IIL, typename IW, typename IUQ>
std::vector<pud_forced_unfold>
pud_unfolder<IGN, IBQ, IUC, IN, IMV, IAI, ILC, IACI, IGLQ, IRLQ, ICLQ, IR, IRCS,
             IOR, IOL, ITP, IIL, IW, IUQ>::collect_yields() {
    std::vector<pud_forced_unfold> yields;
    const std::vector<const pud_rule_id*> leaves = ordered_leaves_.ordered_leaves();
    for (const pud_rule_id* leaf : leaves) {
        const std::vector<pud_query*>& queries = get_leaf_queries_.get(leaf);
        const uint32_t frame_offset = get_node_.get_node(leaf).lvc;
        bool refuted = false;
        std::vector<size_t> unit_idxs;
        for (size_t idx = 0; idx < queries.size(); ++idx) {
            bind_query_.bind_query(*queries[idx], frame_offset);
            const size_t live = live_axiom_count(*queries[idx]);
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

template<typename IGN, typename IBQ, typename IUC, typename IN, typename IMV,
         typename IAI, typename ILC, typename IACI, typename IGLQ, typename IRLQ,
         typename ICLQ, typename IR, typename IRCS, typename IOR, typename IOL,
         typename ITP, typename IIL, typename IW, typename IUQ>
coroutine<pud_forced_unfold, void>
pud_unfolder<IGN, IBQ, IUC, IN, IMV, IAI, ILC, IACI, IGLQ, IRLQ, ICLQ, IR, IRCS,
             IOR, IOL, ITP, IIL, IW, IUQ>::unfold(const pud_rule_id* leaf,
                                                 size_t body_goal_idx,
                                                 const pud_rule_id* callee) {
    const pud_db_node& parent_node = get_node_.get_node(leaf);
    const uint32_t parent_lvc = parent_node.lvc;
    const om_interval parent_interval = parent_node.interval;
    const std::vector<pud_query*>& parent_queries = get_leaf_queries_.get(leaf);
    DEBUG_ASSERT(body_goal_idx < parent_queries.size());
    pud_query* parent_query = parent_queries[body_goal_idx];
    bind_query_.bind_query(*parent_query, parent_lvc);

    std::vector<uint32_t> touched_reps;
    const bool unified = unify_callee_.unify_callee(callee, touched_reps);
    DEBUG_ASSERT(unified);

    std::unordered_map<uint32_t, uint32_t> translation;
    std::vector<pud_added_unification> added_unifications;
    for (uint32_t rep : touched_reps) {
        const expr* value = normalize_.normalize(
            framed_expr{make_var_.make_var(rep), 0}, parent_lvc, translation);
        added_unifications.push_back(pud_added_unification{rep, value});
    }

    const std::vector<const expr*> callee_bodies = effective_body(callee);
    std::vector<const expr*> added_body_goals;
    for (const expr* goal : callee_bodies) {
        added_body_goals.push_back(
            normalize_.normalize(framed_expr{goal, 0}, parent_lvc, translation));
    }
    const uint32_t child_lvc = parent_lvc
        + static_cast<uint32_t>(translation.size());

    pud_db_node child_node{
        parent_interval,
        std::move(added_unifications),
        std::move(added_body_goals),
        child_lvc};
    const pud_rule_id* child = add_inference_.add_inference(
        leaf, body_goal_idx, callee, child_node);
    link_child_.link_child(leaf, child);

    std::vector<pud_query> child_queries;
    for (size_t idx = 0; idx < parent_queries.size(); ++idx) {
        if (idx == body_goal_idx)
            continue;
        pud_query leftover = *parent_queries[idx];
        leftover.interval = allocate_child_interval_.allocate_child_of(
            get_node_.get_node(child).interval);
        leftover.frame_offset = child_lvc;
        child_queries.push_back(std::move(leftover));
    }
    const pud_db_node& stored_child = get_node_.get_node(child);
    for (const expr* goal : stored_child.added_body_goals) {
        std::vector<pud_candidate_search_context> axiom_contexts;
        for (const pud_rule_id* root : ordered_roots_.ordered_roots())
            axiom_contexts.push_back(pud_candidate_search_context{root, {}});
        child_queries.push_back(pud_query{
            allocate_child_interval_.allocate_child_of(stored_child.interval),
            goal,
            std::move(axiom_contexts),
            child_lvc});
    }

    for (pud_query* query : parent_queries)
        unwatch_query_.unwatch_query(query);
    clear_leaf_queries_.clear_leaf_queries(leaf);

    install_queries(child, std::move(child_queries));
    for (pud_query* query : get_leaf_queries_.get(child)) {
        resume_query(*query, child_lvc);
        watch_query(*query);
    }

    invalidate_leaf_.invalidate_leaf(leaf);

    const std::vector<const pud_rule_id*> leaves = ordered_leaves_.ordered_leaves();
    for (const pud_rule_id* remaining : leaves) {
        const std::vector<pud_query*>& queries = get_leaf_queries_.get(remaining);
        for (pud_query* query : queries) {
            unwatch_query_.unwatch_query(query);
            watch_query(*query);
        }
    }

    const std::vector<pud_forced_unfold> yields = collect_yields();
    for (const pud_forced_unfold& yield : yields)
        co_yield yield;
}

#endif
