#ifndef SOLVER_HPP
#define SOLVER_HPP

#include "../interfaces/i_solver.hpp"
#include "../interfaces/i_sim_setup.hpp"
#include "../interfaces/i_sim_teardown.hpp"


struct solver : i_solver {
    solver();
    virtual ~solver();
    bool operator()(std::optional<resolutions>&);
protected:
    virtual std::unique_ptr<sim> construct_sim() = 0;
    virtual void terminate(sim&) = 0;

    const database& db;
    const goals& gl;
    trail& t;
    sequencer& vars;
    unifier& bm;

    expr_pool ep;
    lineage_pool lp;

    size_t max_resolutions;
    cdcl c;

    std::unique_ptr<sim> managed_sim;
};

#endif
