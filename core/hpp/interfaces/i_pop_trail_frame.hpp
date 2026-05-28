#ifndef I_POP_TRAIL_FRAME_HPP
#define I_POP_TRAIL_FRAME_HPP

struct i_pop_trail_frame {
    virtual ~i_pop_trail_frame() = default;
    virtual void pop() = 0;
};

#endif
