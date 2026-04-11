#ifndef SIM_HPP
#define SIM_HPP

#include "lineage.hpp"
#include "defs.hpp"

struct sim {
    virtual ~sim() = default;
    sim(trail&, size_t);
    bool operator()();
    const resolutions& get_resolutions() const;
    const decisions& get_decisions() const;
#ifndef DEBUG
protected:
#endif
    void resolve(const resolution_lineage*);
    
    virtual bool solved() = 0;
    virtual bool conflicted() = 0;
    virtual const resolution_lineage* derive_one() = 0;
    virtual const resolution_lineage* decide_one() = 0;
    virtual void on_resolve(const resolution_lineage*) = 0;

    size_t max_resolutions;

    delta<resolutions> rs;
    delta<decisions> ds;
};

#endif
