#ifndef DECIDER_HPP
#define DECIDER_HPP

#include "../interfaces/i_decider.hpp"
#include "../interfaces/i_decision_generator.hpp"
#include "../interfaces/i_event_producer.hpp"
#include "../events/deciding_event.hpp"

struct decider : i_decider {
    decider();
    void decide() const override;
private:
    i_decision_generator& decision_generator;
    i_event_producer<deciding_event>& deciding_producer;
};

#endif
