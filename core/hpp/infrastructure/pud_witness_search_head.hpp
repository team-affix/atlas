#ifndef PUD_WITNESS_SEARCH_HEAD_HPP
#define PUD_WITNESS_SEARCH_HEAD_HPP

#include <deque>
#include <optional>
#include "value_objects/pud_node.hpp"
#include "value_objects/pud_witness_advance_result.hpp"

template<
    typename QueryHandle,
    typename NodeIterator,
    typename ICheckNodeLeaf,
    typename IGetNodeChildren,
    typename IPropagateQueryNodeHandle,
    typename IGetCallSiteIdx>
struct pud_witness_search_head {
    pud_witness_search_head(
        ICheckNodeLeaf& check_node_leaf,
        IGetNodeChildren& get_node_children,
        IPropagateQueryNodeHandle& propagate_query_node_handle,
        IGetCallSiteIdx& get_call_site_idx,
        QueryHandle search_root_handle);
    pud_witness_search_head(
        const pud_witness_search_head& other,
        QueryHandle search_root_handle);
    std::optional<pud_witness_advance_result<QueryHandle, NodeIterator>> advance();
    std::optional<const pud_node*> resume();
private:
    struct frame {
        std::optional<QueryHandle> handle;
        QueryHandle parent_handle;
        NodeIterator next_sibling_it;
        NodeIterator end_sibling_it;
    };

    std::optional<QueryHandle> try_replace_sibling(const QueryHandle& parent_handle, NodeIterator& next_sibling_it, NodeIterator end_sibling_it);

    ICheckNodeLeaf& check_node_leaf_;
    IGetNodeChildren& get_node_children_;
    IPropagateQueryNodeHandle& propagate_query_node_handle_;
    IGetCallSiteIdx& get_call_site_idx_;

    QueryHandle search_root_handle_;
    std::deque<frame> frame_stack_;
    bool descending_;
};

template<typename QH, typename NI, typename ICNL, typename IGCN, typename IPQN, typename IGCSI>
pud_witness_search_head<QH, NI, ICNL, IGCN, IPQN, IGCSI>::pud_witness_search_head(
    ICNL& check_node_leaf,
    IGCN& get_node_children,
    IPQN& propagate_query_node_handle,
    IGCSI& get_call_site_idx,
    QH search_root_handle) :
    check_node_leaf_(check_node_leaf),
    get_node_children_(get_node_children),
    propagate_query_node_handle_(propagate_query_node_handle),
    get_call_site_idx_(get_call_site_idx),
    search_root_handle_(search_root_handle),
    descending_(true)
{}

template<typename QH, typename CI, typename ICNL, typename IGCN, typename IPQN, typename IGCSI>
pud_witness_search_head<QH, CI, ICNL, IGCN, IPQN, IGCSI>::pud_witness_search_head(
    const pud_witness_search_head& other,
    QH search_root_handle) :
    check_node_leaf_(other.check_node_leaf_),
    get_node_children_(other.get_node_children_),
    propagate_query_node_handle_(other.propagate_query_node_handle_),
    get_call_site_idx_(other.get_call_site_idx_),
    search_root_handle_(search_root_handle)
{
    // walk our frame stack using the new query handle
    // if we cannot proceed, don't search. just leave the current stack

    QH parent_handle = search_root_handle;
    
    for (const auto& other_frame : other.frame_stack_) {
        
        // it will always have a value if we are forking it
        const QH& other_handle = other_frame.handle.value();
        
        // propagate the query to the current node
        auto optional_current_handle = propagate_query_node_handle_.propagate(
            parent_handle, other_handle.node());e

        if (!optional_current_handle.has_value())
            break;

        frame new_frame = {
            .handle = optional_current_handle.value(),
            .parent_handle = parent_handle,
            .next_sibling_it = other_frame.next_sibling_it,
            .end_sibling_it = other_frame.end_sibling_it,
        };

        frame_stack_.push_back(new_frame);
        
        parent_handle = optional_current_handle.value();
    }

    // if we forked all the frames, we are still descending (did not hit conflict)
    descending_ = frame_stack_.size() == other.frame_stack_.size();
}

template<typename QH, typename NI, typename ICNL, typename IGCN, typename IPQN, typename IGCSI>
std::optional<pud_witness_advance_result<QH, NI>> pud_witness_search_head<QH, NI, ICNL, IGCN, IPQN, IGCSI>::advance() {
    if (frame_stack_.empty())
        return std::nullopt;

    frame first_stack_frame = frame_stack_.front();

    frame_stack_.pop_front();

    pud_witness_advance_result<QH, NI> result = {
        .root_handle = first_stack_frame.handle.value(),
        .root_next_sibling_it = first_stack_frame.next_sibling_it,
        .root_end_sibling_it = first_stack_frame.end_sibling_it,
    };

    search_root_handle_ = first_stack_frame.handle.value();

    return result;
}

template<typename QH, typename NI, typename ICNL, typename IGCN, typename IPQN, typename IGCSI>
std::optional<const pud_node*> pud_witness_search_head<QH, NI, ICNL, IGCN, IPQN, IGCSI>::resume() {
    // basic idea:
    //     at any point in time, if the current frame is a leaf, we are done.
    //     while not leaf, get children. initialize the iterator to begin of the children.
    
    while (!frame_stack_.empty()) {
        // get children of current frame
        frame& current_frame = frame_stack_.back();

        const auto& optional_current_handle = current_frame.handle;

        if (!descending_ || !optional_current_handle.has_value()) {
            // if we are backtracking OR need to initialize handle,
            // its the same algo.

            const QH& parent_handle = current_frame.parent_handle;

            // try to propagate the query to the next sibling
            current_frame.handle = try_replace_sibling(
                parent_handle, current_frame.next_sibling_it, current_frame.end_sibling_it);

            if (!current_frame.handle.has_value()) {
                frame_stack_.pop_back();
                continue;
            }
                
            // if we found replacement sibling, descending_ is true.
            // otherwise, descending_ remains false.
            descending_ = true;
        }

        // handle leaf check / children initialization

        const QH& current_handle = current_frame.handle;
        
        const pud_node* current_node = current_handle.node();
        
        if (check_node_leaf_.check_leaf(current_node))
            return current_node;

        const auto& children = get_node_children_.get(current_node);
        
        frame new_frame = {
            .handle = std::nullopt,
            .parent_handle = current_handle,
            .next_sibling_it = children.begin(),
            .end_sibling_it = children.end(),
        };

        frame_stack_.push_back(new_frame);
    }

    return std::nullopt;
}


template<typename QH, typename NI, typename ICNL, typename IGCN, typename IPQN, typename IGCSI>
std::optional<QH> pud_witness_search_head<QH, NI, ICNL, IGCN, IPQN, IGCSI>::try_replace_sibling(const QH& parent_handle, NI& next_sibling_it, NI end_sibling_it) {
    while (next_sibling_it != end_sibling_it) {
        if (auto next_sibling_handle =
            propagate_query_node_handle_.propagate(
                parent_handle, *(next_sibling_it++))) {
            return next_sibling_handle;
        }
    }
    return std::nullopt;
}

#endif
