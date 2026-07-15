#ifndef DBUCT_GENIUS_TERMINATE_SIM_HPP
#define DBUCT_GENIUS_TERMINATE_SIM_HPP

#include <vector>
#include "value_objects/lineage.hpp"

template<typename ITerminateDbuct>
struct dbuct_genius_terminate_sim {
    dbuct_genius_terminate_sim(ITerminateDbuct&);
    std::vector<const resolution_lineage*> terminate();
private:
    ITerminateDbuct& terminate_dbuct_;
};

template<typename ITD>
dbuct_genius_terminate_sim<ITD>::dbuct_genius_terminate_sim(ITD& terminate_dbuct)
    : terminate_dbuct_(terminate_dbuct) {}

template<typename ITD>
std::vector<const resolution_lineage*>
dbuct_genius_terminate_sim<ITD>::terminate() {
    return terminate_dbuct_.terminate();
}

#endif
