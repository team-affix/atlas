#ifndef PUD_CANDIDATE_SEARCH_HPP
#define PUD_CANDIDATE_SEARCH_HPP

#include <optional>
#include <utility>
#include "value_objects/om_interval.hpp"

template<
    typename Node,
    typename WitnessSearch,
    typename IAllocateChildInterval,
    typename IEnterNode>
struct pud_candidate_search {
    pud_candidate_search(
        IAllocateChildInterval& allocate_child_interval,
        IEnterNode& enter_node,
        uint32_t frame_offset,
        const Node& search_root,
        om_interval search_root_interval);
    bool resume(bool witness_search_a);
private:

    uint32_t frame_offset_;
    const Node* current_;
    om_interval current_interval_;

    std::optional<WitnessSearch> witness_search_a_;
    std::optional<WitnessSearch> witness_search_b_;
};

#endif
