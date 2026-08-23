#ifndef LP_AVOIDANCE_UPDATE_HPP
#define LP_AVOIDANCE_UPDATE_HPP

#include <variant>
#include "value_objects/lp_avoidance_content_update.hpp"
#include "value_objects/lp_avoidance_satisfied.hpp"

// Everything one decision frame ever tells another about an avoidance: either
// its current members, or that it is gone.
using lp_avoidance_update = std::variant<
    lp_avoidance_content_update,
    lp_avoidance_satisfied>;

#endif
