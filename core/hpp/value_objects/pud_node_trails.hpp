#ifndef PUD_NODE_TRAILS_HPP
#define PUD_NODE_TRAILS_HPP

#include <immer/box.hpp>
#include <immer/map.hpp>
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_goal_trails.hpp"

// A live leaf's trails per goal. Boxed, so a child shares every goal's trails it does not change.
using pud_node_trails = immer::map<pud_body_goal_id, immer::box<pud_goal_trails>>;

#endif
