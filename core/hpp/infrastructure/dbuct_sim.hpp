#ifndef DBUCT_SIM_HPP
#define DBUCT_SIM_HPP

#include <cstddef>
#include <cstdint>
#include <vector>
#include "value_objects/lineage.hpp"
#include "value_objects/mcts_choice.hpp"

template<typename IGetFrameDepth,
         typename IPushFrame,
         typename IPopCdclFrame,
         typename IGetUltimateDecisionDepth,
         typename IInRollout,
         typename IChoose,
         typename ITerminate>
struct dbuct_sim {
    dbuct_sim(IGetFrameDepth&, IPushFrame&, IPopCdclFrame&,
              IGetUltimateDecisionDepth&, IInRollout&, IChoose&, ITerminate&);

    mcts_choice choose(const std::vector<mcts_choice>&);
    std::vector<const resolution_lineage*> terminate(double reward);
    bool at_root() const;

private:
    IGetFrameDepth&                 get_frame_depth_;
    IPushFrame&                     push_frame_;
    IPopCdclFrame&                  pop_cdcl_frame_;
    IGetUltimateDecisionDepth&      ultimate_decision_depth_;
    IInRollout&                     in_rollout_;
    IChoose&                        choose_;
    ITerminate&                     terminate_;
};

template<typename IGFD, typename IPF, typename IPCF, typename IGUDD,
         typename IIR, typename IC, typename IT>
bool dbuct_sim<IGFD, IPF, IPCF, IGUDD, IIR, IC, IT>::at_root() const {
    return get_frame_depth_.depth() == 1;
}

template<typename IGFD, typename IPF, typename IPCF, typename IGUDD,
         typename IIR, typename IC, typename IT>
dbuct_sim<IGFD, IPF, IPCF, IGUDD, IIR, IC, IT>::dbuct_sim(
    IGFD& get_frame_depth, IPF& push_frame, IPCF& pop_cdcl_frame,
    IGUDD& ultimate_decision_depth, IIR& in_rollout, IC& choose, IT& terminate)
    : get_frame_depth_(get_frame_depth)
    , push_frame_(push_frame)
    , pop_cdcl_frame_(pop_cdcl_frame)
    , ultimate_decision_depth_(ultimate_decision_depth)
    , in_rollout_(in_rollout)
    , choose_(choose)
    , terminate_(terminate) {}

template<typename IGFD, typename IPF, typename IPCF, typename IGUDD,
         typename IIR, typename IC, typename IT>
mcts_choice dbuct_sim<IGFD, IPF, IPCF, IGUDD, IIR, IC, IT>::choose(
    const std::vector<mcts_choice>& choices) {
    const bool was_in_rollout = in_rollout_.in_rollout();
    mcts_choice chosen = choose_.choose(choices, choices);
    if (was_in_rollout)
        return chosen;
    push_frame_.push_frame();
    return chosen;
}

template<typename IGFD, typename IPF, typename IPCF, typename IGUDD,
         typename IIR, typename IC, typename IT>
std::vector<const resolution_lineage*> dbuct_sim<IGFD, IPF, IPCF, IGUDD, IIR, IC, IT>::terminate(
    double reward) {
    const std::size_t ultimate_depth =
        ultimate_decision_depth_.get_ultimate_decision_depth();
    const std::size_t max_return_depth =
        get_frame_depth_.depth() > 1
            ? (ultimate_depth > 0 ? ultimate_depth - 1 : 0)
            : SIZE_MAX;
    const std::size_t return_depth = terminate_.terminate(reward, max_return_depth);
    std::vector<const resolution_lineage*> eliminations;
    while (get_frame_depth_.depth() > return_depth) {
        eliminations.clear();
        auto sm = pop_cdcl_frame_.pop_frame();
        while (!sm.done()) {
            sm.resume();
            if (sm.has_yield())
                eliminations.push_back(sm.consume_yield());
        }
    }
    return eliminations;
}

#endif
