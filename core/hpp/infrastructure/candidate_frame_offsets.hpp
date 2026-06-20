#ifndef CANDIDATE_FRAME_OFFSETS_HPP
#define CANDIDATE_FRAME_OFFSETS_HPP

#include <cstdint>
#include <unordered_map>
#include "value_objects/lineage.hpp"
#include "debug_assert.hpp"

struct candidate_frame_offsets {
    uint32_t get(const resolution_lineage*) const;
    void set(const resolution_lineage*, uint32_t frame_offset);
    void unset(const resolution_lineage*);
    void clear_candidate_frame_offsets();
private:
    std::unordered_map<const resolution_lineage*, uint32_t> offsets_;
};

inline uint32_t candidate_frame_offsets::get(const resolution_lineage* rl) const {
    return offsets_.at(rl);
}

inline void candidate_frame_offsets::set(const resolution_lineage* rl, uint32_t frame_offset) {
    auto [_, inserted] = offsets_.insert({rl, frame_offset});
    DEBUG_ASSERT(inserted);
}

inline void candidate_frame_offsets::unset(const resolution_lineage* rl) {
    auto erased = offsets_.erase(rl);
    DEBUG_ASSERT(erased == 1);
}

inline void candidate_frame_offsets::clear_candidate_frame_offsets() {
    offsets_.clear();
}

#endif
