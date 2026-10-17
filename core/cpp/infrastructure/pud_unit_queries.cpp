#include "infrastructure/pud_unit_queries.hpp"
#include "debug_assert.hpp"

pud_unit_queries::pud_unit_queries()
    : head_ids_() {}

bool pud_unit_queries::empty() const {
    return head_ids_.empty();
}

void pud_unit_queries::push(pud_mhq_head_id head_id) {
    head_ids_.push_back(head_id);
}

pud_mhq_head_id pud_unit_queries::front() const {
    DEBUG_ASSERT(!head_ids_.empty());
    return head_ids_.front();
}

void pud_unit_queries::pop() {
    DEBUG_ASSERT(!head_ids_.empty());
    head_ids_.pop_front();
}
