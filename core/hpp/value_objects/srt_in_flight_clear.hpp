#ifndef SRT_IN_FLIGHT_CLEAR_HPP
#define SRT_IN_FLIGHT_CLEAR_HPP

#include <compare>
#include <set>
#include "value_objects/lineage.hpp"

struct srt_in_flight_clear {
    std::set<const goal_lineage*> saved;
    auto operator<=>(const srt_in_flight_clear&) const = default;
};

#endif
