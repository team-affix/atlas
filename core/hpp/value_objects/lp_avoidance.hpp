#ifndef LP_AVOIDANCE_HPP
#define LP_AVOIDANCE_HPP

#include <compare>
#include <vector>
#include "value_objects/avoidance_id.hpp"
#include "value_objects/lineage.hpp"

// A frame-local copy of an avoidance, already reduced for the frame that holds
// it. No watcher positions: a frame watches every member, so reduction is
// literal member erasure. Members are never empty -- a one-member avoidance is
// a forced elimination, and one that can no longer be violated is reported as
// lp_avoidance_satisfied instead.
struct lp_avoidance {
    avoidance_id id;
    std::vector<const resolution_lineage*> members;
    auto operator<=>(const lp_avoidance&) const = default;
};

#endif
