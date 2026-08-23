#ifndef LP_TIMESTAMP_HPP
#define LP_TIMESTAMP_HPP

#include <cstddef>

// Orders the changes a decision frame has made to its avoidances, so a child
// can ask for everything that moved since it last visited.
using lp_timestamp = size_t;

#endif
