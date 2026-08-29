#ifndef FGT_AVOIDANCE_ACTION_HPP
#define FGT_AVOIDANCE_ACTION_HPP

#include <variant>
#include "value_objects/avoidance_watcher_update.hpp"
#include "value_objects/avoidance_unwatch.hpp"
#include "value_objects/avoidance_made_unevictable.hpp"

using fgt_avoidance_action = std::variant<avoidance_watcher_update, avoidance_unwatch, avoidance_made_unevictable>;

#endif
