#ifndef SET_UP_SIM_HPP
#define SET_UP_SIM_HPP

#include "infrastructure/locator.hpp"
#include "interfaces/i_set_up_sim.hpp"
#include "interfaces/i_push_trail_frame.hpp"

struct set_up_sim : i_set_up_sim {
    set_up_sim(locator& loc);
    void set_up() override;
private:
    i_push_trail_frame& push_trail_frame_;
};

#endif
