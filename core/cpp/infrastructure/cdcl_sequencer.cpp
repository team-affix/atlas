#include "infrastructure/cdcl_sequencer.hpp"

cdcl_sequencer::cdcl_sequencer(locator& loc) :
    seq(loc.locate<i_log_to_current_trail_frame>(), 0) {
}

size_t cdcl_sequencer::next() {
    return seq.next();
}
