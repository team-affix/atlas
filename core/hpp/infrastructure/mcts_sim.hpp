#ifndef MCTS_SIM_HPP
#define MCTS_SIM_HPP

#include <optional>
#include <vector>
#include "mcts.hpp"

// Orchestrates one Monte Carlo simulation episode. All visitors, tables, walker,
// rollout, value-delta, and root are injected; this type only owns the per-
// episode monte_carlo::sim object.
template<typename MctsNodeId,
         typename MctsChoice,
         typename IGetMctsNodeVisits,
         typename ISetMctsNodeVisits,
         typename IGetMctsNodeValue,
         typename ISetMctsNodeValue,
         typename IWalkMctsNode,
         typename IRolloutMctsChoice,
         typename IGetMctsValueDelta,
         typename IGetMctsRootNode>
struct mcts_sim {
    mcts_sim(IGetMctsNodeVisits&,
             ISetMctsNodeVisits&,
             IGetMctsNodeValue&,
             ISetMctsNodeValue&,
             IWalkMctsNode&,
             IRolloutMctsChoice&,
             IGetMctsValueDelta&,
             IGetMctsRootNode&,
             double exploration_constant);

    void set_up();
    void tear_down();
    MctsChoice choose(const std::vector<MctsChoice>&, bool is_rule_choice);

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
                         IGetMctsValueDelta>;

    IGetMctsNodeVisits&  get_mcts_node_visits_;
    ISetMctsNodeVisits&  set_mcts_node_visits_;
    IGetMctsNodeValue&   get_mcts_node_value_;
    ISetMctsNodeValue&   set_mcts_node_value_;
    IWalkMctsNode&       walk_mcts_node_;
    IRolloutMctsChoice&  rollout_mcts_choice_;
    IGetMctsValueDelta&  get_mcts_value_delta_;
    IGetMctsRootNode&    get_mcts_root_node_;
    double               exploration_constant_;
    std::optional<mc_sim_t> mc_sim_;
};

template<typename MN, typename MC,
         typename IGVis, typename ISVis, typename IGVal, typename ISVal,
         typename IWM, typename IRC, typename IGVD, typename IGRN>
mcts_sim<MN, MC, IGVis, ISVis, IGVal, ISVal, IWM, IRC, IGVD, IGRN>::mcts_sim(
        IGVis& get_mcts_node_visits,
        ISVis& set_mcts_node_visits,
        IGVal& get_mcts_node_value,
        ISVal& set_mcts_node_value,
        IWM& walk_mcts_node,
        IRC& rollout_mcts_choice,
        IGVD& get_mcts_value_delta,
        IGRN& get_mcts_root_node,
        double exploration_constant)
    : get_mcts_node_visits_(get_mcts_node_visits)
    , set_mcts_node_visits_(set_mcts_node_visits)
    , get_mcts_node_value_(get_mcts_node_value)
    , set_mcts_node_value_(set_mcts_node_value)
    , walk_mcts_node_(walk_mcts_node)
    , rollout_mcts_choice_(rollout_mcts_choice)
    , get_mcts_value_delta_(get_mcts_value_delta)
    , get_mcts_root_node_(get_mcts_root_node)
    , exploration_constant_(exploration_constant)
    , mc_sim_{}
{}

template<typename MN, typename MC,
         typename IGVis, typename ISVis, typename IGVal, typename ISVal,
         typename IWM, typename IRC, typename IGVD, typename IGRN>
void mcts_sim<MN, MC, IGVis, ISVis, IGVal, ISVal, IWM, IRC, IGVD, IGRN>::set_up() {
    mc_sim_.emplace(get_mcts_node_visits_,
                    get_mcts_node_value_,
                    set_mcts_node_visits_,
                    set_mcts_node_value_,
                    walk_mcts_node_,
                    rollout_mcts_choice_,
                    get_mcts_value_delta_,
                    get_mcts_root_node_.get_mcts_root_node(),
                    exploration_constant_);
}

template<typename MN, typename MC,
         typename IGVis, typename ISVis, typename IGVal, typename ISVal,
         typename IWM, typename IRC, typename IGVD, typename IGRN>
void mcts_sim<MN, MC, IGVis, ISVis, IGVal, ISVal, IWM, IRC, IGVD, IGRN>::tear_down() {
    mc_sim_->terminate();
}

template<typename MN, typename MC,
         typename IGVis, typename ISVis, typename IGVal, typename ISVal,
         typename IWM, typename IRC, typename IGVD, typename IGRN>
MC mcts_sim<MN, MC, IGVis, ISVis, IGVal, ISVal, IWM, IRC, IGVD, IGRN>::choose(
        const std::vector<MC>& choices, bool is_rule_choice) {
    (void)is_rule_choice;
    return mc_sim_->choose(choices, choices);
}

#endif
