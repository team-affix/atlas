#ifndef PUD_QUERY_FRAME_HPP
#define PUD_QUERY_FRAME_HPP

#include "value_objects/pud_query_position.hpp"

template<typename QueryHandle, typename ChildIterator>
struct pud_query_frame {
    pud_query_position<QueryHandle> position;
    ChildIterator next_child_it;
    ChildIterator end_child_it;
};

#endif
