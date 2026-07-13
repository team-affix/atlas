#ifndef RULE_HPP
#define RULE_HPP

#include <compare>
#include <vector>
#include "expr.hpp"

struct rule {
    rule(const expr* head, std::vector<const expr*> body);
    rule(const expr* head, std::vector<const expr*> body, uint32_t var_count);
    const expr* head;
    std::vector<const expr*> body;
    uint32_t var_count;
    auto operator<=>(const rule&) const = default;
};

#endif
