#ifndef GOAL_EXPRS_HPP
#define GOAL_EXPRS_HPP

#include <unordered_map>
#include "value_objects/lineage.hpp"
#include "value_objects/framed_expr.hpp"
#include "debug_assert.hpp"

struct goal_exprs {
    framed_expr get(const goal_lineage*) const;
    void set(const goal_lineage*, framed_expr);
    void unset(const goal_lineage*);
    void clear_goal_exprs();
private:
    std::unordered_map<const goal_lineage*, framed_expr> exprs_;
};

inline framed_expr goal_exprs::get(const goal_lineage* gl) const {
    return exprs_.at(gl);
}

inline void goal_exprs::set(const goal_lineage* gl, framed_expr fe) {
    auto [_, inserted] = exprs_.insert({gl, fe});
    DEBUG_ASSERT(inserted);
}

inline void goal_exprs::unset(const goal_lineage* gl) {
    auto erased = exprs_.erase(gl);
    DEBUG_ASSERT(erased == 1);
}

inline void goal_exprs::clear_goal_exprs() {
    exprs_.clear();
}

#endif
