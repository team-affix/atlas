#ifndef CANDIDATE_ACTIVATOR_HPP
#define CANDIDATE_ACTIVATOR_HPP

#include "../interfaces/i_candidate_activator.hpp"
#include "../interfaces/i_elimination_backlog.hpp"

struct candidate_activator : i_candidate_activator {
    candidate_activator();
    void try_activate(const resolution_lineage*) override;
private:
    i_elimination_backlog& eb;
};

#endif
