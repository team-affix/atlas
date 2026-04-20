#ifndef SOLVER_ARGS_HPP
#define SOLVER_ARGS_HPP

#include <cstddef>
#include "defs.hpp"
#include "trail.hpp"
#include "sequencer.hpp"
#include "bind_map.hpp"

struct solver_args {
    const database& db;
    const goals&    gl;
    trail&          t;
    sequencer&      vars;
    bind_map&       bm;
    size_t          max_resolutions;
};

#endif
