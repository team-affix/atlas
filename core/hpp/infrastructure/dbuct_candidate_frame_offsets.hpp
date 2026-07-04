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

    uint32_t get(const resolution_lineage* rl) const;
    void set(const resolution_lineage* rl, uint32_t frame_offset);
    void unset(const resolution_lineage* rl);
    void clear_candidate_frame_offsets();

    snapshot_t snapshot() const;
    void restore(snapshot_t s);

private:
    std::unordered_map<const resolution_lineage*, uint32_t> offsets_;
};

inline uint32_t dbuct_candidate_frame_offsets::get(const resolution_lineage* rl) const { return offsets_.at(rl); }

inline void dbuct_candidate_frame_offsets::set(const resolution_lineage* rl, uint32_t frame_offset) {
    auto [_, inserted] = offsets_.insert({rl, frame_offset});
    DEBUG_ASSERT(inserted);
}

inline void dbuct_candidate_frame_offsets::unset(const resolution_lineage* rl) {
    auto erased = offsets_.erase(rl);
    DEBUG_ASSERT(erased == 1);
}

inline void dbuct_candidate_frame_offsets::clear_candidate_frame_offsets() { offsets_.clear(); }

inline dbuct_candidate_frame_offsets::snapshot_t dbuct_candidate_frame_offsets::snapshot() const { return offsets_; }
inline void dbuct_candidate_frame_offsets::restore(snapshot_t s) { offsets_ = std::move(s); }

#endif
