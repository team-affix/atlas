#include "infrastructure/pud_trail_cache.hpp"
#include <utility>
#include "value_objects/pud_candidate_trail.hpp"
#include "value_objects/pud_goal_trails.hpp"

namespace {

// A node's trails are estimated from what they hold: each goal's box and each candidate with its witness leaves.
size_t estimate_bytes(const pud_node_trails& trails) {
    size_t bytes = 96;
    for (const auto& [goal, boxed] : trails) {
        bytes += 64;
        for (const pud_candidate_trail& candidate : boxed.get().candidates)
            bytes += 48 + 16 * candidate.witness_leaves.size();
    }
    return bytes;
}

}

pud_trail_cache::pud_trail_cache(size_t budget_bytes)
    : budget_bytes_(budget_bytes)
    , kept_bytes_(0)
    , recency_()
    , entries_() {}

const pud_node_trails* pud_trail_cache::find(pud_global_node_id node) {
    const auto found = entries_.find(node);
    if (found == entries_.end())
        return nullptr;
    recency_.splice(recency_.begin(), recency_, found->second.recency);
    return &found->second.trails;
}

void pud_trail_cache::store(pud_global_node_id node, pud_node_trails trails) {
    erase(node);
    const size_t bytes = estimate_bytes(trails);
    recency_.push_front(node);
    entries_.emplace(node, entry{.trails = std::move(trails), .bytes = bytes, .recency = recency_.begin()});
    kept_bytes_ += bytes;
}

void pud_trail_cache::erase(pud_global_node_id node) {
    const auto found = entries_.find(node);
    if (found == entries_.end())
        return;
    kept_bytes_ -= found->second.bytes;
    recency_.erase(found->second.recency);
    entries_.erase(found);
}

void pud_trail_cache::trim() {
    while (kept_bytes_ > budget_bytes_ && !recency_.empty())
        erase(recency_.back());
}
