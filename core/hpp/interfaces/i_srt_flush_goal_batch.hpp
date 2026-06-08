#ifndef I_SRT_FLUSH_GOAL_BATCH_HPP
#define I_SRT_FLUSH_GOAL_BATCH_HPP

struct i_srt_flush_goal_batch {
    virtual ~i_srt_flush_goal_batch() = default;
    virtual void flush_srt_goal_batch() = 0;
};

#endif
