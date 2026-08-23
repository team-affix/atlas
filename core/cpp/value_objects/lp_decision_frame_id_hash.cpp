#include "value_objects/lp_decision_frame_id_hash.hpp"

#include <functional>

// Commutative combine: the hash must agree with unordered_set's unordered
// operator==, so it cannot depend on iteration order.
size_t lp_decision_frame_id_hash::operator()(const lp_decision_frame_id& id) const noexcept {
    size_t seed = id.size();
    for (const resolution_lineage* rl : id)
        seed += std::hash<const resolution_lineage*>{}(rl);
    return seed;
}
