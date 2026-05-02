#include "../../hpp/infrastructure/cdcl_sequencer.hpp"
#include "../../hpp/bootstrap/resolver.hpp"

cdcl_sequencer::cdcl_sequencer() :
    seq(resolver::resolve<i_trail>()) {
}

size_t cdcl_sequencer::next() {
    return seq.next();
}
