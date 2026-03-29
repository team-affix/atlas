#ifndef SIM_HPP
#define SIM_HPP

#include "lineage.hpp"
#include "defs.hpp"

struct sim {
    sim(size_t);
    bool operator()();
    const resolutions& get_resolutions() const;
    const decisions& get_decisions() const;
#ifndef DEBUG
private:
#endif
    virtual bool solved() const = 0;
    virtual bool conflicted() const = 0;
    virtual const resolution_lineage* derive_one() = 0;
    virtual const resolution_lineage* decide_one() = 0;
    virtual void on_resolve(const resolution_lineage*) = 0;

    resolutions rs;
    decisions ds;
    size_t max_resolutions;
};

#endif
