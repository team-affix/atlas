#include "infrastructure/pud_axioms.hpp"
#include "debug_assert.hpp"

bool pud_axioms::check_axiom(pud_node_id id) const {
    return axiom_set_.contains(id);
}

void pud_axioms::mark_axiom(pud_node_id id) {
    auto [_, inserted] = axiom_set_.insert(id);
    DEBUG_ASSERT(inserted);
    axioms_.push_back(id);
}

const std::vector<pud_node_id>& pud_axioms::get() const {
    return axioms_;
}
