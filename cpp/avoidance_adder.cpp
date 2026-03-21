#include "../hpp/avoidance_adder.hpp"

avoidance_adder::avoidance_adder(
    avoidance_store& as,
    avoidance_map& am) : as(as), am(am) {
}

void avoidance_adder::operator()(const avoidance& avoidance) {

    auto [it, inserted] = as.insert(avoidance);

    if (!inserted)
        return;
    
    for (const auto& rl : avoidance)
        am.insert({rl->parent, avoidance});

}
