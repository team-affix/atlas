#include "../../hpp/infrastructure/cdcl_sequencer.hpp"
#include "../../hpp/bootstrap/locator.hpp"

cdcl_sequencer::cdcl_sequencer() :
    seq(locator::resolve<i_trail>()) {
}

size_t cdcl_sequencer::next() {
    return seq.next();
}
