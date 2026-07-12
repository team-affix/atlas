#ifndef DBUCT_SIM_HPP
#define DBUCT_SIM_HPP

#include <cstddef>
#include <cstdint>
#include <vector>
#include "value_objects/lineage.hpp"
#include "value_objects/mcts_choice.hpp"

template<typename IFrameHub, typename IFrameControl,
         typename IGetUltimateDecisionDepth, typename IDbuct>
struct dbuct_sim {
    dbuct_sim(IFrameHub&, IFrameControl&, IGetUltimateDecisionDepth&, IDbuct&);

    mcts_choice choose(const std::vector<mcts_choice>&);
    std::vector<const resolution_lineage*> terminate(double reward);
    bool at_root() const;
    void push_base_frame();

private:
    IFrameHub&                 hub_;
    IFrameControl&             frames_;
    IGetUltimateDecisionDepth& ultimate_decision_depth_;
    IDbuct&                    dbuct_;
};

template<typename IFrameHub, typename IFrameControl,
         typename IGetUltimateDecisionDepth, typename IDbuct>
bool dbuct_sim<IFrameHub, IFrameControl, IGetUltimateDecisionDepth, IDbuct>::at_root() const {
    return hub_.depth() == 1;
}

template<typename IFrameHub, typename IFrameControl,
         typename IGetUltimateDecisionDepth, typename IDbuct>
dbuct_sim<IFrameHub, IFrameControl, IGetUltimateDecisionDepth, IDbuct>::dbuct_sim(
    IFrameHub& hub, IFrameControl& frames,
    IGetUltimateDecisionDepth& ultimate_decision_depth, IDbuct& dbuct)
    : hub_(hub)
    , frames_(frames)
    , ultimate_decision_depth_(ultimate_decision_depth)
    , dbuct_(dbuct) {}

template<typename IFrameHub, typename IFrameControl,
         typename IGetUltimateDecisionDepth, typename IDbuct>
void dbuct_sim<IFrameHub, IFrameControl, IGetUltimateDecisionDepth, IDbuct>::push_base_frame() {
    hub_.push_frame();
}

template<typename IFrameHub, typename IFrameControl,
         typename IGetUltimateDecisionDepth, typename IDbuct>
mcts_choice dbuct_sim<IFrameHub, IFrameControl, IGetUltimateDecisionDepth, IDbuct>::choose(
    const std::vector<mcts_choice>& choices) {
    const bool was_in_rollout = dbuct_.in_rollout();
    mcts_choice chosen = dbuct_.choose(choices, choices);
    if (was_in_rollout)
        return chosen;
    hub_.push_frame();
    frames_.push_frame();
    return chosen;
}

template<typename IFrameHub, typename IFrameControl,
         typename IGetUltimateDecisionDepth, typename IDbuct>
std::vector<const resolution_lineage*> dbuct_sim<IFrameHub, IFrameControl, IGetUltimateDecisionDepth, IDbuct>::terminate(
    double reward) {
    const std::size_t ultimate_depth =
        ultimate_decision_depth_.get_ultimate_decision_depth();
    const std::size_t max_return_depth =
        hub_.depth() > 1
            ? (ultimate_depth > 0 ? ultimate_depth - 1 : 0)
            : SIZE_MAX;
    const std::size_t return_depth = dbuct_.terminate(reward, max_return_depth);
    std::vector<const resolution_lineage*> eliminations;
    while (hub_.depth() > return_depth) {
        eliminations.clear();
        hub_.pop_frame();
        auto sm = frames_.pop_frame();
        while (!sm.done()) {
            sm.resume();
            if (sm.has_yield())
                eliminations.push_back(sm.consume_yield());
        }
    }
    return eliminations;
}

#endif
