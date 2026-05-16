#ifndef ACTIVE_ELIMINATOR_HPP
#define ACTIVE_ELIMINATOR_HPP

#include "../interfaces/i_active_eliminator.hpp"
#include "../interfaces/i_frontier.hpp"

struct active_eliminator : i_active_eliminator {
    active_eliminator();
    void eliminate(const resolution_lineage*) override;
private:
    i_frontier& f;

    const resolution_lineage* current_rl = nullptr;
};

#endif
