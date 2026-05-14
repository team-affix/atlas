#include "../../hpp/infrastructure/rep_change_sink.hpp"

void rep_change_sink::push(uint32_t v) {
    q_.push(v);
}

uint32_t rep_change_sink::pop() {
    uint32_t v = q_.front();
    q_.pop();
    return v;
}

bool rep_change_sink::empty() const {
    return q_.empty();
}
