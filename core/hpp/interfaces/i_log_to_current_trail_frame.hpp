#ifndef I_LOG_TO_CURRENT_TRAIL_FRAME_HPP
#define I_LOG_TO_CURRENT_TRAIL_FRAME_HPP

#include <memory>
#include "interfaces/i_backtrackable.hpp"

struct i_log_to_current_trail_frame {
    virtual ~i_log_to_current_trail_frame() = default;
    virtual void log(std::unique_ptr<i_backtrackable>) = 0;
};

#endif
