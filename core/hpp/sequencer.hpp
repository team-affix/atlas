#ifndef SEQUENCER_HPP
#define SEQUENCER_HPP

#include <cstdint>
#include "trail.hpp"
#include "tracked.hpp"

struct sequencer {
    sequencer(trail&);
    uint32_t operator()();
#ifndef DEBUG
private:
#endif
    tracked<uint32_t> index;
};

#endif
