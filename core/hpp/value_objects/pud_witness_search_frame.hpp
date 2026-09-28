#ifndef PUD_WITNESS_SEARCH_FRAME_HPP
#define PUD_WITNESS_SEARCH_FRAME_HPP

#include "value_objects/pud_lineage.hpp"
#include "value_objects/pud_node.hpp"

template<typename QueryNodeHandle, typename NodeChildren>
struct pud_witness_search_frame {
    QueryNodeHandle handle;
    const pud_node* node;
    const pud_lineage* lineage;
    typename NodeChildren::const_iterator next_child_it;
};

#endif
