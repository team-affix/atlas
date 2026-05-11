#ifndef RESOLVER_YIELDED_EVENT_HPP
#define RESOLVER_YIELDED_EVENT_HPP

#include <ostream>

struct resolve_yielded_event {};

std::ostream& operator<<(std::ostream&, const resolve_yielded_event&);

#endif
