#ifndef PUD_WITNESS_SEARCH_FRAME_HPP
#define PUD_WITNESS_SEARCH_FRAME_HPP

#include "value_objects/pud_lineage.hpp"
#include "value_objects/pud_node.hpp"

template<typename QueryNodeHandle, typename NodeChildren>
struct pud_witness_search_frame {
    const pud_node* node;
    QueryNodeHandle handle;
    const pud_lineage* callee_lineage;
    typename NodeChildren::const_iterator next_child_it;
    typename NodeChildren::const_iterator end_child_it;
};

#endif
