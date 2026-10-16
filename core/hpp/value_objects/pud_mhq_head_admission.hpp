#ifndef PUD_MHQ_HEAD_ADMISSION_HPP
#define PUD_MHQ_HEAD_ADMISSION_HPP

#include <compare>
#include <optional>
#include "value_objects/pud_forced_query_propagation.hpp"
#include "value_objects/pud_mhq_head_id.hpp"

struct pud_mhq_head_admission {
    pud_mhq_head_id head_id;
    std::optional<pud_forced_query_propagation> forced_propagation;
    auto operator<=>(const pud_mhq_head_admission&) const = default;
};

#endif
