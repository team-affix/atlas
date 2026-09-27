#ifndef PUD_QUERY_PROPAGATOR_HPP
#define PUD_QUERY_PROPAGATOR_HPP

#include <memory>
#include <optional>
#include "value_objects/pud_query_node.hpp"
#include "value_objects/pud_rule_id.hpp"

template<
    typename BindMap,
    typename Unifier,
    typename Specializer,
    typename Normalizer,
    typename IAllocateRootInterval,
    typename IAllocateChildInterval,
    typename IMakeVar,
    typename IGlobalize,
    typename IRecordFPArrayBinding,
    typename IQueryFPArrayBinding>
struct pud_query_propagator {
    struct query_node_handle {
    private:
        std::shared_ptr<pud_query_node> query_node;
        friend struct pud_query_propagator;
    };
    pud_query_propagator(
        IAllocateRootInterval& allocate_root_interval,
        IAllocateChildInterval& allocate_child_interval,
        IMakeVar& make_var,
        IGlobalize& globalize,
        IRecordFPArrayBinding& record_fp,
        IQueryFPArrayBinding& query_fp,
        const pud_node& root);
    query_node_handle root();
    std::optional<query_node_handle> child(query_node_handle current, const pud_rule_id* child_callee);
    query_node_handle open_query(query_node_handle caller, const expr* query);
    pud_node close_query(query_node_handle query);
private:
    IAllocateRootInterval& allocate_root_interval_;
    IAllocateChildInterval& allocate_child_interval_;
    IMakeVar& make_var_;
    IGlobalize& globalize_;
    IRecordFPArrayBinding& record_fp_;
    IQueryFPArrayBinding& query_fp_;

    const pud_node& root_;
};

template<
    typename BM,
    typename U,
    typename S,
    typename N,
    typename IAR,
    typename IAC,
    typename IMV,
    typename IG,
    typename IRFAB,
    typename IQFAB>
pud_query_propagator<BM, U, S, N, IAR, IAC, IMV, IG, IRFAB, IQFAB>::pud_query_propagator(
    IAR& allocate_root_interval,
    IAC& allocate_child_interval,
    IMV& make_var,
    IG& globalize,
    IRFAB& record_fp,
    IQFAB& query_fp,
    const pud_node& root) :
    allocate_root_interval_(allocate_root_interval),
    allocate_child_interval_(allocate_child_interval),
    make_var_(make_var),
    globalize_(globalize),
    record_fp_(record_fp),
    query_fp_(query_fp),
    root_(root) {}

template<
    typename BM,
    typename U,
    typename S,
    typename N,
    typename IAR,
    typename IAC,
    typename IMV,
    typename IG,
    typename IRFAB,
    typename IQFAB>
pud_query_propagator<BM, U, S, N, IAR, IAC, IMV, IG, IRFAB, IQFAB>::query_node_handle pud_query_propagator<BM, U, S, N, IAR, IAC, IMV, IG, IRFAB, IQFAB>::root() {
    return query_node_handle{
        std::make_shared<pud_query_node>(
            pud_query_node{
                0,
                &root_,
                {},
                std::shared_ptr<pud_query_node>{},
                allocate_root_interval_.allocate_root()
                })};
}

template<
    typename BM,
    typename U,
    typename S,
    typename N,
    typename IAR,
    typename IAC,
    typename IMV,
    typename IG,
    typename IRFAB,
    typename IQFAB>
std::optional<typename pud_query_propagator<BM, U, S, N, IAR, IAC, IMV, IG, IRFAB, IQFAB>::query_node_handle> pud_query_propagator<BM, U, S, N, IAR, IAC, IMV, IG, IRFAB, IQFAB>::child(query_node_handle current, const pud_rule_id* child_callee) {
    const pud_node* current_node = current.query_node->node;
    const pud_node* child_node = &current_node->children.at(child_callee);

    om_interval current_interval = current.query_node->interval;
    om_interval child_interval = allocate_child_interval_.allocate_child_of(current_interval);

    BM bind_map{globalize_, record_fp_, query_fp_, child_interval};
    U unifier{globalize_, &bind_map};
    S specializer{make_var_, unifier};

    uint32_t frame_offset = current.query_node->frame_offset;

    for (pud_specialization spec : child_node->added_specializations) {
        if (!specializer.specialize(current.query_node->frame_offset, spec))
            return std::nullopt; // can't traverse to child
    }
    
}

#endif
