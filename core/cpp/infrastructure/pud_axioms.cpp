#include "infrastructure/pud_axioms.hpp"
#include "debug_assert.hpp"

void pud_axioms::mark_axiom(pud_node_id id) {
    auto [_, inserted] = axiom_set_.insert(id);
    DEBUG_ASSERT(inserted);
    axioms_.push_back(id);
}

const std::vector<pud_node_id>& pud_axioms::get() const {
    return axioms_;
}
