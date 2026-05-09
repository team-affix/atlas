#ifndef I_SOLVED_DETECTOR_HPP
#define I_SOLVED_DETECTOR_HPP

struct i_solved_detector {
    virtual ~i_solved_detector() = default;
    virtual void detect_solved() = 0;
};

#endif
