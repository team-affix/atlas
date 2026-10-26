#ifndef UBS_BIND_MAP_HPP
#define UBS_BIND_MAP_HPP

#include <cstdint>
#include <optional>
#include <immer/map.hpp>
#include <immer/map_transient.hpp>
#include "value_objects/framed_functor.hpp"
#include "value_objects/pud_ubs_state.hpp"
#include "value_objects/ubs_class.hpp"

// Union by size over the variables of one environment, edited in place on top of a persistent
// state. A class handed to equate or instantiate must come from find since the last edit.
struct ubs_bind_map {
    ubs_bind_map(const pud_ubs_state& state);
    ubs_class find(uint32_t var) const;
    // When both classes are bound, the merged class keeps one value and returns the other, which
    // the caller must unify with it.
    std::optional<framed_functor> equate(const ubs_class& class_a, const ubs_class& class_b);
    void instantiate(const ubs_class& unbound, framed_expr value);
    pud_ubs_state persist() &&;
private:
    immer::map<uint32_t, uint32_t>::transient_type parents_;
    immer::map<uint32_t, ubs_class>::transient_type classes_;
};

#endif
