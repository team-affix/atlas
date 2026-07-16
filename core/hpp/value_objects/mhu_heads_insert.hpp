#ifndef MHU_HEADS_INSERT_HPP
#define MHU_HEADS_INSERT_HPP

#include <compare>
#include "value_objects/lineage.hpp"
#include "value_objects/unify_head.hpp"

template<typename IBindMap, typename IUnifier>
struct mhu_heads_insert {
    const resolution_lineage* rl;
    unify_head<IBindMap, IUnifier>* head;
    auto operator<=>(const mhu_heads_insert&) const = default;
};

#endif
