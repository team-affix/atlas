#ifndef VAR_SEQUENCER_HPP
#define VAR_SEQUENCER_HPP

#include "infrastructure/locator.hpp"
#include "interfaces/i_var_sequencer.hpp"
#include "interfaces/i_log_to_current_trail_frame.hpp"
#include "infrastructure/sequencer.hpp"

struct var_sequencer : i_var_sequencer {
    var_sequencer(locator& loc);
    uint32_t next() override;
private:
    sequencer<uint32_t> seq;
};

#endif
