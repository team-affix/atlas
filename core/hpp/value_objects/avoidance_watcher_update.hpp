#ifndef AVOIDANCE_WATCHER_UPDATE_HPP
#define AVOIDANCE_WATCHER_UPDATE_HPP

#include <compare>
#include <cstddef>
#include "value_objects/avoidance_id.hpp"

struct avoidance_watcher_update {
    avoidance_id id;
    bool watcher_a_fired;
    size_t prev_watcher_pos;
    auto operator<=>(const avoidance_watcher_update&) const = default;
};

#endif
