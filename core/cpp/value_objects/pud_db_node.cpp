#include "value_objects/pud_db_node.hpp"

std::strong_ordering pud_db_node::operator<=>(const pud_db_node& other) const {
    if (auto cmp = lvc <=> other.lvc; cmp != 0)
        return cmp;
    if (auto cmp = added_unifications <=> other.added_unifications; cmp != 0)
        return cmp;
    return added_body_goals <=> other.added_body_goals;
}

bool pud_db_node::operator==(const pud_db_node& other) const {
    return (*this <=> other) == std::strong_ordering::equal;
}
