#ifndef VAR_SEQUENCER_CPP
#define VAR_SEQUENCER_CPP

#include "../../hpp/infrastructure/var_sequencer.hpp"
#include "../../hpp/bootstrap/locator.hpp"

var_sequencer::var_sequencer() :
    seq(locator::resolve<i_trail>()) {
}

uint32_t var_sequencer::next() {
    return seq.next();
}

#endif
