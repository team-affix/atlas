#ifndef PUD_TRANSIENT_UBS_STATE_HPP
#define PUD_TRANSIENT_UBS_STATE_HPP

#include <cstdint>
#include <immer/map.hpp>
#include <immer/map_transient.hpp>
#include "infrastructure/ubs_bind_map.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_ubs_state.hpp"

struct pud_transient_ubs_state {
    pud_transient_ubs_state();
    pud_transient_ubs_state(const pud_ubs_state& state);

    const uint32_t* get_parent(uint32_t var) const;
    void set_parent(uint32_t child, uint32_t parent);
    const uint32_t* get_size(uint32_t root) const;
    void set_size(uint32_t root, uint32_t class_size);
    const expr* const* get_value(uint32_t root) const;
    void set_value(uint32_t root, const expr* value);

    immer::map<uint32_t, uint32_t> persistent_parents();
    immer::map<uint32_t, uint32_t> persistent_sizes();
    immer::map<uint32_t, const expr*> persistent_values();

    pud_ubs_state to_persistent_state();

    template<typename IMakeVar, typename IMakeFunctor>
    ubs_bind_map<
        IMakeVar,
        IMakeFunctor,
        pud_transient_ubs_state,
        pud_transient_ubs_state,
        pud_transient_ubs_state,
        pud_transient_ubs_state,
        pud_transient_ubs_state,
        pud_transient_ubs_state>
    make_bind_map(IMakeVar& make_var, IMakeFunctor& make_functor);

private:
    immer::map<uint32_t, uint32_t>::transient_type parents_;
    immer::map<uint32_t, uint32_t>::transient_type sizes_;
    immer::map<uint32_t, const expr*>::transient_type values_;
};

template<typename IMakeVar, typename IMakeFunctor>
ubs_bind_map<
    IMakeVar,
    IMakeFunctor,
    pud_transient_ubs_state,
    pud_transient_ubs_state,
    pud_transient_ubs_state,
    pud_transient_ubs_state,
    pud_transient_ubs_state,
    pud_transient_ubs_state>
pud_transient_ubs_state::make_bind_map(IMakeVar& make_var, IMakeFunctor& make_functor) {
    return ubs_bind_map<
        IMakeVar,
        IMakeFunctor,
        pud_transient_ubs_state,
        pud_transient_ubs_state,
        pud_transient_ubs_state,
        pud_transient_ubs_state,
        pud_transient_ubs_state,
        pud_transient_ubs_state>{
        make_var,
        make_functor,
        *this,
        *this,
        *this,
        *this,
        *this,
        *this};
}

#endif
