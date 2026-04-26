#ifndef SOLVER_HPP
#define SOLVER_HPP

#include <memory>
#include <optional>
#include "defs.hpp"
#include "trail.hpp"
#include "expr.hpp"
#include "bind_map.hpp"
#include "lineage.hpp"
#include "sequencer.hpp"
#include "cdcl.hpp"
#include "sim.hpp"

struct solver {
    solver();
    virtual ~solver();
    bool operator()(std::optional<resolutions>&);
#ifndef DEBUG
protected:
#endif
    virtual std::unique_ptr<sim> construct_sim() = 0;
    virtual void terminate(sim&) = 0;

    const database& db;
    const goals& gl;
    trail& t;
    sequencer& vars;
    bind_map& bm;

    expr_pool ep;
    lineage_pool lp;

    size_t max_resolutions;
    cdcl c;

    std::unique_ptr<sim> managed_sim;
};

#endif
