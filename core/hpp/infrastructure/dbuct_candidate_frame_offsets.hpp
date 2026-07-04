#ifndef DBUCT_CANDIDATE_FRAME_OFFSETS_HPP
#define DBUCT_CANDIDATE_FRAME_OFFSETS_HPP

#include <cstdint>
#include <unordered_map>
#include "value_objects/lineage.hpp"
#include "debug_assert.hpp"

// Delayed-backtracking variant of candidate_frame_offsets (see dbuct_goal_exprs
// for the rationale behind snapshot()/restore()).
struct dbuct_candidate_frame_offsets {
    using snapshot_t = std::unordered_map<const resolution_lineage*, uint32_t>;

    uint32_t get(const resolution_lineage* rl) const { return offsets_.at(rl); }

    void set(const resolution_lineage* rl, uint32_t frame_offset) {
        auto [_, inserted] = offsets_.insert({rl, frame_offset});
        DEBUG_ASSERT(inserted);
    }

    void unset(const resolution_lineage* rl) {
        auto erased = offsets_.erase(rl);
        DEBUG_ASSERT(erased == 1);
    }

    void clear_candidate_frame_offsets() { offsets_.clear(); }

    snapshot_t snapshot() const { return offsets_; }
    void restore(snapshot_t s) { offsets_ = std::move(s); }

private:
    std::unordered_map<const resolution_lineage*, uint32_t> offsets_;
};

#endif
