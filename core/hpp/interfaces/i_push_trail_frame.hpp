#ifndef I_PUSH_TRAIL_FRAME_HPP
#define I_PUSH_TRAIL_FRAME_HPP

struct i_push_trail_frame {
    virtual ~i_push_trail_frame() = default;
    virtual void push() = 0;
};

#endif
