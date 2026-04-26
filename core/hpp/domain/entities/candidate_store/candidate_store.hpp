#ifndef CANDIDATE_STORE_HPP
#define CANDIDATE_STORE_HPP

#include "frontier.hpp"
#include "candidate_expander.hpp"

struct candidate_store : frontier<std::unordered_set<size_t>, candidate_expander> {
    candidate_store();
    candidate_expander make_expander(const std::unordered_set<size_t>&, const rule&) override;
#ifndef DEBUG
private:
#endif
    std::unordered_set<size_t> initial_candidates;
};

#endif
