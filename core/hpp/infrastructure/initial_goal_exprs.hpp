#ifndef INITIAL_GOAL_EXPRS_HPP
#define INITIAL_GOAL_EXPRS_HPP

#include <vector>
#include "interfaces/i_push_initial_goal_expr.hpp"
#include "value_objects/expr.hpp"

struct initial_goal_exprs : i_push_initial_goal_expr {
    void push(const expr*) override;
    size_t count() const;
    const expr* get(size_t idx) const;
private:
    std::vector<const expr*> exprs_;
};

inline void initial_goal_exprs::push(const expr* e) {
    exprs_.push_back(e);
}

inline size_t initial_goal_exprs::count() const {
    return exprs_.size();
}

inline const expr* initial_goal_exprs::get(size_t idx) const {
    return exprs_.at(idx);
}

#endif
