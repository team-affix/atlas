#include "../hpp/avoidance_adder.hpp"

avoidance_adder::avoidance_adder(
    avoidance_store& as,
    avoidance_map& am) : as(as), am(am) {
}

void avoidance_adder::operator()(const avoidance& av) {

    for (const auto& rl : av)
        am.insert({rl->parent, as.size()});

    as.push_back(av);
    
}
