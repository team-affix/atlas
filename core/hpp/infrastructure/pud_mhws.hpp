#ifndef PUD_MHWS_HPP
#define PUD_MHWS_HPP

#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include "debug_assert.hpp"
#include "infrastructure/pud_witness_search_head.hpp"
#include "value_objects/pud_mhws_head_id.hpp"

template<
    typename QueryPosition,
    typename ChildIterator,
    typename ICheckNodeLeaf,
    typename IGetNodeChildren,
    typename IPropagateQueryNodeHandle,
    typename IGetCallSiteIdx>
struct pud_mhws {
    pud_mhws(
        ICheckNodeLeaf& check_node_leaf,
        IGetNodeChildren& get_node_children,
        IPropagateQueryNodeHandle& propagate_query_node_handle,
        IGetCallSiteIdx& get_call_site_idx);
    std::optional<pud_mhws_head_id> try_add_head(QueryPosition search_root_position);
    void remove_head(pud_mhws_head_id head_id);
    std::vector<pud_mhws_head_id> invalidate_leaf(const pud_node* node);
private:
    using head_type = pud_witness_search_head<
        QueryPosition,
        ChildIterator,
        ICheckNodeLeaf,
        IGetNodeChildren,
        IPropagateQueryNodeHandle,
        IGetCallSiteIdx
    >;

    void link(pud_mhws_head_id head_id, const pud_node* witness);
    const pud_node* unlink_head(pud_mhws_head_id head_id);
    std::vector<pud_mhws_head_id> unlink_witness(const pud_node* witness);

    ICheckNodeLeaf& check_node_leaf_;
    IGetNodeChildren& get_node_children_;
    IPropagateQueryNodeHandle& propagate_query_node_handle_;
    IGetCallSiteIdx& get_call_site_idx_;

    pud_mhws_head_id next_head_id_;
    std::unordered_map<pud_mhws_head_id, head_type> heads_;
    std::unordered_map<pud_mhws_head_id, const pud_node*> head_to_witness_;
    std::unordered_map<const pud_node*, std::unordered_set<pud_mhws_head_id>> witness_to_heads_;
};

template<
    typename QP,
    typename CI,
    typename ICNL,
    typename IGNC,
    typename IPQN,
    typename IGCSI>
pud_mhws<QP, CI, ICNL, IGNC, IPQN, IGCSI>::pud_mhws(
    ICNL& check_node_leaf,
    IGNC& get_node_children,
    IPQN& propagate_query_node_handle,
    IGCSI& get_call_site_idx)
    : check_node_leaf_(check_node_leaf)
    , get_node_children_(get_node_children)
    , propagate_query_node_handle_(propagate_query_node_handle)
    , get_call_site_idx_(get_call_site_idx)
    , next_head_id_(0) {}

template<
    typename QP,
    typename CI,
    typename ICNL,
    typename IGNC,
    typename IPQN,
    typename IGCSI>
std::optional<pud_mhws_head_id> pud_mhws<QP, CI, ICNL, IGNC, IPQN, IGCSI>::try_add_head(
    QP search_root_position) {
    auto [head_it, head_inserted] = heads_.emplace(
        next_head_id_,
        head_type{
            check_node_leaf_,
            get_node_children_,
            propagate_query_node_handle_,
            get_call_site_idx_,
            std::move(search_root_position)}).first;

    DEBUG_ASSERT(head_inserted);

    head_type& head = head_it->second;

    const pud_node* witness = head.resume();

    if (witness == nullptr) {
        heads_.erase(head_it);
        return std::nullopt;
    }
    
    head_to_witness_.insert({next_head_id_, witness});
    witness_to_heads_[witness].insert(next_head_id_);
    
    return next_head_id_++;
}

template<
    typename QP,
    typename CI,
    typename ICNL,
    typename IGNC,
    typename IPQN,
    typename IGCSI>
void pud_mhws<QP, CI, ICNL, IGNC, IPQN, IGCSI>::remove_head(pud_mhws_head_id head_id) {

    if (!heads_.contains(head_id))
        return;
    
    const pud_node* witness = head_to_witness_.at(head_id);

    auto& head_ids = witness_to_heads_.at(witness);
    head_ids.erase(head_id);

    if (head_ids.empty())
        witness_to_heads_.erase(witness);

    head_to_witness_.erase(head_id);
    heads_.erase(head_id);
}

template<
    typename QP,
    typename CI,
    typename ICNL,
    typename IGNC,
    typename IPQN,
    typename IGCSI>
std::vector<pud_mhws_head_id> pud_mhws<QP, CI, ICNL, IGNC, IPQN, IGCSI>::invalidate_leaf(const pud_node* node) {
    const auto& head_ids = witness_to_heads_.at(node);

    std::vector<pud_mhws_head_id> to_relink;
    std::vector<pud_mhws_head_id> to_remove;

    for (const auto& head_id : head_ids) {
        auto& head = heads_.at(head_id);
        const pud_node* new_witness = head.resume();
        if (new_witness == nullptr) {
            to_remove.push_back(head_id);
            continue;
        }
        to_relink.push_back(head_id);
    }
    
}

template<
    typename QP,
    typename CI,
    typename ICNL,
    typename IGNC,
    typename IPQN,
    typename IGCSI>
void pud_mhws<QP, CI, ICNL, IGNC, IPQN, IGCSI>::link(pud_mhws_head_id head_id, const pud_node* witness) {
    head_to_witness_.insert({next_head_id_, witness});
    witness_to_heads_[witness].insert(next_head_id_);
}

template<
    typename QP,
    typename CI,
    typename ICNL,
    typename IGNC,
    typename IPQN,
    typename IGCSI>
const pud_node* pud_mhws<QP, CI, ICNL, IGNC, IPQN, IGCSI>::unlink_head(pud_mhws_head_id head_id) {

}

template<
    typename QP,
    typename CI,
    typename ICNL,
    typename IGNC,
    typename IPQN,
    typename IGCSI>
std::vector<pud_mhws_head_id> pud_mhws<QP, CI, ICNL, IGNC, IPQN, IGCSI>::unlink_witness(const pud_node* witness) {

}


#endif
