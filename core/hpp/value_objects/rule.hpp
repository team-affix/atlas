#ifndef RULE_HPP
#define RULE_HPP

#include <vector>
#include "expr.hpp"

struct rule {
    const expr* head;
    std::vector<const expr*> body;
    uint32_t var_count = 0;
    auto operator<=>(const rule&) const = default;
};

#endif
