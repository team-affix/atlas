#ifndef PUD_CANDIDATE_SPECIALIZATION_HEAD_HPP
#define PUD_CANDIDATE_SPECIALIZATION_HEAD_HPP

#include <optional>
#include <vector>
#include "value_objects/pud_candidate_resume_context.hpp"
#include "value_objects/pud_mhws_head_id.hpp"
#include "value_objects/pud_candidate_choice_point.hpp"
#include "value_objects/pud_candidate_self_witness.hpp"
#include "value_objects/pud_witness_advance_result.hpp"

template<
    typename QueryHandle,
    typename NodeIterator,
    typename ITryAddHead,
    typename IAdvanceWitnessSearchHead,
    typename IForkWitnessSearchHead,
    typename ICheckNodeLeaf,
    typename IGetChildren,
    typename IDescend>
struct pud_candidate_specialization_head {
    pud_candidate_specialization_head(
        ITryAddHead& try_add_head,
        IAdvanceWitnessSearchHead& advance_witness_search_head,
        IForkWitnessSearchHead& fork_witness_search_head,
        ICheckNodeLeaf& check_node_leaf,
        IGetChildren& get_children,
        IDescend& descend,
        QueryHandle specialization_root_handle);
    pud_candidate_specialization_head(
        const pud_candidate_specialization_head& other,
        QueryHandle specialization_root_handle);
    std::optional<pud_candidate_resume_context<QueryHandle>> resume();
    void witness_refuted(pud_mhws_head_id witness_id);
private:
    struct witness {
        pud_mhws_head_id id;
        QueryHandle handle;
    };
    struct witness_scan {
        std::optional<witness> witness_a;
        std::optional<witness> witness_b;
        NodeIterator next_witness_root_it;
        NodeIterator end_witness_root_it;
    };

    std::optional<witness> try_replace_witness();
    void advance(witness& survivor);

    ITryAddHead& try_add_head_;
    IAdvanceWitnessSearchHead& advance_witness_search_head_;
    IForkWitnessSearchHead& fork_witness_search_head_;
    ICheckNodeLeaf& check_node_leaf_;
    IGetChildren& get_children_;
    IDescend& descend_; // was propagate_query_handle_

    std::vector<pud_node_id> node_path_;
    QueryHandle                 current_handle_;
    std::optional<witness_scan> witness_scan_;
    bool node_path_truncated_;
};

template<typename QH, typename NI, typename ITAH, typename IAWSH, typename IFWSH, typename ICNL, typename IGC, typename IDESC>
pud_candidate_specialization_head<QH, NI, ITAH, IAWSH, IFWSH, ICNL, IGC, IDESC>::pud_candidate_specialization_head(
    ITAH& try_add_head,
    IAWSH& advance_witness_search_head,
    IFWSH& fork_witness_search_head,
    ICNL& check_node_leaf,
    IGC& get_children,
    IDESC& descend,
    QH specialization_root_handle) :
    try_add_head_(try_add_head),
    advance_witness_search_head_(advance_witness_search_head),
    fork_witness_search_head_(fork_witness_search_head),
    check_node_leaf_(check_node_leaf),
    get_children_(get_children),
    descend_(descend),
    current_handle_(specialization_root_handle),
    node_path_truncated_(false) {
}

template<typename QH, typename NI, typename ITAH, typename IAWSH, typename IFWSH, typename ICNL, typename IGC, typename IDESC>
pud_candidate_specialization_head<QH, NI, ITAH, IAWSH, IFWSH, ICNL, IGC, IDESC>::pud_candidate_specialization_head(
    const pud_candidate_specialization_head& other,
    QH specialization_root_handle) :
    try_add_head_(other.try_add_head_),
    advance_witness_search_head_(other.advance_witness_search_head_),
    fork_witness_search_head_(other.fork_witness_search_head_),
    check_node_leaf_(other.check_node_leaf_),
    get_children_(other.get_children_),
    descend_(other.descend_),
    node_path_truncated_(false) {

    QH current_query_handle = specialization_root_handle;
    
    // walk our frame stack using the new query handle
    for (const auto& node : other.node_path_) {
        auto optional_new_query_handle = descend_.descend(current_query_handle, node);

        if (!optional_new_query_handle.has_value())
            break;

        node_path_.push_back(node);

        current_query_handle = optional_new_query_handle.value();
    }

    if (node_path_.size() != other.node_path_.size()) {
        // we hit a conflict
        node_path_truncated_ = true;
        return;
    }

    // if the other head was a self-witness, we are done
    if (!other.witness_scan_.has_value())
        return;

    // we are a choice-point
    
    const auto& other_witness_search_a = other.witness_scan_->witness_a;
    const auto& other_witness_search_b = other.witness_scan_->witness_b;
    
    std::optional<pud_mhws_head_id> forked_witness_a;
    std::optional<pud_mhws_head_id> forked_witness_b;

    if (other_witness_search_a.has_value()) {
        const witness& wn = other_witness_search_a.value();
        
        // step in direction of witness A
        auto optional_search_root = descend_.descend(current_query_handle, wn.handle.node);

        if (optional_search_root.has_value())
            forked_witness_a = fork_witness_search_head_.try_fork_head(wn.id, optional_search_root.value());
    }
    if (other_witness_search_b.has_value()) {
        const witness& wn = other_witness_search_b.value();
        
        // step in direction of witness B
        auto optional_search_root = descend_.descend(current_query_handle, wn.handle.node);

        if (optional_search_root.has_value())
            forked_witness_b = fork_witness_search_head_.try_fork_head(wn.id, optional_search_root.value());
    }
    
    auto other_next_witness_root = other.witness_scan_->next_witness_root_it;
    auto other_end_witness_root = other.witness_scan_->end_witness_root_it;
        
    witness_scan_ = {
        .witness_search_a_ = forked_witness_a,
        .witness_search_b_ = forked_witness_b,
        .next_witness_root_ = other_next_witness_root,
        .end_witness_root_ = other_end_witness_root,
    };
}

template<typename QH, typename NI, typename ITAH, typename IAWSH, typename IFWSH, typename ICNL, typename IGC, typename IDESC>
std::optional<pud_candidate_resume_context<QH>> pud_candidate_specialization_head<QH, NI, ITAH, IAWSH, IFWSH, ICNL, IGC, IDESC>::resume() {
    // there are three possible starting states:
    // 1. no witnesses found yet
    // 2. self-witness situation
    // 3. choice-point situation

    // when advancing, whether we find ourselves as a self-witness or not
    //     is unknown until we do some witness search.

    // check if we are at a leaf node
    
    
    // if we are currently a choice-point, ~~try to resume both witness searches.~~
    //     EDIT: we don't resume them manually. this is because, everything is reactive.
    //           If a witness search is refuted, we get notified and update accordingly.
    //           If we need to find a replacement, we do so and eagerly determine whether
    //           it was successful. The invariant we are using is: if a witness search is not
    //           nullopt, then it is valid and pointing to a valid witness.
    //     any witness search which fails to find a solution is marked as such and
    //     we resume from next_witness_root_ searching for replacements.
    
    // we always try to find two witnesses eagerly. Each witness is found down a
    //     unique outgoing edge from the current node.
    
    // if we cannot find two witnesses, we advance toward the surviving witness search

    if (node_path_truncated_)
        return std::nullopt;
    
    while (true) {

        const pud_node_id current_node = current_handle_.node();
        
        // if we are already at a leaf node, we are done. we are a self-witness
        if (check_node_leaf_.check_leaf(current_node)) {
            witness_scan_ = std::nullopt;
            return pud_candidate_resume_context<QH>{
                .justification = pud_candidate_self_witness{
                    .node = current_node,
                },
                .query_handle = current_handle_,
            };
        }
    
        // from this point on, we are not a self-witness
    
        if (!witness_scan_.has_value()) {
            const auto& children = get_children_.get(current_node);
            witness_scan_ = {
                .witness_a = std::nullopt,
                .witness_b = std::nullopt,
                .next_witness_root_it = children.begin(),
                .end_witness_root_it = children.end(),
            };
        }
    
        auto& witness_a = witness_scan_->witness_a;
        auto& witness_b = witness_scan_->witness_b;
    
        // try to find replacement witnesses if missing
        if (!witness_a.has_value())
            witness_a = try_replace_witness();
        if (!witness_b.has_value())
            witness_b = try_replace_witness();
    
        // if both witnesses exist, we are done (valid choice point)
        if (witness_a.has_value() && witness_b.has_value())
            return pud_candidate_resume_context<QH>{
                .justification = pud_candidate_choice_point{
                    .witness_a = witness_a.value().id,
                    .witness_b = witness_b.value().id,
                },
                .query_handle = current_handle_,
            };
        
        // if both witnesses are missing, we are refuted
        if (!witness_a.has_value() && !witness_b.has_value())
            return std::nullopt;
    
        // if only A exists, we advance toward it
        if (witness_a.has_value() && !witness_b.has_value())
            advance(witness_a.value());
    
        // if only B exists, we advance toward it
        if (!witness_a.has_value() && witness_b.has_value())
            advance(witness_b.value());
    }

    return std::nullopt;
}

template<typename QH, typename NI, typename ITAH, typename IAWSH, typename IFWSH, typename ICNL, typename IGC, typename IDESC>
void pud_candidate_specialization_head<QH, NI, ITAH, IAWSH, IFWSH, ICNL, IGC, IDESC>::witness_refuted(pud_mhws_head_id witness_id) {
    DEBUG_ASSERT(witness_scan_.has_value());
    
    // invalidate the witness
    auto& witness_a = witness_scan_->witness_a;
    auto& witness_b = witness_scan_->witness_b;

    if (witness_a.has_value() && witness_a.value() == witness_id)
        witness_a = std::nullopt;
    else {
        // wasn't the first guy, so it SHOULD be the second
        DEBUG_ASSERT(witness_b.has_value() && witness_b.value() == witness_id);
        witness_b = std::nullopt;
    }
}

template<typename QH, typename NI, typename ITAH, typename IAWSH, typename IFWSH, typename ICNL, typename IGC, typename IDESC>
std::optional<typename pud_candidate_specialization_head<QH, NI, ITAH, IAWSH, IFWSH, ICNL, IGC, IDESC>::witness>
pud_candidate_specialization_head<QH, NI, ITAH, IAWSH, IFWSH, ICNL, IGC, IDESC>::try_replace_witness() {
    // scan for replacement starting from next_witness_root_

    auto& next_witness_root = witness_scan_->next_witness_root_it;
    auto& end_witness_root = witness_scan_->end_witness_root_it;

    for (; next_witness_root != end_witness_root; ++next_witness_root) {
        QH search_root_handle = descend_.descend(current_handle_, next_witness_root->handle.node).value();
    
        auto optional_new_head_id = try_add_head_.try_add_head(search_root_handle);
        
        if (!optional_new_head_id.has_value())
            continue; // try the next witness root
    
        return witness{
            .id = optional_new_head_id.value(),
            .handle = search_root_handle,
        };
    }

    return std::nullopt;
}

template<typename QH, typename NI, typename ITAH, typename IAWSH, typename IFWSH, typename ICNL, typename IGC, typename IDESC>
void pud_candidate_specialization_head<QH, NI, ITAH, IAWSH, IFWSH, ICNL, IGC, IDESC>::advance(witness& survivor) {
    // advance toward the surviving witness search
    std::optional<pud_witness_advance_result<QH, NI>> ar = advance_witness_search_head_.advance_head(survivor.id);
    
    // update our position
    node_path_.push_back(survivor.handle.node());
    current_handle_ = survivor.handle;
    survivor.handle = ar->root_handle;
    
    if (!ar.has_value()) {
        // we advanced onto a leaf. there are no children to initialize the
        //     witness scan with.
        witness_scan_ = std::nullopt;
        return;
    }

    // update our witness scan
    witness_scan_->next_witness_root_it = ar->root_next_sibling_it;
    witness_scan_->end_witness_root_it = ar->root_end_sibling_it;
}

#endif
