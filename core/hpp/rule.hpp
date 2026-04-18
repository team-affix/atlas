#ifndef RULE_HPP
#define RULE_HPP

#include <vector>
#include "expr.hpp"

struct rule {
    const expr::pred* head;
    std::vector<const expr::pred*> body;
    auto operator<=>(const rule&) const = default;
};

#endif
