#ifndef LP_AVOIDANCE_CONTENT_UPDATE_HPP
#define LP_AVOIDANCE_CONTENT_UPDATE_HPP

#include <compare>
#include "value_objects/lp_avoidance.hpp"

// "Here is my current version of this avoidance." Carries the whole avoidance
// rather than a delta, because a receiver may be behind by several reductions.
struct lp_avoidance_content_update {
    lp_avoidance avoidance;
    auto operator<=>(const lp_avoidance_content_update&) const = default;
};

#endif
