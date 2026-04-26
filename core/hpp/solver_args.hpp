#ifndef SOLVER_ARGS_HPP
#define SOLVER_ARGS_HPP

#include <cstddef>
#include "defs.hpp"
#include "trail.hpp"
#include "sequencer.hpp"
#include "bind_map.hpp"
#include "topic.hpp"

struct solver_args {
    const database& db;
    const goals&    gl;
    trail&          t;
    sequencer&      vars;
    bind_map&       bm;
    topic<uint32_t>& rep_changed_topic;
    size_t          max_resolutions;
};

#endif
