#ifndef SIM_HPP
#define SIM_HPP

#include "defs.hpp"
#include "sequencer.hpp"
#include "bind_map.hpp"
#include "goal_store.hpp"
#include "candidate_store.hpp"
#include "cdcl.hpp"

struct sim_context {
    size_t          max_resolutions;
    const database& db;
    const goals&    gl;
    trail&          t;
    sequencer&      vars;
    expr_pool&      ep;
    bind_map&       bm;
    lineage_pool&   lp;
    cdcl            c;
};

struct sim {
    sim(sim_context);
    bool operator()();
    const resolutions& get_resolutions() const;
    const decisions& get_decisions() const;
#ifndef DEBUG
protected:
#endif
    bool solved();
    bool conflicted();
    const resolution_lineage* derive_one();
    void resolve(const resolution_lineage*);
    virtual const resolution_lineage* decide_one() = 0;
    virtual void on_resolve(const resolution_lineage*) = 0;

    const database& db;
    trail& t;
    lineage_pool& lp;

    goal_store gs;
    candidate_store cs;

    copier cp;

    cdcl c;

    resolutions rs;
    decisions ds;
    size_t max_resolutions;
};

#endif
