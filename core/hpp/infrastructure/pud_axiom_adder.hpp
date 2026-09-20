#ifndef PUD_AXIOM_ADDER_HPP
#define PUD_AXIOM_ADDER_HPP

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>
#include "value_objects/om_interval.hpp"
#include "value_objects/pud_candidate_search_context.hpp"
#include "value_objects/pud_db_node.hpp"
#include "value_objects/pud_query.hpp"
#include "value_objects/pud_rule_id.hpp"
#include "value_objects/rule.hpp"

template<typename IAddAxiom,
         typename IGetNode,
         typename IAllocateChildInterval,
         typename IReplaceLeafQueries,
         typename IGetLeafQueries,
         typename IBindQuery,
         typename IReinit,
         typename IResumeCandidateSearch,
         typename IOrderedRoots,
         typename IOrderedLeaves,
         typename IWatch>
struct pud_axiom_adder {
    pud_axiom_adder(IAddAxiom& add_axiom,
                    IGetNode& get_node,
                    IAllocateChildInterval& allocate_child_interval,
                    IReplaceLeafQueries& replace_leaf_queries,
                    IGetLeafQueries& get_leaf_queries,
                    IBindQuery& bind_query,
                    IReinit& reinit,
                    IResumeCandidateSearch& resume_candidate_search,
                    IOrderedRoots& ordered_roots,
                    IOrderedLeaves& ordered_leaves,
                    IWatch& watch);
    const pud_rule_id* add_axiom(const rule& axiom);
private:
    void watch_query(pud_query& query);
    void watch_context(pud_query& query, const pud_candidate_search_context& axiom_ctx);
    void install_axiom_queries(const pud_rule_id* axiom);
    void attach_to_existing_leaves(const pud_rule_id* axiom);

    IAddAxiom& add_axiom_;
    IGetNode& get_node_;
    IAllocateChildInterval& allocate_child_interval_;
    IReplaceLeafQueries& replace_leaf_queries_;
    IGetLeafQueries& get_leaf_queries_;
    IBindQuery& bind_query_;
    IReinit& reinit_;
    IResumeCandidateSearch& resume_candidate_search_;
    IOrderedRoots& ordered_roots_;
    IOrderedLeaves& ordered_leaves_;
    IWatch& watch_;
    uint64_t dummy_open_;
    uint64_t dummy_close_;
    om_interval dummy_interval_;
    size_t next_entry_idx_;
};

template<typename IAA, typename IGN, typename IACI, typename IRLQ, typename IGLQ,
         typename IBQ, typename IR, typename IRCS, typename IOR, typename IOL,
         typename IW>
pud_axiom_adder<IAA, IGN, IACI, IRLQ, IGLQ, IBQ, IR, IRCS, IOR, IOL, IW>::
pud_axiom_adder(IAA& add_axiom,
                IGN& get_node,
                IACI& allocate_child_interval,
                IRLQ& replace_leaf_queries,
                IGLQ& get_leaf_queries,
                IBQ& bind_query,
                IR& reinit,
                IRCS& resume_candidate_search,
                IOR& ordered_roots,
                IOL& ordered_leaves,
                IW& watch)
    : add_axiom_(add_axiom)
    , get_node_(get_node)
    , allocate_child_interval_(allocate_child_interval)
    , replace_leaf_queries_(replace_leaf_queries)
    , get_leaf_queries_(get_leaf_queries)
    , bind_query_(bind_query)
    , reinit_(reinit)
    , resume_candidate_search_(resume_candidate_search)
    , ordered_roots_(ordered_roots)
    , ordered_leaves_(ordered_leaves)
    , watch_(watch)
    , dummy_open_(0)
    , dummy_close_(1)
    , dummy_interval_{om_label(&dummy_open_), om_label(&dummy_close_)}
    , next_entry_idx_(0) {}

template<typename IAA, typename IGN, typename IACI, typename IRLQ, typename IGLQ,
         typename IBQ, typename IR, typename IRCS, typename IOR, typename IOL,
         typename IW>
void pud_axiom_adder<IAA, IGN, IACI, IRLQ, IGLQ, IBQ, IR, IRCS, IOR, IOL, IW>::
watch_context(pud_query& query, const pud_candidate_search_context& axiom_ctx) {
    if (axiom_ctx.live_edges.empty()) {
        watch_.watch(axiom_ctx.cursor, &query);
        return;
    }
    for (const pud_witness_search_context& edge : axiom_ctx.live_edges)
        watch_.watch(edge.current, &query);
}

template<typename IAA, typename IGN, typename IACI, typename IRLQ, typename IGLQ,
         typename IBQ, typename IR, typename IRCS, typename IOR, typename IOL,
         typename IW>
void pud_axiom_adder<IAA, IGN, IACI, IRLQ, IGLQ, IBQ, IR, IRCS, IOR, IOL, IW>::
watch_query(pud_query& query) {
    for (const pud_candidate_search_context& axiom_ctx : query.axiom_contexts)
        watch_context(query, axiom_ctx);
}

template<typename IAA, typename IGN, typename IACI, typename IRLQ, typename IGLQ,
         typename IBQ, typename IR, typename IRCS, typename IOR, typename IOL,
         typename IW>
void pud_axiom_adder<IAA, IGN, IACI, IRLQ, IGLQ, IBQ, IR, IRCS, IOR, IOL, IW>::
install_axiom_queries(const pud_rule_id* axiom) {
    const pud_db_node& node = get_node_.get_node(axiom);
    const uint32_t frame_offset = node.lvc;
    std::vector<pud_query> queries;
    for (const expr* goal : node.added_body_goals) {
        std::vector<pud_candidate_search_context> axiom_contexts;
        for (const pud_rule_id* root : ordered_roots_.ordered_roots())
            axiom_contexts.push_back(pud_candidate_search_context{root, {}});
        queries.push_back(pud_query{
            allocate_child_interval_.allocate_child_of(node.interval),
            goal,
            std::move(axiom_contexts),
            frame_offset});
    }
    replace_leaf_queries_.replace_leaf_queries(axiom, std::move(queries));
    for (pud_query* query : get_leaf_queries_.get(axiom)) {
        bind_query_.bind_query(*query, frame_offset);
        reinit_.reinit(*query, frame_offset);
        for (pud_candidate_search_context& axiom_ctx : query->axiom_contexts)
            resume_candidate_search_.resume(axiom_ctx);
        watch_query(*query);
    }
}

template<typename IAA, typename IGN, typename IACI, typename IRLQ, typename IGLQ,
         typename IBQ, typename IR, typename IRCS, typename IOR, typename IOL,
         typename IW>
void pud_axiom_adder<IAA, IGN, IACI, IRLQ, IGLQ, IBQ, IR, IRCS, IOR, IOL, IW>::
attach_to_existing_leaves(const pud_rule_id* axiom) {
    for (const pud_rule_id* leaf : ordered_leaves_.ordered_leaves()) {
        if (leaf == axiom)
            continue;
        for (pud_query* query : get_leaf_queries_.get(leaf)) {
            query->axiom_contexts.push_back(pud_candidate_search_context{axiom, {}});
            bind_query_.bind_query(*query, query->frame_offset);
            pud_candidate_search_context& new_ctx = query->axiom_contexts.back();
            resume_candidate_search_.resume(new_ctx);
            watch_context(*query, new_ctx);
        }
    }
}

template<typename IAA, typename IGN, typename IACI, typename IRLQ, typename IGLQ,
         typename IBQ, typename IR, typename IRCS, typename IOR, typename IOL,
         typename IW>
const pud_rule_id*
pud_axiom_adder<IAA, IGN, IACI, IRLQ, IGLQ, IBQ, IR, IRCS, IOR, IOL, IW>::
add_axiom(const rule& axiom) {
    pud_db_node node{
        dummy_interval_,
        {{0, axiom.head}},
        axiom.body,
        axiom.var_count};
    const pud_rule_id* id = add_axiom_.add_axiom(next_entry_idx_, std::move(node));
    ++next_entry_idx_;
    install_axiom_queries(id);
    attach_to_existing_leaves(id);
    return id;
}

#endif
