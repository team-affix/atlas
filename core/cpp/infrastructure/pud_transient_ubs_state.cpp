#include "infrastructure/pud_transient_ubs_state.hpp"

pud_transient_ubs_state::pud_transient_ubs_state(const pud_ubs_state& state)
    : parents_(state.parents.transient())
    , sizes_(state.sizes.transient())
    , reps_(state.reps.transient())
    , values_(state.values.transient()) {}

const uint32_t* pud_transient_ubs_state::get_parent(uint32_t var) const {
    return parents_.find(var);
}

void pud_transient_ubs_state::set_parent(uint32_t child, uint32_t parent) {
    parents_.set(child, parent);
}

const uint32_t* pud_transient_ubs_state::get_size(uint32_t root) const {
    return sizes_.find(root);
}

void pud_transient_ubs_state::set_size(uint32_t root, uint32_t class_size) {
    sizes_.set(root, class_size);
}

const uint32_t* pud_transient_ubs_state::get_rep(uint32_t root) const {
    return reps_.find(root);
}

void pud_transient_ubs_state::set_rep(uint32_t root, uint32_t rep) {
    reps_.set(root, rep);
}

const framed_expr* pud_transient_ubs_state::get_value(uint32_t root) const {
    return values_.find(root);
}

void pud_transient_ubs_state::set_value(uint32_t root, framed_expr value) {
    values_.set(root, value);
}

ubs_bind_map<
    pud_transient_ubs_state,
    pud_transient_ubs_state,
    pud_transient_ubs_state,
    pud_transient_ubs_state,
    pud_transient_ubs_state,
    pud_transient_ubs_state,
    pud_transient_ubs_state,
    pud_transient_ubs_state>
pud_transient_ubs_state::make_bind_map() {
    return {*this, *this, *this, *this, *this, *this, *this, *this};
}

pud_ubs_state pud_transient_ubs_state::to_persistent_state() {
    return pud_ubs_state{
        .parents = std::move(parents_).persistent(),
        .sizes = std::move(sizes_).persistent(),
        .reps = std::move(reps_).persistent(),
        .values = std::move(values_).persistent(),
    };
}
