#ifndef I_SOLUTION_DETECTOR_HPP
#define I_SOLUTION_DETECTOR_HPP

struct i_solution_detector {
    virtual ~i_solution_detector() = default;
    virtual bool detect() const = 0;
};

#endif
