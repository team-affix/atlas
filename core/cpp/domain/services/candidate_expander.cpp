#include "../../../hpp/domain/data_structures/candidate_expander.hpp"

candidate_expander::candidate_expander(const std::unordered_set<size_t>& initial_candidates)
    : initial_candidates(initial_candidates) {}

std::unordered_set<size_t> candidate_expander::operator()() {
    return initial_candidates;
}
