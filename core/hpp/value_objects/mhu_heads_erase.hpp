#ifndef MHU_HEADS_ERASE_HPP
#define MHU_HEADS_ERASE_HPP

#include <compare>
#include "value_objects/lineage.hpp"
#include "value_objects/unify_head.hpp"

template<typename IBindMap, typename IUnifier>
struct mhu_heads_erase {
    const resolution_lineage* rl;
    unify_head<IBindMap, IUnifier>* head;
    auto operator<=>(const mhu_heads_erase&) const = default;
};

#endif
