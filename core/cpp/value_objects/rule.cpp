#include "value_objects/rule.hpp"

rule::rule(const expr* head, std::vector<const expr*> body, uint32_t var_count)
    : head(head), body(std::move(body)), var_count(var_count) {}
