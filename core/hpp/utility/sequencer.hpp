#ifndef SEQUENCER_HPP
#define SEQUENCER_HPP

#include <cstddef>
#include "../utility/tracked.hpp"

struct sequencer {
    sequencer(i_trail& t);
    size_t next();
#ifndef DEBUG
private:
#endif
    tracked<size_t> index;
};

#endif
