#ifndef I_CDCL_SEQUENCER_HPP
#define I_CDCL_SEQUENCER_HPP

#include <cstddef>

struct i_cdcl_sequencer {
    virtual ~i_cdcl_sequencer() = default;
    virtual size_t next() = 0;
};

#endif
