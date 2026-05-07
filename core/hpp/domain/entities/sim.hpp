#ifndef SIM_HPP
#define SIM_HPP

#include "goal_store.hpp"
#include "candidate_store.hpp"
#include "cdcl.hpp"
#include "initial_condition_detector.hpp"
#include "cdcl_eliminator.hpp"
#include "head_eliminator.hpp"
#include "frontier_watch.hpp"

struct sim {
    sim();
    bool operator()();
    const resolutions& get_resolutions() const;
    const decisions& get_decisions() const;
protected:
    bool solved();
    bool conflicted();
    const resolution_lineage* derive_one();
    virtual const resolution_lineage* decide_one() = 0;
    void resolve(const resolution_lineage*);
    virtual void on_resolve(const resolution_lineage*) = 0;

    const database& db;
    trail& t;
    lineage_pool& lp;

    goal_store gs;
    candidate_store cs;
    copier cp;
    cdcl c;
    topic<const goal_lineage*> goal_inserted_topic;
    topic<const resolution_lineage*> goal_resolved_topic;
    topic<const resolution_lineage*> new_eliminated_resolution_topic;
    topic<const resolution_lineage*> unit_topic;
    topic<const resolution_lineage*>::subscription unit_subscription;
    initial_condition_detector icd;
    cdcl_eliminator ce;
    head_eliminator he;
    frontier_watch fw; // fw needs to be constructed last for events to be signaled correctly to listeners
    resolutions rs;
    decisions ds;
    size_t max_resolutions;
};

#endif
