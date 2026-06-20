#ifndef INITIAL_GOAL_EXPRS_HPP
#define INITIAL_GOAL_EXPRS_HPP

#include <vector>
#include "value_objects/expr.hpp"

struct initial_goal_exprs {
    void push(const expr*);
    size_t count() const;
    const expr* get(size_t idx) const;
private:
    std::vector<const expr*> exprs_;
};

#endif
