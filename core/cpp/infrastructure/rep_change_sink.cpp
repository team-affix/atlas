#include "../../hpp/infrastructure/rep_change_sink.hpp"

void rep_change_sink::push(uint32_t v) {
    q_.push(v);
}

void rep_change_sink::accept(i_visitor<uint32_t>& visitor) {
    while (!q_.empty()) {
        visitor.visit(q_.front());
        q_.pop();
    }
}
