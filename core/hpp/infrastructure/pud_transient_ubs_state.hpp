#ifndef PUD_TRANSIENT_UBS_STATE_HPP
#define PUD_TRANSIENT_UBS_STATE_HPP

#include <cstdint>
#include <immer/map.hpp>
#include <immer/map_transient.hpp>
#include "infrastructure/ubs_bind_map.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/pud_ubs_state.hpp"

struct pud_transient_ubs_state {
    pud_transient_ubs_state(const pud_ubs_state& state);

    const uint32_t* get_parent(uint32_t var) const;
    void set_parent(uint32_t child, uint32_t parent);
    const uint32_t* get_size(uint32_t root) const;
    void set_size(uint32_t root, uint32_t class_size);
    const uint32_t* get_rep(uint32_t root) const;
    void set_rep(uint32_t root, uint32_t rep);
    const framed_expr* get_value(uint32_t root) const;
    void set_value(uint32_t root, framed_expr value);

    ubs_bind_map<
        pud_transient_ubs_state,
        pud_transient_ubs_state,
        pud_transient_ubs_state,
        pud_transient_ubs_state,
        pud_transient_ubs_state,
        pud_transient_ubs_state,
        pud_transient_ubs_state,
        pud_transient_ubs_state> make_bind_map();
    pud_ubs_state to_persistent_state();

private:
    immer::map<uint32_t, uint32_t>::transient_type parents_;
    immer::map<uint32_t, uint32_t>::transient_type sizes_;
    immer::map<uint32_t, uint32_t>::transient_type reps_;
    immer::map<uint32_t, framed_expr>::transient_type values_;
};

#endif
