#include "../hpp/sequencer.hpp"
#include "../hpp/locator.hpp"

sequencer::sequencer() :
    trail_ref(locator::locate<trail>(locator_keys::inst_trail)), index(0) {
}

uint32_t sequencer::operator()() {
    trail_ref.log([this]{--index;});
    return index++;
}
