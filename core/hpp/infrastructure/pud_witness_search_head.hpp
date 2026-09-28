#ifndef PUD_WITNESS_SEARCH_HEAD_HPP
#define PUD_WITNESS_SEARCH_HEAD_HPP

#include <deque>
#include "value_objects/pud_witness_search_frame.hpp"
#include "value_objects/pud_node.hpp"

template<
    typename QueryNodeHandle,
    typename NodeChildren,
    typename IGetNodeChildren,
    typename IGetChildQueryNodeHandle>
struct pud_witness_search_head {
    using frame = pud_witness_search_frame<QueryNodeHandle, NodeChildren>;
    pud_witness_search_head(
        IGetNodeChildren& get_node_children,
        IGetChildQueryNodeHandle& get_child_query_node_handle,
        QueryNodeHandle search_root_handle);
    frame advance_root();
    bool next_solution();
private:
    IGetNodeChildren& get_node_children_;
    IGetChildQueryNodeHandle& get_child_query_node_handle_;
    
    QueryNodeHandle search_root_handle_;
    std::deque<frame> frame_stack_;
};

#endif
