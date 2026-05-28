#ifndef VAR_SEQUENCER_CPP
#define VAR_SEQUENCER_CPP

#include "infrastructure/var_sequencer.hpp"

var_sequencer::var_sequencer(i_trail& t) :
    seq(t) {
}

uint32_t var_sequencer::next() {
    return seq.next();
}

#endif
