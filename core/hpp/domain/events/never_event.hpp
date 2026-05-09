#ifndef NEVER_EVENT_HPP
#define NEVER_EVENT_HPP

#include <ostream>

// Sentinel event that is never emitted; used as the reset event for
// cancellable_event_handler instances that should stay cancelled permanently
// once cancelled (e.g. no restart after refutation).
struct never_event {
};

std::ostream& operator<<(std::ostream&, const never_event&);

#endif
