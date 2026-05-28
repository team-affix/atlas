#ifndef VAR_SEQUENCER_CPP
#define VAR_SEQUENCER_CPP

#include "infrastructure/var_sequencer.hpp"

var_sequencer::var_sequencer(locator& loc) :
    seq(loc.locate<i_log_to_current_trail_frame>()) {
}

uint32_t var_sequencer::next() {
    return seq.next();
}

#endif
