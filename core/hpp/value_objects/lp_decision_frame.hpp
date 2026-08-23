#ifndef LP_DECISION_FRAME_HPP
#define LP_DECISION_FRAME_HPP

#include <cstddef>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "value_objects/avoidance_id.hpp"
#include "value_objects/lineage.hpp"
#include "value_objects/lp_avoidance.hpp"

// One decision frame's cached CDCL state: the avoidances that matter at this
// frame and below, how far each child edge has been fed, and a watchlist over
// every member of every cached avoidance.
//
// operator== only, no operator<=>: the unordered containers are not three-way
// comparable, and nothing ever orders a frame -- frames are only map values and
// are mutated in place.
struct lp_decision_frame {
    // append-only so child continuations stay valid; dead entries are tombstoned
    std::vector<lp_avoidance> cache;
    // child edge -> how much of cache that child has already received
    std::unordered_map<const resolution_lineage*, size_t> continuations;
    // goal -> positions in cache holding a member on that goal
    std::unordered_map<const goal_lineage*, std::unordered_set<size_t>> watched_goals;
    // avoidance ids already received, so a DAG re-delivery is dropped
    std::unordered_set<avoidance_id> received;
    // eliminations this frame has proven. The proof is permanent but the
    // elimination itself is undone at tear-down, so these are re-applied every
    // time the sim walks back into this frame.
    std::unordered_set<const resolution_lineage*> forced;
    bool operator==(const lp_decision_frame&) const = default;
};

#endif
