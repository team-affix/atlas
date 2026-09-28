#ifndef PUD_WITNESS_SEARCH_HEAD_HPP
#define PUD_WITNESS_SEARCH_HEAD_HPP

#include <deque>
#include "value_objects/pud_witness_search_frame.hpp"
#include "value_objects/pud_node.hpp"

template<
    typename WitnessSearchFrame,
    typename ICheckNodeLeaf,
    typename IGetNodeChildren,
    typename IPropagateQueryNodeHandle,
    typename IMakeInferenceLineage>
struct pud_witness_search_head {
    pud_witness_search_head(
        ICheckNodeLeaf& check_node_leaf,
        IGetNodeChildren& get_node_children,
        IPropagateQueryNodeHandle& propagate_query_node_handle,
        IMakeInferenceLineage& make_inference_lineage,
        WitnessSearchFrame search_root_frame);
    WitnessSearchFrame advance_root();
    bool resume();
private:
    ICheckNodeLeaf& check_node_leaf_;
    IGetNodeChildren& get_node_children_;
    IPropagateQueryNodeHandle& propagate_query_node_handle_;
    IMakeInferenceLineage& make_inference_lineage_;

    std::deque<WitnessSearchFrame> frame_stack_;
    bool descending_;
};

template<typename WSF, typename ICNL, typename IGCN, typename IPQN, typename IML>
pud_witness_search_head<WSF, ICNL, IGCN, IPQN, IML>::pud_witness_search_head(
    ICNL& check_node_leaf,
    IGCN& get_node_children,
    IPQN& propagate_query_node_handle,
    IML& make_inference_lineage,
    WSF search_root_frame) :
    check_node_leaf_(check_node_leaf),
    get_node_children_(get_node_children),
    propagate_query_node_handle_(propagate_query_node_handle),
    make_inference_lineage_(make_inference_lineage),
    frame_stack_({search_root_frame}),
    descending_(true)
{}

template<typename WSF, typename ICNL, typename IGCN, typename IPQN, typename IML>
WSF pud_witness_search_head<WSF, ICNL, IGCN, IPQN, IML>::advance_root() {
    WSF root_frame = frame_stack_.front();
    frame_stack_.pop_front();
    return root_frame;
}

template<typename WSF, typename ICNL, typename IGCN, typename IPQN, typename IML>
bool pud_witness_search_head<WSF, ICNL, IGCN, IPQN, IML>::resume() {
    // basic idea:
    //     at any point in time, if the current frame is a leaf, we are done.
    //     while not leaf, get children. initialize the iterator to begin of the children.
    
    while (!frame_stack_.empty()) {
        // get children of current frame
        WSF current_frame = frame_stack_.back();
        
        const pud_node* current_node = current_frame.node;
        
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

        auto next_child_it = current_frame.next_child_it++;
        auto child_node = next_child_it.value;
        auto child_query_handle = propagate_query_node_handle_.propagate(current_frame.handle, child_node);
        auto child_lineage = make_inference_lineage_.make_inference(current_frame.lineage, child_node);

        WSF new_frame = {
            
            current_node,
            current_frame.lineage,
            next_child_it,
            current_frame.end_child_it
        };

        frame_stack_.push_back(new_frame);
        descending_ = true;
        
    }

    return false;

}

#endif
