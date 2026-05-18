#ifndef ELIMINATION_RESULT_HPP
#define ELIMINATION_RESULT_HPP

#include <variant>
#include "../value_objects/lineage.hpp"

struct elimination_result {
    struct eliminated {};
    struct goal_candidates_empty {const goal_lineage* gl;};
    struct goal_made_unit {const goal_lineage* gl;};
    struct already_eliminated {};
    struct added_to_backlog {};
    std::variant<eliminated, goal_candidates_empty, goal_made_unit, already_eliminated, added_to_backlog> content;
};

#endif
