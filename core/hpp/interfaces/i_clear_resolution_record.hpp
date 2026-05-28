#ifndef I_CLEAR_RESOLUTION_RECORD_HPP
#define I_CLEAR_RESOLUTION_RECORD_HPP

struct i_clear_resolution_record {
    virtual ~i_clear_resolution_record() = default;
    virtual void clear_resolution_record() = 0;
};

#endif

