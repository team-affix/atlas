#ifndef PUD_WITNESS_SEARCH_HEAD_HPP
#define PUD_WITNESS_SEARCH_HEAD_HPP

#include <deque>
#include "value_objects/pud_witness_search_frame.hpp"
#include "value_objects/pud_node.hpp"

template<
    typename WitnessSearchFrame,
    typename IGetNodeChildren,
    typename IGetChildQueryNodeHandle>
struct pud_witness_search_head {
    pud_witness_search_head(
        IGetNodeChildren& get_node_children,
        IGetChildQueryNodeHandle& get_child_query_node_handle,
        WitnessSearchFrame search_root_frame);
    WitnessSearchFrame advance_root();
    bool next_solution();
private:
    IGetNodeChildren& get_node_children_;
    IGetChildQueryNodeHandle& get_child_query_node_handle_;
    
    std::deque<WitnessSearchFrame> frame_stack_;
};

template<typename WSF, typename IGCN, typename IGCQN>
pud_witness_search_head<WSF, IGCN, IGCQN>::pud_witness_search_head(
    IGCN& get_node_children,
    IGCQN& get_child_query_node_handle,
    WSF search_root_frame) :
    get_node_children_(get_node_children),
    get_child_query_node_handle_(get_child_query_node_handle),
    frame_stack_({search_root_frame})
{}

template<typename WSF, typename IGCN, typename IGCQN>
WSF pud_witness_search_head<WSF, IGCN, IGCQN>::advance_root() {
    WSF root_frame = frame_stack_.front();
    frame_stack_.pop_front();
    return root_frame;
}

template<typename WSF, typename IGCN, typename IGCQN>
bool pud_witness_search_head<WSF, IGCN, IGCQN>::next_solution() {
    // basic idea:
    //     at any point in time, if the current frame is a leaf, we are done.
    //     while not leaf, get children. initialize the iterator to begin of the children.
    
    if (frame_stack_.empty())
        return false;

    while (true) {
        // get children of current frame
        WSF current_frame = frame_stack_.back();
        const pud_node* current_node = current_frame.node;
        const auto& children = get_node_children_.get(current_node);

        if (children.empty())
            return false;

        // initialize the iterator to begin of the children
        auto child_it = children.begin();

        // push the child frames onto the stack
        for (const pud_node* child_node : children) {
            frame_stack_.push_back(WSF{child_node});
    }
    
}

#endif
