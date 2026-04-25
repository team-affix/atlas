#ifndef CANDIDATE_EXPANDER_HPP
#define CANDIDATE_EXPANDER_HPP

#include "defs.hpp"
#include "expr.hpp"
#include "rule.hpp"
#include "copier.hpp"
#include "bind_map.hpp"

struct candidate_expander {
    candidate_expander(const std::unordered_set<size_t>&);
    std::unordered_set<size_t> operator()();
#ifndef DEBUG
private:
#endif
    const std::unordered_set<size_t>& initial_candidates;
};

#endif