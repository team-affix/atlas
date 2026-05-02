#include "../../hpp/infrastructure/cdcl_sequencer.hpp"

cdcl_sequencer::cdcl_sequencer() : seq() {}

size_t cdcl_sequencer::next() {
    return seq.next();
}
