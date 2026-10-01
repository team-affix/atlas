#include "value_objects/pud_candidate_self_witness_hash.hpp"

#include <functional>

size_t pud_candidate_self_witness_hash::operator()(const pud_candidate_self_witness& context) const noexcept {
    return std::hash<const pud_node*>{}(context.node);
}
