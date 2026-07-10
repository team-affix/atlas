#ifndef MHU_BIND_MAP_JOURNAL_HPP
#define MHU_BIND_MAP_JOURNAL_HPP

#include <compare>
#include "value_objects/bind_map_action.hpp"

template<typename IBindMap>
struct mhu_bind_map_journal {
    IBindMap* bind_map = nullptr;
    bind_map_action action;

    auto operator<=>(const mhu_bind_map_journal&) const = default;
};

#endif
