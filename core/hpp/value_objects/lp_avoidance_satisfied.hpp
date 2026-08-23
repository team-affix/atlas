#ifndef LP_AVOIDANCE_SATISFIED_HPP
#define LP_AVOIDANCE_SATISFIED_HPP

#include <compare>

// "This avoidance can never be violated, drop it." Deliberately empty: the type
// name is the whole message, and the avoidance it refers to is the key it is
// stored under.
struct lp_avoidance_satisfied {
    auto operator<=>(const lp_avoidance_satisfied&) const = default;
};

#endif
