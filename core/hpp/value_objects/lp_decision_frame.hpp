#ifndef LP_DECISION_FRAME_HPP
#define LP_DECISION_FRAME_HPP

#include <map>
#include <unordered_map>
#include <unordered_set>
#include "value_objects/avoidance_id.hpp"
#include "value_objects/lineage.hpp"
#include "value_objects/lp_avoidance_update.hpp"
#include "value_objects/lp_timestamp.hpp"

// One decision frame's cached CDCL state.
//
// Everything keys on avoidance_id. change_log is the only ordered container and
// is the point of the design: lower_bound(continuations[edge]) to the end is
// exactly the set of avoidances that changed since that child last visited.
// timestamps is the reverse half of that bimap, so a changed avoidance can have
// its old log entry erased before the new one is appended -- the log holds one
// entry per avoidance, not one per update.
//
// operator== only, no operator<=>: the unordered containers are not three-way
// comparable, and nothing ever orders a frame -- frames are only map values and
// are mutated in place.
struct lp_decision_frame {
    // what this frame knows: current members, or that the avoidance is gone
    std::unordered_map<avoidance_id, lp_avoidance_update> avoidances;
    std::unordered_map<avoidance_id, lp_timestamp> timestamps;
    std::map<lp_timestamp, avoidance_id> change_log;
    // child edge -> the next timestamp that child is owed
    std::unordered_map<const resolution_lineage*, lp_timestamp> continuations;
    // delivered by a parent, not yet merged in by enter()
    std::unordered_map<avoidance_id, lp_avoidance_update> mailbox;
    std::unordered_map<const goal_lineage*, std::unordered_set<avoidance_id>> watched_goals;
    // eliminations this frame has proven. The proof is permanent but the
    // elimination itself is undone at tear-down, so these are re-applied every
    // time the sim walks back into this frame.
    std::unordered_set<const resolution_lineage*> forced;
    bool operator==(const lp_decision_frame&) const = default;
};

#endif
