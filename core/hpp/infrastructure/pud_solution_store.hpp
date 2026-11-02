#ifndef PUD_SOLUTION_STORE_HPP
#define PUD_SOLUTION_STORE_HPP

#include <cstddef>
#include "value_objects/expr.hpp"

struct pud_solution_store {
    pud_solution_store();
    void set(const expr* solution, size_t solution_depth);
    void clear();
    const expr* solution() const;
    size_t solution_depth() const;
    size_t solution_count() const;
private:
    const expr* solution_;
    size_t solution_depth_;
    size_t solution_count_;
};

#endif
