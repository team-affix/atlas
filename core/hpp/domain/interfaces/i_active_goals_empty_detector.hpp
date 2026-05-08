#ifndef I_ACTIVE_GOALS_EMPTY_DETECTOR_HPP
#define I_ACTIVE_GOALS_EMPTY_DETECTOR_HPP

struct i_active_goals_empty_detector {
    virtual ~i_active_goals_empty_detector() = default;
    virtual void goal_deactivated() = 0;
};

#endif
