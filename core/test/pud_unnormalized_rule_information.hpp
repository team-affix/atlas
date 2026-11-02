#ifndef PUD_UNNORMALIZED_RULE_INFORMATION_HPP
#define PUD_UNNORMALIZED_RULE_INFORMATION_HPP

#include <immer/map.hpp>
#include "value_objects/expr.hpp"
#include "value_objects/pud_body_goal_id.hpp"
#include "infrastructure/ubs_bind_map.hpp"

struct pud_unnormalized_rule_information {
    const expr* head;
    immer::map<pud_body_goal_id, const expr*> body;
    ubs_bind_map bindings;
};

#endif
