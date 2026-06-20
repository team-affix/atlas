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

#endif
