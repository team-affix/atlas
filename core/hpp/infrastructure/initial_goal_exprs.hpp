#ifndef INITIAL_GOAL_EXPRS_HPP
#define INITIAL_GOAL_EXPRS_HPP

#include <vector>
#include "../interfaces/i_initial_goal_exprs.hpp"

struct initial_goal_exprs : i_initial_goal_exprs {
    void push(const expr*);
    const expr* at(size_t i) const override;
    size_t size() const override;
private:
    std::vector<const expr*> exprs_;
};

#endif
