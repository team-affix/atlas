#ifndef DBUCT_SIM_HPP
#define DBUCT_SIM_HPP

#include <cstddef>
#include <vector>
#include "value_objects/lineage.hpp"
#include "value_objects/mcts_choice.hpp"

template<typename IPushSolverFrame,
         typename IPopSolverFrame,
         typename IGetSolverFrameDepth,
         typename IGetDecisionCount,
         typename IGetPenultimateMctsFrameDepth,
         typename IGetUltimateMctsFrameDepth,
         typename IGetMctsFrameDepth,
         typename IBackstepMctsFrame,
         typename IInRollout,
         typename IChoose,
         typename IComputeMctsReward,
         typename ITerminate>
struct dbuct_sim {
    dbuct_sim(IPushSolverFrame&,
              IPopSolverFrame&,
              IGetSolverFrameDepth&,
              IGetDecisionCount&,
              IGetPenultimateMctsFrameDepth&,
              IGetUltimateMctsFrameDepth&,
              IGetMctsFrameDepth&,
              IBackstepMctsFrame&,
              IInRollout&,
              IChoose&,
              IComputeMctsReward&,
              ITerminate&);

    mcts_choice choose(const std::vector<mcts_choice>&, bool is_rule_choice);
    std::vector<const resolution_lineage*> terminate();
    bool at_root() const;

private:
    void backtrack_mcts();
    std::vector<const resolution_lineage*> backtrack_solver_and_align();

    IPushSolverFrame&                   push_solver_frame_;
    IPopSolverFrame&                    pop_solver_frame_;
    IGetSolverFrameDepth&               get_solver_frame_depth_;
    IGetDecisionCount&                  get_decision_count_;
    IGetPenultimateMctsFrameDepth&      get_penultimate_mcts_frame_depth_;
    IGetUltimateMctsFrameDepth&         get_ultimate_mcts_frame_depth_;
    IGetMctsFrameDepth&                 get_mcts_frame_depth_;
    IBackstepMctsFrame&                 backstep_mcts_frame_;
    IInRollout&                         in_rollout_;
    IChoose&                            choose_;
    IComputeMctsReward&                 compute_mcts_reward_;
    ITerminate&                         terminate_;
};

template<typename IPSF, typename IPopSF, typename IGSFD, typename IGDC,
         typename IGPMFD, typename IGUMFD, typename IGMFD, typename IBMF,
         typename IIR, typename IC, typename ICMR, typename IT>
dbuct_sim<IPSF, IPopSF, IGSFD, IGDC, IGPMFD, IGUMFD, IGMFD, IBMF, IIR, IC, ICMR, IT>::dbuct_sim(
    IPSF& push_solver_frame,
    IPopSF& pop_solver_frame,
    IGSFD& get_solver_frame_depth,
    IGDC& get_decision_count,
    IGPMFD& get_penultimate_mcts_frame_depth,
    IGUMFD& get_ultimate_mcts_frame_depth,
    IGMFD& get_mcts_frame_depth,
    IBMF& backstep_mcts_frame,
    IIR& in_rollout,
    IC& choose,
    ICMR& compute_mcts_reward,
    IT& terminate)
    : push_solver_frame_(push_solver_frame)
    , pop_solver_frame_(pop_solver_frame)
    , get_solver_frame_depth_(get_solver_frame_depth)
    , get_decision_count_(get_decision_count)
    , get_penultimate_mcts_frame_depth_(get_penultimate_mcts_frame_depth)
    , get_ultimate_mcts_frame_depth_(get_ultimate_mcts_frame_depth)
    , get_mcts_frame_depth_(get_mcts_frame_depth)
    , backstep_mcts_frame_(backstep_mcts_frame)
    , in_rollout_(in_rollout)
    , choose_(choose)
    , compute_mcts_reward_(compute_mcts_reward)
    , terminate_(terminate) {}

template<typename IPSF, typename IPopSF, typename IGSFD, typename IGDC,
         typename IGPMFD, typename IGUMFD, typename IGMFD, typename IBMF,
         typename IIR, typename IC, typename ICMR, typename IT>
bool dbuct_sim<IPSF, IPopSF, IGSFD, IGDC, IGPMFD, IGUMFD, IGMFD, IBMF, IIR, IC, ICMR, IT>::at_root() const {
    return get_decision_count_.count() == 0;
}

template<typename IPSF, typename IPopSF, typename IGSFD, typename IGDC,
         typename IGPMFD, typename IGUMFD, typename IGMFD, typename IBMF,
         typename IIR, typename IC, typename ICMR, typename IT>
mcts_choice dbuct_sim<IPSF, IPopSF, IGSFD, IGDC, IGPMFD, IGUMFD, IGMFD, IBMF, IIR, IC, ICMR, IT>::choose(
    const std::vector<mcts_choice>& choices, bool is_rule_choice) {
    const bool was_in_rollout = in_rollout_.in_rollout();
    mcts_choice chosen = choose_.choose(choices, choices);
    if (was_in_rollout)
        return chosen;
    if (is_rule_choice)
        push_solver_frame_.push_solver_frame();
    return chosen;
}

template<typename IPSF, typename IPopSF, typename IGSFD, typename IGDC,
         typename IGPMFD, typename IGUMFD, typename IGMFD, typename IBMF,
         typename IIR, typename IC, typename ICMR, typename IT>
std::vector<const resolution_lineage*> dbuct_sim<IPSF, IPopSF, IGSFD, IGDC, IGPMFD, IGUMFD, IGMFD, IBMF, IIR, IC, ICMR, IT>::terminate() {
    backtrack_mcts();
    return backtrack_solver_and_align();
}

template<typename IPSF, typename IPopSF, typename IGSFD, typename IGDC,
         typename IGPMFD, typename IGUMFD, typename IGMFD, typename IBMF,
         typename IIR, typename IC, typename ICMR, typename IT>
void dbuct_sim<IPSF, IPopSF, IGSFD, IGDC, IGPMFD, IGUMFD, IGMFD, IBMF, IIR, IC, ICMR, IT>::backtrack_mcts() {
    // get reward
    double reward = compute_mcts_reward_.compute_mcts_reward();

    // terminate mcts + backstep to suggested return depth
    terminate_.terminate(reward);

    // if we are still at or deeper than ultimate, backstep once
    if (get_mcts_frame_depth_.mcts_frame_depth() >= get_ultimate_mcts_frame_depth_.get_ultimate_mcts_frame_depth())
        backstep_mcts_frame_.backstep();
}

template<typename IPSF, typename IPopSF, typename IGSFD, typename IGDC,
         typename IGPMFD, typename IGUMFD, typename IGMFD, typename IBMF,
         typename IIR, typename IC, typename ICMR, typename IT>
std::vector<const resolution_lineage*> dbuct_sim<IPSF, IPopSF, IGSFD, IGDC, IGPMFD, IGUMFD, IGMFD, IBMF, IIR, IC, ICMR, IT>::backtrack_solver_and_align() {
    // pop solver frames until at or before current mcts frame depth
    std::vector<const resolution_lineage*> eliminations;
    while (get_ultimate_mcts_frame_depth_.get_ultimate_mcts_frame_depth() > get_mcts_frame_depth_.mcts_frame_depth()) {
        eliminations.clear();
        auto sm = pop_solver_frame_.pop_solver_frame();
        while (!sm.done()) {
            sm.resume();
            if (sm.has_yield())
                eliminations.push_back(sm.consume_yield());
        }
    }
    
    // get ultimate mcts frame depth (mcts depth of the ultimate decision)
    size_t ultimate_mcts_frame_depth = get_ultimate_mcts_frame_depth_.get_ultimate_mcts_frame_depth();
    
    // backstep mcts frames until at solver frame
    while (get_mcts_frame_depth_.mcts_frame_depth() > ultimate_mcts_frame_depth)
        backstep_mcts_frame_.backstep();

    return eliminations;
}

#endif
