#include "infrastructure/cdcl_sequencer.hpp"

cdcl_sequencer::cdcl_sequencer(i_trail& t) :
    seq(t) {
}

size_t cdcl_sequencer::next() {
    return seq.next();
}
