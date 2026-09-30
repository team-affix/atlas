#ifndef PUD_WITNESS_SEARCH_HEAD_HPP
#define PUD_WITNESS_SEARCH_HEAD_HPP

#include <deque>
#include <optional>
#include "value_objects/pud_node.hpp"
#include "value_objects/pud_query_position.hpp"

template<
    typename QueryHandle,
    typename ChildIterator,
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
        pud_query_position<QueryHandle> search_root_position);
    pud_witness_search_head(
        const pud_witness_search_head& other,
        QueryHandle search_root_handle);
    pud_query_position<QueryHandle> advance_root();
    std::optional<const pud_node*> resume();
private:
    struct frame {
        pud_query_position<QueryHandle> position;
        ChildIterator next_child_it;
        ChildIterator end_child_it;
    };

    ICheckNodeLeaf& check_node_leaf_;
    IGetNodeChildren& get_node_children_;
    IPropagateQueryNodeHandle& propagate_query_node_handle_;
    IGetCallSiteIdx& get_call_site_idx_;

    std::deque<frame> frame_stack_;
    bool descending_;
};

template<typename QH, typename CI, typename ICNL, typename IGCN, typename IPQN, typename IGCSI>
pud_witness_search_head<QH, CI, ICNL, IGCN, IPQN, IGCSI>::pud_witness_search_head(
    ICNL& check_node_leaf,
    IGCN& get_node_children,
    IPQN& propagate_query_node_handle,
    IGCSI& get_call_site_idx,
    pud_query_position<QH> search_root_position) :
    check_node_leaf_(check_node_leaf),
    get_node_children_(get_node_children),
    propagate_query_node_handle_(propagate_query_node_handle),
    get_call_site_idx_(get_call_site_idx),
    frame_stack_({search_root_position}),
    descending_(true)
{}

template<typename QH, typename CI, typename ICNL, typename IGCN, typename IPQN, typename IGCSI>
pud_witness_search_head<QH, CI, ICNL, IGCN, IPQN, IGCSI>::pud_witness_search_head(
    const pud_witness_search_head& other,
    QH new_query_handle) :
    check_node_leaf_(other.check_node_leaf_),
    get_node_children_(other.get_node_children_),
    propagate_query_node_handle_(other.propagate_query_node_handle_),
    get_call_site_idx_(other.get_call_site_idx_)
{
    // walk our frame stack using the new query handle
    // if we cannot proceed, don't search. just leave the current stack

    QH current_query_handle = new_query_handle;
    
    for (const auto& current_frame : other.frame_stack_) {

        // propagate the query to the current node
        auto optional_current_query_handle = propagate_query_node_handle_.propagate(
            current_query_handle, current_frame.position.node);

        if (!optional_current_query_handle.has_value())
            break;

        pud_query_position<QH> new_position = {
            .handle = current_query_handle,
            .node = current_frame.position.node,
        };

        frame new_frame {
            .position = new_position,
            .next_child_it = current_frame.next_child_it,
            .end_child_it = current_frame.end_child_it,
        };

        frame_stack_.push_back(new_frame);
    }

    // if we forked all the frames, we are still descending (did not hit conflict)
    descending_ = frame_stack_.size() == other.frame_stack_.size();
}

template<typename QH, typename CI, typename ICNL, typename IGCN, typename IPQN, typename IGCSI>
pud_query_position<QH> pud_witness_search_head<QH, CI, ICNL, IGCN, IPQN, IGCSI>::advance_root() {
    pud_query_position<QH> root_position = frame_stack_.front();
    frame_stack_.pop_front();
    return root_position;
}
template<typename QH, typename CI, typename ICNL, typename IGCN, typename IPQN, typename IGCSI>
std::optional<const pud_node*> pud_witness_search_head<QH, CI, ICNL, IGCN, IPQN, IGCSI>::resume() {
    // basic idea:
    //     at any point in time, if the current frame is a leaf, we are done.
    //     while not leaf, get children. initialize the iterator to begin of the children.
    
    while (!frame_stack_.empty()) {
        // get children of current frame
        frame& current_frame = frame_stack_.back();

        const pud_query_position<QH>& current_position = current_frame.position;
        
        const pud_node* current_node = current_position.node;
        
        if (check_node_leaf_.check_leaf(current_node))
            return current_node;

        if (descending_) {
            auto children = get_node_children_.get(current_node);
            current_frame.next_child_it = children.begin();
            current_frame.end_child_it = children.end();
        }

        if (current_frame.next_child_it ==
            current_frame.end_child_it) {
            frame_stack_.pop_back();
            descending_ = false;
            continue;
        }

        // see if we can propagate the query to the next child
        auto child_node = *(current_frame.next_child_it++);
        auto child_optional_query_node_handle = propagate_query_node_handle_.propagate(current_frame.handle, child_node);

        // if failed to propagate, skip the child
        if (!child_optional_query_node_handle.has_value())
            continue;
        
        // create child frame
        frame new_frame = {
            .position = {
                .node = child_node,
                .handle = child_optional_query_node_handle.value(),
            },
        };

        frame_stack_.push_back(new_frame);
        descending_ = true;
    }

    return std::nullopt;
}

#endif
