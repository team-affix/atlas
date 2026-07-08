#ifndef AVOIDANCE_VISITATION_HPP
#define AVOIDANCE_VISITATION_HPP

#include <cstddef>
#include "avoidance_id.hpp"

struct avoidance_visitation {
    avoidance_id id;
    size_t prev_watcher_a_pos;
    size_t prev_watcher_b_pos;
    size_t watcher_a_pos;
    size_t watcher_b_pos;
};

#endif
