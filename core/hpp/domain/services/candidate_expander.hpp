#ifndef CANDIDATE_EXPANDER_HPP
#define CANDIDATE_EXPANDER_HPP

#include <unordered_set>

struct candidate_expander {
    candidate_expander(const std::unordered_set<size_t>&);
    std::unordered_set<size_t> operator()();
#ifndef DEBUG
private:
#endif
    const std::unordered_set<size_t>& initial_candidates;
};

#endif
