#ifndef DBUCT_HORIZON_TERMINATE_SIM_HPP
#define DBUCT_HORIZON_TERMINATE_SIM_HPP

#include <vector>
#include "value_objects/lineage.hpp"

template<typename IComputeMctsReward, typename ISetValueDelta, typename ITerminateDbuct>
struct dbuct_horizon_terminate_sim {
    dbuct_horizon_terminate_sim(IComputeMctsReward&, ISetValueDelta&, ITerminateDbuct&);
    std::vector<const resolution_lineage*> terminate();
private:
    IComputeMctsReward& compute_mcts_reward_;
    ISetValueDelta& set_value_delta_;
    ITerminateDbuct& terminate_dbuct_;
};

template<typename ICMR, typename ISVD, typename ITD>
dbuct_horizon_terminate_sim<ICMR, ISVD, ITD>::dbuct_horizon_terminate_sim(
    ICMR& compute_mcts_reward, ISVD& set_value_delta, ITD& terminate_dbuct)
    : compute_mcts_reward_(compute_mcts_reward)
    , set_value_delta_(set_value_delta)
    , terminate_dbuct_(terminate_dbuct) {}

template<typename ICMR, typename ISVD, typename ITD>
std::vector<const resolution_lineage*>
dbuct_horizon_terminate_sim<ICMR, ISVD, ITD>::terminate() {
    set_value_delta_.set_value(compute_mcts_reward_.compute_mcts_reward());
    return terminate_dbuct_.terminate();
}

#endif
