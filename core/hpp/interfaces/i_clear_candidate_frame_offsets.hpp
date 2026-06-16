#ifndef I_CLEAR_CANDIDATE_FRAME_OFFSETS_HPP
#define I_CLEAR_CANDIDATE_FRAME_OFFSETS_HPP

struct i_clear_candidate_frame_offsets {
    virtual ~i_clear_candidate_frame_offsets() = default;
    virtual void clear_candidate_frame_offsets() = 0;
};

#endif
