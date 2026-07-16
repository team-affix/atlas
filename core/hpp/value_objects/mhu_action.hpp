#ifndef MHU_ACTION_HPP
#define MHU_ACTION_HPP

#include <variant>
#include "value_objects/mhu_arena_emplace.hpp"
#include "value_objects/mhu_heads_erase.hpp"
#include "value_objects/mhu_heads_insert.hpp"
#include "value_objects/mhu_rep_at_erase.hpp"
#include "value_objects/mhu_rep_at_insert.hpp"
#include "value_objects/mhu_rep_map_erase.hpp"
#include "value_objects/mhu_rep_map_insert.hpp"
#include "value_objects/mhu_rl_at_erase.hpp"
#include "value_objects/mhu_rl_at_insert.hpp"
#include "value_objects/mhu_rl_map_erase.hpp"
#include "value_objects/mhu_rl_map_insert.hpp"

template<typename IBindMap, typename IUnifier>
using mhu_action = std::variant<
    mhu_arena_emplace,
    mhu_heads_insert<IBindMap, IUnifier>,
    mhu_heads_erase<IBindMap, IUnifier>,
    mhu_rep_map_insert,
    mhu_rep_map_erase,
    mhu_rep_at_insert,
    mhu_rep_at_erase,
    mhu_rl_map_insert,
    mhu_rl_map_erase,
    mhu_rl_at_insert,
    mhu_rl_at_erase>;

#endif
