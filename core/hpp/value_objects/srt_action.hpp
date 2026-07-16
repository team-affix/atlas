#ifndef SRT_ACTION_HPP
#define SRT_ACTION_HPP

#include <variant>
#include "value_objects/srt_children_at_erase.hpp"
#include "value_objects/srt_children_at_insert.hpp"
#include "value_objects/srt_children_erase.hpp"
#include "value_objects/srt_children_insert.hpp"
#include "value_objects/srt_parent_assign.hpp"
#include "value_objects/srt_parent_erase.hpp"
#include "value_objects/srt_parent_insert.hpp"
#include "value_objects/srt_set_erase.hpp"
#include "value_objects/srt_set_insert.hpp"

template<typename NodeId>
using srt_action = std::variant<
    srt_set_insert<NodeId>,
    srt_set_erase<NodeId>,
    srt_children_insert<NodeId>,
    srt_children_erase<NodeId>,
    srt_parent_insert<NodeId>,
    srt_parent_erase<NodeId>,
    srt_parent_assign<NodeId>,
    srt_children_at_insert<NodeId>,
    srt_children_at_erase<NodeId>>;

#endif
