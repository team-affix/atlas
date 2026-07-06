#ifndef DBUCT_CANDIDATE_FRAME_OFFSETS_HPP
#define DBUCT_CANDIDATE_FRAME_OFFSETS_HPP

#include <cstdint>
#include <memory>
#include <unordered_map>
#include "infrastructure/backtrackable_map_erase.hpp"
#include "infrastructure/backtrackable_map_insert.hpp"
#include "infrastructure/tracked.hpp"
#include "infrastructure/trail.hpp"
#include "value_objects/lineage.hpp"

// Delayed-backtracking variant of candidate_frame_offsets (see dbuct_goal_exprs
// for the rationale behind trail-journalled mutations).
struct dbuct_candidate_frame_offsets {
    using map_t = std::unordered_map<const resolution_lineage*, uint32_t>;

    explicit dbuct_candidate_frame_offsets(trail& t);

    uint32_t get(const resolution_lineage* rl) const;
    void set(const resolution_lineage* rl, uint32_t frame_offset);
    void unset(const resolution_lineage* rl);

private:
    tracked<map_t, trail> offsets_;
};

inline dbuct_candidate_frame_offsets::dbuct_candidate_frame_offsets(trail& t) : offsets_(t, map_t{}) {}

inline uint32_t dbuct_candidate_frame_offsets::get(const resolution_lineage* rl) const { return offsets_.get().at(rl); }

inline void dbuct_candidate_frame_offsets::set(const resolution_lineage* rl, uint32_t frame_offset) {
    offsets_.mutate(std::make_unique<backtrackable_map_insert<map_t>>(rl, frame_offset));
}

inline void dbuct_candidate_frame_offsets::unset(const resolution_lineage* rl) {
    offsets_.mutate(std::make_unique<backtrackable_map_erase<map_t>>(rl));
}

#endif
