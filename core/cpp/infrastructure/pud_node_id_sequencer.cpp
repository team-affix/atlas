#include "infrastructure/pud_node_id_sequencer.hpp"

pud_node_id_sequencer::pud_node_id_sequencer()
    : next_(0) {}

pud_node_id pud_node_id_sequencer::next() {
    return next_++;
}
