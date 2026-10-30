#include "infrastructure/pud_solution_store.hpp"
#include "debug_assert.hpp"

pud_solution_store::pud_solution_store()
    : solution_(nullptr)
    , solution_depth_(0)
    , solution_count_(0) {}

void pud_solution_store::set(const expr* solution, size_t solution_depth) {
    DEBUG_ASSERT(solution != nullptr);
    solution_ = solution;
    solution_depth_ = solution_depth;
    ++solution_count_;
}

void pud_solution_store::clear() {
    solution_ = nullptr;
    solution_depth_ = 0;
}

const expr* pud_solution_store::solution() const {
    return solution_;
}

size_t pud_solution_store::solution_depth() const {
    return solution_depth_;
}

size_t pud_solution_store::solution_count() const {
    return solution_count_;
}
