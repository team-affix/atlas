#ifndef PUD_CANDIDATE_SEARCH_HEAD_HPP
#define PUD_CANDIDATE_SEARCH_HEAD_HPP

#include <optional>
#include "infrastructure/pud_mhws.hpp"
#include "value_objects/pud_mhws_head_id.hpp"

template<
    typename QueryHandle,
    typename ChildIterator, 
    typename IAdvanceWitnessSearchHead,
    typename IForkWitnessSearchHead,
    typename IGetNodeIsLeaf,
    typename IGetChildren,
    typename IPropagateQueryHandle>
struct pud_candidate_search_head {
    pud_candidate_search_head(
        IAdvanceWitnessSearchHead& advance_witness_search_head,
        IForkWitnessSearchHead& fork_witness_search_head,
        IGetNodeIsLeaf& get_node_is_leaf,
        IGetChildren& get_children,
        IPropagateQueryHandle& propagate_query_handle,
        pud_query_position<QueryHandle> search_root_position);
    pud_candidate_search_head(
        const pud_candidate_search_head& other,
        QueryHandle new_query_handle);
        std::optional<pud_query_position<QueryHandle>> resume();
private:
    struct choice_point_context {
        std::optional<pud_mhws_head_id> witness_search_a_;
        std::optional<pud_mhws_head_id> witness_search_b_;
        ChildIterator next_witness_root_;
        ChildIterator end_witness_root_;
    };

    IAdvanceWitnessSearchHead& advance_witness_search_head_;
    IForkWitnessSearchHead& fork_witness_search_head_;
    IGetNodeIsLeaf& get_node_is_leaf_;
    IGetChildren& get_children_;
    IPropagateQueryHandle& propagate_query_handle_;

    std::deque<const pud_node*>         node_path_;
    QueryHandle                         current_query_handle_;
    std::optional<choice_point_context> choice_point_context_;
};

template<typename QH, typename CI, typename IAWSH, typename IFWSH, typename IGNL, typename IGC, typename IPQH>
pud_candidate_search_head<QH, CI, IAWSH, IFWSH, IGNL, IGC, IPQH>::pud_candidate_search_head(
    IAWSH& advance_witness_search_head,
    IFWSH& fork_witness_search_head,
    IGNL& get_node_is_leaf,
    IGC& get_children,
    IPQH& propagate_query_handle,
    pud_query_position<QH> search_root_position) :
    advance_witness_search_head_(advance_witness_search_head),
    fork_witness_search_head_(fork_witness_search_head),
    get_node_is_leaf_(get_node_is_leaf),
    get_children_(get_children),
    propagate_query_handle_(propagate_query_handle),
    node_path_({search_root_position.node}),
    current_query_handle_(search_root_position.query_handle) {
}

template<typename QH, typename CI, typename IAWSH, typename IFWSH, typename IGNL, typename IGC, typename IPQH>
pud_candidate_search_head<QH, CI, IAWSH, IFWSH, IGNL, IGC, IPQH>::pud_candidate_search_head(
    const pud_candidate_search_head& other,
    QH new_query_handle) :
    advance_witness_search_head_(other.advance_witness_search_head_),
    fork_witness_search_head_(other.fork_witness_search_head_),
    get_node_is_leaf_(other.get_node_is_leaf_),
    get_children_(other.get_children_),
    propagate_query_handle_(other.propagate_query_handle_) {

    QH current_query_handle = new_query_handle;
    
    // walk our frame stack using the new query handle
    for (const auto& node : other.node_path_) {
        auto optional_new_query_handle = propagate_query_handle_.propagate(current_query_handle, node);

        if (!optional_new_query_handle.has_value())
            break;

        node_path_.push_back(node);

        current_query_handle = optional_new_query_handle.value();
    }

    if (node_path_.size() != other.node_path_.size()) {
        // we hit a conflict, set next = end so resume fails to find replacement witnesses
        choice_point_context_ = {
            .witness_search_a_ = std::nullopt,
            .witness_search_b_ = std::nullopt,
            .next_witness_root_ = CI{},
            .end_witness_root_ = CI{},
        };
        return;
    }

    // if the other head was a self-witness, we are done
    if (!other.choice_point_context_.has_value())
        return;

    // we are a choice-point
    
    const auto& other_witness_search_a = other.choice_point_context_->witness_search_a_;
    const auto& other_witness_search_b = other.choice_point_context_->witness_search_b_;
    
    std::optional<pud_mhws_head_id> forked_witness_a;
    std::optional<pud_mhws_head_id> forked_witness_b;

    if (other_witness_search_a.has_value())
        forked_witness_a = fork_witness_search_head_.try_fork_head(other_witness_search_a.value(), current_query_handle);
    if (other_witness_search_b.has_value())
        forked_witness_b = fork_witness_search_head_.try_fork_head(other_witness_search_b.value(), current_query_handle);
    
    auto other_next_witness_root = other.choice_point_context_->next_witness_root_;
    auto other_end_witness_root = other.choice_point_context_->end_witness_root_;
        
    choice_point_context_ = {
        .witness_search_a_ = forked_witness_a,
        .witness_search_b_ = forked_witness_b,
        .next_witness_root_ = other_next_witness_root,
        .end_witness_root_ = other_end_witness_root,
    };
}

template<typename QH, typename CI, typename IAWSH, typename IFWSH, typename IGNL, typename IGC, typename IPQH>
std::optional<pud_query_position<QH>> pud_candidate_search_head<QH, CI, IAWSH, IFWSH, IGNL, IGC, IPQH>::resume() {
    // there are three possible starting states:
    // 1. no witnesses found yet
    // 2. self-witness situation
    // 3. choice-point situation

    // when advancing, whether we find ourselves as a self-witness or not
    //     is unknown until we do some witness search.

    // check if we are at a leaf node
    
    
    // if we are currently a choice-point, try to resume both witness searches.
    //     any witness search which fails to find a solution is marked as such and
    //     we resume from next_witness_root_ searching for replacements.
    
    // we always try to find two witnesses eagerly. Each witness is found down a
    //     unique outgoing edge from the current node.
    
    // if we cannot find two witnesses, we advance toward the surviving witness search

    const auto& current_position = position_stack_.back();
    
    // if we are already at a leaf node, we are done. we are a self-witness
    if (get_node_is_leaf_.get(current_position.node)) {
        choice_point_context_ = std::nullopt;
        return current_position;
    }

    // from this point on, we are not a self-witness
    if (!choice_point_context_.has_value()) {
        const auto& children = get_children_.get(current_position_.node);
        
    }
        
}

#endif
