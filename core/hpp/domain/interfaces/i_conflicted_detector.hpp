#ifndef I_CONFLICTED_DETECTOR_HPP
#define I_CONFLICTED_DETECTOR_HPP

struct i_conflicted_detector {
    virtual ~i_conflicted_detector() = default;
    virtual void candidates_empty() = 0;
    virtual void avoidance_empty() = 0;
};

#endif
