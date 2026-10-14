#include "infrastructure/pud_transient_ubs_state.hpp"

pud_transient_ubs_state::pud_transient_ubs_state()
    : parents_(immer::map<uint32_t, uint32_t>{}.transient())
    , sizes_(immer::map<uint32_t, uint32_t>{}.transient())
    , values_(immer::map<uint32_t, const expr*>{}.transient()) {}

pud_transient_ubs_state::pud_transient_ubs_state(const pud_ubs_state& state)
    : parents_(state.parents.transient())
    , sizes_(state.sizes.transient())
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

const expr* const* pud_transient_ubs_state::get_value(uint32_t root) const {
    return values_.find(root);
}

void pud_transient_ubs_state::set_value(uint32_t root, const expr* value) {
    values_.set(root, value);
}

immer::map<uint32_t, uint32_t> pud_transient_ubs_state::persistent_parents() {
    return std::move(parents_).persistent();
}

immer::map<uint32_t, uint32_t> pud_transient_ubs_state::persistent_sizes() {
    return std::move(sizes_).persistent();
}

immer::map<uint32_t, const expr*> pud_transient_ubs_state::persistent_values() {
    return std::move(values_).persistent();
}

pud_ubs_state pud_transient_ubs_state::to_persistent_state() {
    pud_ubs_state state;
    state.parents = persistent_parents();
    state.sizes = persistent_sizes();
    state.values = persistent_values();
    return state;
}
