#include "../hpp/sequencer.hpp"

sequencer::sequencer(trail& t)
    : index(t, 0) {

}

uint32_t sequencer::operator()() {
    uint32_t result = index.get();
    index.mutate(
        [](uint32_t& index) { ++index; },
        [](uint32_t& index) { --index; }
    );
    return result;
}
