#ifndef SEQUENCER_HPP
#define SEQUENCER_HPP

#include <cstdint>
#include "trail.hpp"

struct sequencer {
    sequencer(trail&);
    uint32_t operator()();
#ifndef DEBUG
private:
#endif
    trail& trail_ref;
    uint32_t index;
};

#endif
