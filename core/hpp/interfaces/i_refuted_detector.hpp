#ifndef I_REFUTED_DETECTOR_HPP
#define I_REFUTED_DETECTOR_HPP

struct i_refuted_detector {
    virtual ~i_refuted_detector() = default;
    virtual void conflicted() = 0;
};

#endif
