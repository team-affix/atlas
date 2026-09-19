#include "value_objects/pud_query.hpp"

std::strong_ordering pud_query::operator<=>(const pud_query& other) const {
    if (auto cmp = axiom_contexts <=> other.axiom_contexts; cmp != 0)
        return cmp;
    if (auto cmp = body_goal <=> other.body_goal; cmp != 0)
        return cmp;
    if (auto cmp = frame_offset <=> other.frame_offset; cmp != 0)
        return cmp;
    if (interval.open < other.interval.open)
        return std::strong_ordering::less;
    if (other.interval.open < interval.open)
        return std::strong_ordering::greater;
    if (interval.close < other.interval.close)
        return std::strong_ordering::less;
    if (other.interval.close < interval.close)
        return std::strong_ordering::greater;
    return std::strong_ordering::equal;
}

bool pud_query::operator==(const pud_query& other) const {
    return (*this <=> other) == std::strong_ordering::equal;
}
