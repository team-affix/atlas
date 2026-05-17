#ifndef GOAL_HPP
#define GOAL_HPP

#include "expr.hpp"

struct goal {
    virtual ~goal() = default;
    const expr* e;
};

#endif
