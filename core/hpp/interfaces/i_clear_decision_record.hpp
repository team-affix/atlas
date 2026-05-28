#ifndef I_CLEAR_DECISION_RECORD_HPP
#define I_CLEAR_DECISION_RECORD_HPP

struct i_clear_decision_record {
    virtual ~i_clear_decision_record() = default;
    virtual void clear_decision_record() = 0;
};

#endif

