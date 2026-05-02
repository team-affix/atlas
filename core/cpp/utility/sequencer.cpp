#include "../../hpp/utility/sequencer.hpp"
#include "../../hpp/utility/backtrackable_increment.hpp"

sequencer::sequencer(i_trail& t) :
    index(t, 0) {
}

size_t sequencer::next() {
    size_t result = index.get();
    index.mutate(std::make_unique<backtrackable_increment<size_t>>());
    return result;
}
