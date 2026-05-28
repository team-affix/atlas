#include "infrastructure/cdcl_sequencer.hpp"

cdcl_sequencer::cdcl_sequencer(i_log_to_current_trail_frame& t) :
    seq(t) {
}

size_t cdcl_sequencer::next() {
    return seq.next();
}
