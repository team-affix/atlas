#ifndef SRT_IN_FLIGHT_INSERT_HPP
#define SRT_IN_FLIGHT_INSERT_HPP

#include <compare>
#include "value_objects/lineage.hpp"

struct srt_in_flight_insert {
    const goal_lineage* gl;
    auto operator<=>(const srt_in_flight_insert&) const = default;
};

#endif
