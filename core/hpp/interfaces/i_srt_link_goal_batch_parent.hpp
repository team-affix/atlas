#ifndef I_SRT_LINK_GOAL_BATCH_PARENT_HPP
#define I_SRT_LINK_GOAL_BATCH_PARENT_HPP

#include "value_objects/lineage.hpp"

struct i_srt_link_goal_batch_parent {
    virtual ~i_srt_link_goal_batch_parent() = default;
    virtual void link_srt_goal_batch_parent(const goal_lineage*) = 0;
};

#endif
