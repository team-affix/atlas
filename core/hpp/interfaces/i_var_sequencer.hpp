#ifndef I_VAR_SEQUENCER_HPP
#define I_VAR_SEQUENCER_HPP

#include <cstdint>

struct i_var_sequencer {
    virtual ~i_var_sequencer() = default;
    virtual uint32_t next() = 0;
};

#endif
