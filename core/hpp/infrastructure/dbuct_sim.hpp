#ifndef DBUCT_SIM_HPP
#define DBUCT_SIM_HPP

#include <cstddef>
#include <vector>
#include "value_objects/lineage.hpp"
#include "value_objects/mcts_choice.hpp"

template<typename IPushDecisionFrame,
         typename IPopDecisionFrame,
         typename IGetDecisionDepth,
         typename IGetDecisionCount,
         typename IGetPenultimateDecisionChoiceDepth,
         typename IGetUltimateDecisionChoiceDepth,
         typename IGetChoiceDepth,
         typename IBackstepChoiceFrame,
         typename IInRollout,
         typename IChoose,
         typename ITerminate>
struct dbuct_sim {
    dbuct_sim(IPushDecisionFrame&,
              IPopDecisionFrame&,
              IGetDecisionDepth&,
              IGetDecisionCount&,
              IGetPenultimateDecisionChoiceDepth&,
              IGetUltimateDecisionChoiceDepth&,
              IGetChoiceDepth&,
              IBackstepChoiceFrame&,
              IInRollout&,
              IChoose&,
              ITerminate&);

    mcts_choice choose(const std::vector<mcts_choice>&, bool is_rule_choice);
    std::vector<const resolution_lineage*> terminate(double reward);
    bool at_root() const;

private:
    IPushDecisionFrame&                   push_decision_frame_;
    IPopDecisionFrame&                    pop_decision_frame_;
    IGetDecisionDepth&                    get_decision_depth_;
    IGetDecisionCount&                    get_decision_count_;
    IGetPenultimateDecisionChoiceDepth&   get_penultimate_decision_choice_depth_;
    IGetUltimateDecisionChoiceDepth&      get_ultimate_decision_choice_depth_;
    IGetChoiceDepth&                      get_choice_depth_;
    IBackstepChoiceFrame&                 backstep_choice_frame_;
    IInRollout&                           in_rollout_;
    IChoose&                              choose_;
    ITerminate&                           terminate_;
};

template<typename IPDF, typename IPopDF, typename IGDD, typename IGDC,
         typename IGPDCD, typename IGUDCD, typename IGCD, typename IBSCF,
         typename IIR, typename IC, typename IT>
dbuct_sim<IPDF, IPopDF, IGDD, IGDC, IGPDCD, IGUDCD, IGCD, IBSCF, IIR, IC, IT>::dbuct_sim(
    IPDF& push_decision_frame,
    IPopDF& pop_decision_frame,
    IGDD& get_decision_depth,
    IGDC& get_decision_count,
    IGPDCD& get_penultimate_decision_choice_depth,
    IGUDCD& get_ultimate_decision_choice_depth,
    IGCD& get_choice_depth,
    IBSCF& backstep_choice_frame,
    IIR& in_rollout,
    IC& choose,
    IT& terminate)
    : push_decision_frame_(push_decision_frame)
    , pop_decision_frame_(pop_decision_frame)
    , get_decision_depth_(get_decision_depth)
    , get_decision_count_(get_decision_count)
    , get_penultimate_decision_choice_depth_(get_penultimate_decision_choice_depth)
    , get_ultimate_decision_choice_depth_(get_ultimate_decision_choice_depth)
    , get_choice_depth_(get_choice_depth)
    , backstep_choice_frame_(backstep_choice_frame)
    , in_rollout_(in_rollout)
    , choose_(choose)
    , terminate_(terminate) {}

template<typename IPDF, typename IPopDF, typename IGDD, typename IGDC,
         typename IGPDCD, typename IGUDCD, typename IGCD, typename IBSCF,
         typename IIR, typename IC, typename IT>
bool dbuct_sim<IPDF, IPopDF, IGDD, IGDC, IGPDCD, IGUDCD, IGCD, IBSCF, IIR, IC, IT>::at_root() const {
    return get_decision_depth_.depth() == 0;
}

template<typename IPDF, typename IPopDF, typename IGDD, typename IGDC,
         typename IGPDCD, typename IGUDCD, typename IGCD, typename IBSCF,
         typename IIR, typename IC, typename IT>
mcts_choice dbuct_sim<IPDF, IPopDF, IGDD, IGDC, IGPDCD, IGUDCD, IGCD, IBSCF, IIR, IC, IT>::choose(
    const std::vector<mcts_choice>& choices, bool is_rule_choice) {
    const bool was_in_rollout = in_rollout_.in_rollout();
    mcts_choice chosen = choose_.choose(choices, choices);
    if (was_in_rollout)
        return chosen;
    if (is_rule_choice)
        push_decision_frame_.push_decision_frame();
    return chosen;
}

template<typename IPDF, typename IPopDF, typename IGDD, typename IGDC,
         typename IGPDCD, typename IGUDCD, typename IGCD, typename IBSCF,
         typename IIR, typename IC, typename IT>
std::vector<const resolution_lineage*> dbuct_sim<IPDF, IPopDF, IGDD, IGDC, IGPDCD, IGUDCD, IGCD, IBSCF, IIR, IC, IT>::terminate(
    double reward) {

    // 1. terminate dbuct
    terminate_.terminate(reward);

    // 2. navigate back to nearest decision frame that is
    //    at most as deep as the penultimate decision. pop choice frames

    // navigate back to the penultimate frame IF DEEPER than penultimate
    size_t penultimate_decision_choice_depth = get_penultimate_decision_choice_depth_.get_penultimate_decision_choice_depth();
    while (get_choice_depth_.depth() > penultimate_decision_choice_depth)
        backstep_choice_frame_.backstep();
    
    // 3. pop decision frames to synchronize
    std::vector<const resolution_lineage*> eliminations;

    size_t return_depth = get_choice_depth_.depth();
    
    while(get_ultimate_decision_choice_depth_.get_ultimate_decision_choice_depth() > return_depth) {
        eliminations.clear();
        // pop decision frame
        auto sm = pop_decision_frame_.pop_decision_frame();
        while (!sm.done()) {
            sm.resume();
            if (sm.has_yield())
                eliminations.push_back(sm.consume_yield());
        }
    }
    
    // snap to nearest decision frame IF return depth is DEEPER than nearest decision frame
    size_t new_ultimate_decision_choice_depth = get_ultimate_decision_choice_depth_.get_ultimate_decision_choice_depth();
    while (get_choice_depth_.depth() > new_ultimate_decision_choice_depth)
        backstep_choice_frame_.backstep();
    
    // 4. return eliminations
    return eliminations;
}

#endif
