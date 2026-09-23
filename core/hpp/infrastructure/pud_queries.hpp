#ifndef PUD_QUERIES_HPP
#define PUD_QUERIES_HPP

#include <algorithm>
#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include "value_objects/expr.hpp"
#include "value_objects/pud_candidate_search_context.hpp"
#include "value_objects/pud_forced_unfold.hpp"
#include "value_objects/pud_rule_id.hpp"
#include "value_objects/pud_unfold_site.hpp"
#include "value_objects/pud_witness_search_context.hpp"
#include "debug_assert.hpp"

template<typename IGetAddedBodyGoals,
         typename IGetLvc,
         typename IResumeCandidateSearch,
         typename IResumeWitnessSearch,
         typename IStartQuery,
         typename IRebaseCandidate>
struct pud_queries {
    pud_queries();
    
};

#endif
