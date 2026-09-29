#ifndef PUD_CANDIDATE_SEARCH_HEAD_HPP
#define PUD_CANDIDATE_SEARCH_HEAD_HPP

#include "value_objects/pud_query_position.hpp"

template<typename QueryPosition>
struct pud_candidate_search_head {
    pud_candidate_search_head(
        QueryPosition search_root_position);
    QueryPosition accept();
    bool resume();
private:
    QueryPosition search_root_position_;
    
    
};

#endif
