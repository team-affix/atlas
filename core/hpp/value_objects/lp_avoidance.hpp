#ifndef LP_AVOIDANCE_HPP
#define LP_AVOIDANCE_HPP

#include <compare>
#include <vector>
#include "value_objects/avoidance_id.hpp"
#include "value_objects/lineage.hpp"

// A frame-local copy of an avoidance, already reduced for the frame that holds
// it. No watcher positions: a frame watches every member, so reduction is
// literal member erasure. An empty members vector is a tombstone -- a live entry
// always has at least two members -- which keeps cache positions stable for the
// child continuations that index into them.
struct lp_avoidance {
    avoidance_id id;
    std::vector<const resolution_lineage*> members;
    auto operator<=>(const lp_avoidance&) const = default;
};

#endif
