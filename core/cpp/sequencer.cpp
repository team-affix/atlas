#include "../hpp/sequencer.hpp"

sequencer::sequencer(trail& trail_ref)
    : trail_ref(trail_ref), index(0) {

}

uint32_t sequencer::operator()() {
    trail_ref.log([this]{--index;});
    return index++;
}
