#ifndef RULE_HPP
#define RULE_HPP

#include <vector>
#include "expr.hpp"

struct rule {
    const expr* head;
    std::vector<const expr*> body;
    auto operator<=>(const rule&) const = default;
};

#endif
