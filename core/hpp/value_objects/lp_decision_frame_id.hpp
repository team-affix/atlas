#ifndef LP_DECISION_FRAME_ID_HPP
#define LP_DECISION_FRAME_ID_HPP

#include <unordered_set>
#include "value_objects/lineage.hpp"

// A decision frame is identified by the set of decisions taken to reach it.
// Only set equivalence matters, never order.
using lp_decision_frame_id = std::unordered_set<const resolution_lineage*>;

#endif
