#ifndef PUD_WITNESS_SEARCH_HEAD_HPP
#define PUD_WITNESS_SEARCH_HEAD_HPP

#include <deque>
#include "value_objects/pud_query_position.hpp"
#include "value_objects/pud_node.hpp"

template<
    typename QueryPosition,
    typename ChildIterator,
    typename ICheckNodeLeaf,
    typename IGetNodeChildren,
    typename IPropagateQueryNodeHandle,
    typename IMakeInferenceLineage,
    typename IGetCallSiteIdx>
struct pud_witness_search_head {
    pud_witness_search_head(
        ICheckNodeLeaf& check_node_leaf,
        IGetNodeChildren& get_node_children,
        IPropagateQueryNodeHandle& propagate_query_node_handle,
        IMakeInferenceLineage& make_inference_lineage,
        IGetCallSiteIdx& get_call_site_idx,
        QueryPosition search_root_position);
    QueryPosition advance_root();
    bool resume();
private:
    struct frame {
        QueryPosition position;
        ChildIterator next_child_it;
        ChildIterator end_child_it;
    };

    ICheckNodeLeaf& check_node_leaf_;
    IGetNodeChildren& get_node_children_;
    IPropagateQueryNodeHandle& propagate_query_node_handle_;
    IMakeInferenceLineage& make_inference_lineage_;
    IGetCallSiteIdx& get_call_site_idx_;

    std::deque<frame> frame_stack_;
    bool descending_;
};

template<typename QP, typename CI, typename ICNL, typename IGCN, typename IPQN, typename IML, typename IGCSI>
pud_witness_search_head<QP, CI, ICNL, IGCN, IPQN, IML, IGCSI>::pud_witness_search_head(
    ICNL& check_node_leaf,
    IGCN& get_node_children,
    IPQN& propagate_query_node_handle,
    IML& make_inference_lineage,
    IGCSI& get_call_site_idx,
    QP search_root_position) :
    check_node_leaf_(check_node_leaf),
    get_node_children_(get_node_children),
    propagate_query_node_handle_(propagate_query_node_handle),
    make_inference_lineage_(make_inference_lineage),
    get_call_site_idx_(get_call_site_idx),
    frame_stack_({search_root_position}),
    descending_(true)
{}

template<typename QP, typename CI, typename ICNL, typename IGCN, typename IPQN, typename IML, typename IGCSI>
QP pud_witness_search_head<QP, CI, ICNL, IGCN, IPQN, IML, IGCSI>::advance_root() {
    QP root_position = frame_stack_.front();
    frame_stack_.pop_front();
    return root_position;
}

template<typename QP, typename CI, typename ICNL, typename IGCN, typename IPQN, typename IML, typename IGCSI>
bool pud_witness_search_head<QP, CI, ICNL, IGCN, IPQN, IML, IGCSI>::resume() {
    // basic idea:
    //     at any point in time, if the current frame is a leaf, we are done.
    //     while not leaf, get children. initialize the iterator to begin of the children.
    
    while (!frame_stack_.empty()) {
        // get children of current frame
        frame& current_frame = frame_stack_.back();

        const QP& current_position = current_frame.position;
        
        const pud_node* current_node = current_position.node;
        
        if (check_node_leaf_.check_leaf(current_node))
            return true;

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
        auto next_child_it = current_frame.next_child_it++;
        auto child_node = next_child_it.value;
        auto child_optional_handle = propagate_query_node_handle_.propagate(current_frame.handle, child_node);

        // if failed to propagate, skip the child
        if (!child_optional_handle.has_value())
            continue;
        
        // compute the child lineage
        auto current_call_site_idx = get_call_site_idx_.get(current_node);
        auto child_callee_lineage = next_child_it.key;
        auto child_lineage = make_inference_lineage_.make_inference(current_position.lineage, current_call_site_idx, child_callee_lineage);

        // create child frame
        frame new_frame = {
            .position = {
                .node = child_node,
                .lineage = child_lineage,
                .handle = child_optional_handle.value(),
            },
        };

        frame_stack_.push_back(new_frame);
        descending_ = true;
    }

    return false;
}

#endif
