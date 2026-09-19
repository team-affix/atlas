#ifndef PUD_INVALIDATION_ROUTER_HPP
#define PUD_INVALIDATION_ROUTER_HPP

#include <unordered_set>
#include <variant>
#include <vector>
#include "value_objects/pud_candidate_search_result.hpp"
#include "value_objects/pud_query.hpp"
#include "value_objects/pud_rule_id.hpp"
#include "value_objects/pud_witness_search_result.hpp"

template<typename IGetWatchers,
         typename IResumeWitnessSearch,
         typename IResumeCandidateSearch,
         typename IBindQuery>
struct pud_invalidation_router {
    pud_invalidation_router(IGetWatchers& get_watchers,
                            IResumeWitnessSearch& resume_witness_search,
                            IResumeCandidateSearch& resume_candidate_search,
                            IBindQuery& bind_query);
    void invalidate_leaf(const pud_rule_id* node);
private:
    void resume_query_for_dead_witness(pud_query& query, const pud_rule_id* node);

    IGetWatchers& get_watchers_;
    IResumeWitnessSearch& resume_witness_search_;
    IResumeCandidateSearch& resume_candidate_search_;
    IBindQuery& bind_query_;
};

template<typename IGW, typename IRWS, typename IRCS, typename IBQ>
pud_invalidation_router<IGW, IRWS, IRCS, IBQ>::pud_invalidation_router(
        IGW& get_watchers,
        IRWS& resume_witness_search,
        IRCS& resume_candidate_search,
        IBQ& bind_query)
    : get_watchers_(get_watchers)
    , resume_witness_search_(resume_witness_search)
    , resume_candidate_search_(resume_candidate_search)
    , bind_query_(bind_query) {}

template<typename IGW, typename IRWS, typename IRCS, typename IBQ>
void pud_invalidation_router<IGW, IRWS, IRCS, IBQ>::resume_query_for_dead_witness(
        pud_query& query, const pud_rule_id* node) {
    bind_query_.bind_query(query, query.frame_offset);
    for (pud_candidate_search_context& axiom_ctx : query.axiom_contexts) {
        if (axiom_ctx.cursor == node && axiom_ctx.live_edges.empty()) {
            resume_candidate_search_.resume(axiom_ctx);
            continue;
        }
        bool lost_edge = false;
        for (pud_witness_search_context& edge : axiom_ctx.live_edges) {
            if (edge.current != node)
                continue;
            const pud_witness_search_result result = resume_witness_search_.resume(edge);
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
        resume_candidate_search_.resume(axiom_ctx);
    }
}

template<typename IGW, typename IRWS, typename IRCS, typename IBQ>
void pud_invalidation_router<IGW, IRWS, IRCS, IBQ>::invalidate_leaf(const pud_rule_id* node) {
    const std::unordered_set<pud_query*>& watchers = get_watchers_.get(node);
    const std::vector<pud_query*> queries(watchers.begin(), watchers.end());
    for (pud_query* query : queries)
        resume_query_for_dead_witness(*query, node);
}

#endif
