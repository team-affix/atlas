#ifndef CANDIDATE_FRAME_OFFSETS_HPP
#define CANDIDATE_FRAME_OFFSETS_HPP

#include <cstdint>
#include <unordered_map>
#include "interfaces/i_get_candidate_frame_offset.hpp"
#include "interfaces/i_set_candidate_frame_offset.hpp"
#include "interfaces/i_unset_candidate_frame_offset.hpp"
#include "interfaces/i_clear_candidate_frame_offsets.hpp"

struct candidate_frame_offsets
    : i_get_candidate_frame_offset
    , i_set_candidate_frame_offset
    , i_unset_candidate_frame_offset
    , i_clear_candidate_frame_offsets {
    uint32_t get(const resolution_lineage*) const override;
    void set(const resolution_lineage*, uint32_t frame_offset) override;
    void unset(const resolution_lineage*) override;
    void clear_candidate_frame_offsets() override;
private:
    std::unordered_map<const resolution_lineage*, uint32_t> offsets_;
};

#endif
