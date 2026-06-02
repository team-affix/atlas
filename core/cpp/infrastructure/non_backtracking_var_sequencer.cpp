#include "infrastructure/non_backtracking_var_sequencer.hpp"

non_backtracking_var_sequencer::non_backtracking_var_sequencer(uint32_t initial)
    : counter_(initial) {}

uint32_t non_backtracking_var_sequencer::next() {
    return counter_++;
}

uint32_t non_backtracking_var_sequencer::peek() const {
    return counter_;
}
