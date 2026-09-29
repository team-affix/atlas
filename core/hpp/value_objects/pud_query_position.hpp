#ifndef PUD_QUERY_POSITION_HPP
#define PUD_QUERY_POSITION_HPP

#include "value_objects/pud_node.hpp"

template<typename QueryNodeHandle>
struct pud_query_position {
    QueryNodeHandle handle;
    const pud_node* node;
};

#endif
