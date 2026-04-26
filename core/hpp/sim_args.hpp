#ifndef SIM_ARGS_HPP
#define SIM_ARGS_HPP

#include <cstddef>
#include "defs.hpp"
#include "trail.hpp"
#include "sequencer.hpp"
#include "expr.hpp"
#include "bind_map.hpp"
#include "lineage.hpp"
#include "cdcl.hpp"
#include "topic.hpp"

struct sim_args {
    size_t           max_resolutions;
    const database&  db;
    const goals&     gl;
    trail&           t;
    sequencer&       vars;
    expr_pool&       ep;
    bind_map&        bm;
    topic<uint32_t>& rep_changed_topic;
    lineage_pool&    lp;
    cdcl             c;
};

#endif
