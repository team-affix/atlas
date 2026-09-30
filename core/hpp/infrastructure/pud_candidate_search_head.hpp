#ifndef PUD_CANDIDATE_SEARCH_HEAD_HPP
#define PUD_CANDIDATE_SEARCH_HEAD_HPP

#include <optional>
#include <utility>
#include "infrastructure/pud_mhws.hpp"
#include "value_objects/pud_mhws_head_id.hpp"

template<
    typename QueryPosition,
    typename ChildIterator, 
    typename IAdvanceWitnessSearchHead>
struct pud_candidate_search_head {
    pud_candidate_search_head(
        IAdvanceWitnessSearchHead& advance_witness_search_head,
        QueryPosition search_root_position);
    std::optional<QueryPosition> resume();
private:
    struct choice_point_context {
        ChildIterator next_witness_root_;
        std::pair<pud_mhws_head_id, pud_mhws_head_id> witness_search_head_ids_;
    };

    IAdvanceWitnessSearchHead& advance_witness_search_head_;

    QueryPosition current_position_;
    std::optional<choice_point_context> choice_point_context_;
};

template<typename QP, typename CI, typename IAWSH>
pud_candidate_search_head<QP, CI, IAWSH>::pud_candidate_search_head(
    IAWSH& advance_witness_search_head,
    QP search_root_position) :
    advance_witness_search_head_(advance_witness_search_head),
    current_position_(search_root_position) {
}

template<typename QP, typename CI, typename IAWSH>
std::optional<QP> pud_candidate_search_head<QP, CI, IAWSH>::resume() {
    
}

#endif
