#ifndef DBUCT_SIM_HPP
#define DBUCT_SIM_HPP

#include <cstddef>
#include <vector>
#include "value_objects/lineage.hpp"

template<typename MctsChoice,
         typename IPushSolverFrame,
         typename IPopSolverFrame,
         typename IGetSolverFrameDepth,
         typename IGetDecisionCount,
         typename IGetPenultimateMctsFrameDepth,
         typename IGetUltimateMctsFrameDepth,
         typename IGetMctsFrameDepth,
         typename IBackstepMctsFrame,
         typename IInRollout,
         typename IChoose,
         typename ITerminate,
         typename ICheckMctsChoiceIsRuleChoice>
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
              ITerminate&,
              ICheckMctsChoiceIsRuleChoice&);

    MctsChoice choose(const std::vector<MctsChoice>&);
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
    ITerminate&                         terminate_;
    ICheckMctsChoiceIsRuleChoice&       check_rule_choice_;
};

template<typename MC, typename IPSF, typename IPopSF, typename IGSFD, typename IGDC,
         typename IGPMFD, typename IGUMFD, typename IGMFD, typename IBMF,
         typename IIR, typename IC, typename IT, typename ICMCR>
dbuct_sim<MC, IPSF, IPopSF, IGSFD, IGDC, IGPMFD, IGUMFD, IGMFD, IBMF, IIR, IC, IT, ICMCR>::dbuct_sim(
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
    IT& terminate,
    ICMCR& check_rule_choice)
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
    , terminate_(terminate)
    , check_rule_choice_(check_rule_choice) {}

template<typename MC, typename IPSF, typename IPopSF, typename IGSFD, typename IGDC,
         typename IGPMFD, typename IGUMFD, typename IGMFD, typename IBMF,
         typename IIR, typename IC, typename IT, typename ICMCR>
bool dbuct_sim<MC, IPSF, IPopSF, IGSFD, IGDC, IGPMFD, IGUMFD, IGMFD, IBMF, IIR, IC, IT, ICMCR>::at_root() const {
    return get_decision_count_.count() == 0;
}

template<typename MC, typename IPSF, typename IPopSF, typename IGSFD, typename IGDC,
         typename IGPMFD, typename IGUMFD, typename IGMFD, typename IBMF,
         typename IIR, typename IC, typename IT, typename ICMCR>
MC dbuct_sim<MC, IPSF, IPopSF, IGSFD, IGDC, IGPMFD, IGUMFD, IGMFD, IBMF, IIR, IC, IT, ICMCR>::choose(
    const std::vector<MC>& choices) {
    MC chosen = choose_.choose(choices, choices);
    // if there has not been a mcts frame since the last decision, don't push solver frame.
    if (get_mcts_frame_depth_.depth() <= get_ultimate_mcts_frame_depth_.get_ultimate_mcts_frame_depth())
        return chosen;
    if (check_rule_choice_.check_is_rule_choice(chosen))
        push_solver_frame_.push_solver_frame();
    return chosen;
}

template<typename MC, typename IPSF, typename IPopSF, typename IGSFD, typename IGDC,
         typename IGPMFD, typename IGUMFD, typename IGMFD, typename IBMF,
         typename IIR, typename IC, typename IT, typename ICMCR>
std::vector<const resolution_lineage*> dbuct_sim<MC, IPSF, IPopSF, IGSFD, IGDC, IGPMFD, IGUMFD, IGMFD, IBMF, IIR, IC, IT, ICMCR>::terminate() {
    backtrack_mcts();
    return backtrack_solver_and_align();
}

template<typename MC, typename IPSF, typename IPopSF, typename IGSFD, typename IGDC,
         typename IGPMFD, typename IGUMFD, typename IGMFD, typename IBMF,
         typename IIR, typename IC, typename IT, typename ICMCR>
void dbuct_sim<MC, IPSF, IPopSF, IGSFD, IGDC, IGPMFD, IGUMFD, IGMFD, IBMF, IIR, IC, IT, ICMCR>::backtrack_mcts() {
    terminate_.terminate();

    // if we are still at or deeper than ultimate, backstep once
    if (get_mcts_frame_depth_.depth() > 1 && get_mcts_frame_depth_.depth() >= get_ultimate_mcts_frame_depth_.get_ultimate_mcts_frame_depth())
        backstep_mcts_frame_.backstep();
}

template<typename MC, typename IPSF, typename IPopSF, typename IGSFD, typename IGDC,
         typename IGPMFD, typename IGUMFD, typename IGMFD, typename IBMF,
         typename IIR, typename IC, typename IT, typename ICMCR>
std::vector<const resolution_lineage*> dbuct_sim<MC, IPSF, IPopSF, IGSFD, IGDC, IGPMFD, IGUMFD, IGMFD, IBMF, IIR, IC, IT, ICMCR>::backtrack_solver_and_align() {
    // pop solver frames until at or before current mcts frame depth
    std::vector<const resolution_lineage*> eliminations;
    while (get_ultimate_mcts_frame_depth_.get_ultimate_mcts_frame_depth() > get_mcts_frame_depth_.depth()) {
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
    while (get_mcts_frame_depth_.depth() > ultimate_mcts_frame_depth)
        backstep_mcts_frame_.backstep();

    return eliminations;
}

#endif
