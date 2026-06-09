#include "infrastructure/set_up_sim.hpp"

set_up_sim::set_up_sim(locator& loc)
    : push_trail_frame_(loc.locate<i_push_trail_frame>()) {}

void set_up_sim::set_up() {
    push_trail_frame_.push();
}
