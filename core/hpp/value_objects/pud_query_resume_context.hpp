#ifndef PUD_QUERY_RESUME_CONTEXT_HPP
#define PUD_QUERY_RESUME_CONTEXT_HPP

#include <compare>
#include <optional>
#include <vector>
#include "value_objects/pud_forced_unfold_reason.hpp"
#include "value_objects/pud_mhcs_head_id.hpp"

struct pud_query_resume_context {
    std::vector<pud_mhcs_head_id> candidate_ids;
    std::optional<pud_forced_unfold_reason> forced_unfold_reason;
    auto operator<=>(const pud_query_resume_context&) const = default;
};

#endif
