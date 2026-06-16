#include "debug_assert.hpp"
#include "infrastructure/candidate_frame_offsets.hpp"

uint32_t candidate_frame_offsets::get(const resolution_lineage* rl) const {
    return offsets_.at(rl);
}

void candidate_frame_offsets::set(const resolution_lineage* rl, uint32_t frame_offset) {
    auto [_, inserted] = offsets_.insert({rl, frame_offset});
    DEBUG_ASSERT(inserted);
}

void candidate_frame_offsets::unset(const resolution_lineage* rl) {
    auto erased = offsets_.erase(rl);
    DEBUG_ASSERT(erased == 1);
}

void candidate_frame_offsets::clear_candidate_frame_offsets() {
    offsets_.clear();
}
