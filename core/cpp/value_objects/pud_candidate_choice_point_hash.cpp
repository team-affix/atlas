#include "value_objects/pud_candidate_choice_point_hash.hpp"

#include <functional>

size_t pud_candidate_choice_point_hash::operator()(const pud_candidate_choice_point& context) const noexcept {
    size_t seed = std::hash<pud_mhws_head_id>{}(context.witness_a);
    const size_t value = std::hash<pud_mhws_head_id>{}(context.witness_b);
    return seed ^ (value + 0x9e3779b9 + (seed << 6) + (seed >> 2));
}
