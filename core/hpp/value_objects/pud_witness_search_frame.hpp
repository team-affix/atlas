#ifndef PUD_WITNESS_SEARCH_FRAME_HPP
#define PUD_WITNESS_SEARCH_FRAME_HPP

#include "value_objects/pud_lineage.hpp"

template<typename QueryNodeHandle, typename NodeChildren>
struct pud_witness_search_frame {
    const pud_lineage* lineage;
    QueryNodeHandle handle;
    typename NodeChildren::const_iterator next_child_it;
};

#endif
