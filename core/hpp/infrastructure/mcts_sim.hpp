#ifndef MCTS_SIM_HPP
#define MCTS_SIM_HPP

#include <optional>
#include <vector>
#include "mcts.hpp"

// Orchestrates one Monte Carlo simulation episode. All visitors, tables, walker,
// rollout, value-delta, exploration constant, and root are injected; this type
// only owns the per-episode monte_carlo::sim object.
template<typename MctsNodeId,
         typename MctsChoice,
         typename IGetMctsNodeVisits,
         typename ISetMctsNodeVisits,
         typename IGetMctsNodeValue,
         typename ISetMctsNodeValue,
         typename IWalkMctsNode,
         typename IRolloutMctsChoice,
         typename IGetMctsValueDelta,
         typename IGetExplorationConstant,
         typename IGetMctsRootNode>
struct mcts_sim {
    mcts_sim(IGetMctsNodeVisits&,
             ISetMctsNodeVisits&,
             IGetMctsNodeValue&,
             ISetMctsNodeValue&,
             IWalkMctsNode&,
             IRolloutMctsChoice&,
             IGetMctsValueDelta&,
             IGetExplorationConstant&,
             IGetMctsRootNode&);

    void set_up();
    void tear_down();
    MctsChoice choose(const std::vector<MctsChoice>&);

private:
    using vec_t = std::vector<MctsChoice>;
    using mc_sim_t = monte_carlo::sim<
                         MctsNodeId,
                         MctsChoice,
                         double,
                         IGetMctsNodeVisits,
                         IGetMctsNodeValue,
                         ISetMctsNodeVisits,
                         ISetMctsNodeValue,
                         IWalkMctsNode,
                         vec_t,
                         vec_t,
                         IRolloutMctsChoice,
                         IGetMctsValueDelta,
                         IGetExplorationConstant>;

    IGetMctsNodeVisits&       get_mcts_node_visits_;
    ISetMctsNodeVisits&       set_mcts_node_visits_;
    IGetMctsNodeValue&        get_mcts_node_value_;
    ISetMctsNodeValue&        set_mcts_node_value_;
    IWalkMctsNode&            walk_mcts_node_;
    IRolloutMctsChoice&       rollout_mcts_choice_;
    IGetMctsValueDelta&       get_mcts_value_delta_;
    IGetExplorationConstant&  get_exploration_constant_;
    IGetMctsRootNode&         get_mcts_root_node_;
    std::optional<mc_sim_t>   mc_sim_;
};

template<typename MN, typename MC,
         typename IGVis, typename ISVis, typename IGVal, typename ISVal,
         typename IWM, typename IRC, typename IGVD, typename IGEC, typename IGRN>
mcts_sim<MN, MC, IGVis, ISVis, IGVal, ISVal, IWM, IRC, IGVD, IGEC, IGRN>::mcts_sim(
        IGVis& get_mcts_node_visits,
        ISVis& set_mcts_node_visits,
        IGVal& get_mcts_node_value,
        ISVal& set_mcts_node_value,
        IWM& walk_mcts_node,
        IRC& rollout_mcts_choice,
        IGVD& get_mcts_value_delta,
        IGEC& get_exploration_constant,
        IGRN& get_mcts_root_node)
    : get_mcts_node_visits_(get_mcts_node_visits)
    , set_mcts_node_visits_(set_mcts_node_visits)
    , get_mcts_node_value_(get_mcts_node_value)
    , set_mcts_node_value_(set_mcts_node_value)
    , walk_mcts_node_(walk_mcts_node)
    , rollout_mcts_choice_(rollout_mcts_choice)
    , get_mcts_value_delta_(get_mcts_value_delta)
    , get_exploration_constant_(get_exploration_constant)
    , get_mcts_root_node_(get_mcts_root_node)
    , mc_sim_{}
{}

template<typename MN, typename MC,
         typename IGVis, typename ISVis, typename IGVal, typename ISVal,
         typename IWM, typename IRC, typename IGVD, typename IGEC, typename IGRN>
void mcts_sim<MN, MC, IGVis, ISVis, IGVal, ISVal, IWM, IRC, IGVD, IGEC, IGRN>::set_up() {
    mc_sim_.emplace(get_mcts_node_visits_,
                    get_mcts_node_value_,
                    set_mcts_node_visits_,
                    set_mcts_node_value_,
                    walk_mcts_node_,
                    rollout_mcts_choice_,
                    get_mcts_value_delta_,
                    get_exploration_constant_,
                    get_mcts_root_node_.get_mcts_root_node());
}

template<typename MN, typename MC,
         typename IGVis, typename ISVis, typename IGVal, typename ISVal,
         typename IWM, typename IRC, typename IGVD, typename IGEC, typename IGRN>
void mcts_sim<MN, MC, IGVis, ISVis, IGVal, ISVal, IWM, IRC, IGVD, IGEC, IGRN>::tear_down() {
    mc_sim_->terminate();
}

template<typename MN, typename MC,
         typename IGVis, typename ISVis, typename IGVal, typename ISVal,
         typename IWM, typename IRC, typename IGVD, typename IGEC, typename IGRN>
MC mcts_sim<MN, MC, IGVis, ISVis, IGVal, ISVal, IWM, IRC, IGVD, IGEC, IGRN>::choose(
        const std::vector<MC>& choices) {
    return mc_sim_->choose(choices, choices);
}

#endif
