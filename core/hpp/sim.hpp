#ifndef SIM_HPP
#define SIM_HPP

#include "sim_args.hpp"
#include "goal_store.hpp"
#include "candidate_store.hpp"
#include "event_aggregator.hpp"

struct sim {
    sim(sim_args);
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

    event_aggregator ea;

    resolutions rs;
    decisions ds;
    size_t max_resolutions;
};

#endif
