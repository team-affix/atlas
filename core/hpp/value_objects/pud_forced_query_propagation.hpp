#ifndef PUD_FORCED_QUERY_PROPAGATION_HPP
#define PUD_FORCED_QUERY_PROPAGATION_HPP

#include <variant>
#include "value_objects/pud_refuted_query.hpp"
#include "value_objects/pud_unit_query.hpp"

using pud_forced_query_propagation = std::variant<pud_unit_query, pud_refuted_query>;

#endif
