#ifndef AVOIDANCE_ACTION_HPP
#define AVOIDANCE_ACTION_HPP

#include <variant>
#include "value_objects/avoidance_watcher_update.hpp"
#include "value_objects/avoidance_unwatch.hpp"

using avoidance_action = std::variant<avoidance_watcher_update, avoidance_unwatch>;

#endif
