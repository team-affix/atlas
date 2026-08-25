#ifndef ATLAS_DBUCT_MANIFEST_HPP
#define ATLAS_DBUCT_MANIFEST_HPP

#include <cstddef>
#include <limits>
#include "infrastructure/dbuct_chooser.hpp"
#include "infrastructure/dbuct_frame_stack.hpp"
#include "infrastructure/dbuct_frame_stack_controller.hpp"
#include "infrastructure/dbuct_terminator.hpp"
#include "infrastructure/dbuct_value_adder.hpp"
#include "infrastructure/dbuct_value_creditor.hpp"
#include "infrastructure/dbuct_value_stack.hpp"
#include "infrastructure/dbuct_value_stack_controller.hpp"
#include "infrastructure/dbuct_visit_adder.hpp"
#include "infrastructure/dbuct_visit_creditor.hpp"
#include "infrastructure/in_rollout_flag.hpp"
#include "infrastructure/ucb1.hpp"
#include "infrastructure/visit_proportional_grant.hpp"
#include "value_objects/dbuct_frame.hpp"
#include "value_objects/dbuct_value_frame.hpp"

template<
    typename INodeHandle,
    typename IChoice,
    typename IFloat,
    typename IGetVisits,
    typename ISetVisits,
    typename IGetValue,
    typename ISetValue,
    typename IWalker,
    typename IGetChoiceCount,
    typename IGetChoiceAt,
    typename IRolloutChoose,
    typename IGetExplorationConstant,
    typename IGetValueDelta
>
struct atlas_dbuct_manifest
{
    using grant_t        = monte_carlo::visit_proportional_grant<INodeHandle, IFloat, IGetVisits>;
    using frame_stack_t  = monte_carlo::dbuct_frame_stack<INodeHandle>;
    using visit_adder_t  = monte_carlo::dbuct_visit_adder<INodeHandle, frame_stack_t,
                                              IGetVisits, ISetVisits>;
    using frame_stack_controller_t
                         = monte_carlo::dbuct_frame_stack_controller<INodeHandle,
                                                         frame_stack_t, frame_stack_t,
                                                         frame_stack_t, visit_adder_t>;
    using value_stack_t  = monte_carlo::dbuct_value_stack<INodeHandle, IFloat>;
    using value_adder_t  = monte_carlo::dbuct_value_adder<INodeHandle, IFloat, value_stack_t,
                                              IGetValue, ISetValue>;
    using value_stack_controller_t
                         = monte_carlo::dbuct_value_stack_controller<INodeHandle, IFloat,
                                                         frame_stack_controller_t,
                                                         frame_stack_controller_t,
                                                         value_stack_t, value_stack_t,
                                                         value_stack_t, value_adder_t>;
    using visit_creditor_t = monte_carlo::dbuct_visit_creditor<visit_adder_t>;
    using value_creditor_t = monte_carlo::dbuct_value_creditor<visit_creditor_t, value_stack_t,
                                                  value_adder_t, IGetValueDelta>;
    using policy_t       = monte_carlo::ucb1<INodeHandle, IChoice, IFloat,
                                 IGetVisits, IGetValue, IWalker,
                                 IGetExplorationConstant, IGetChoiceCount, IGetChoiceAt>;
    using chooser_t      = monte_carlo::dbuct_chooser<INodeHandle, IChoice,
                                          IGetVisits, grant_t,
                                          value_stack_controller_t,
                                          frame_stack_t,
                                          IWalker,
                                          IGetChoiceCount, IGetChoiceAt,
                                          policy_t, IRolloutChoose,
                                          monte_carlo::in_rollout_flag, monte_carlo::in_rollout_flag>;
    using terminator_t   = monte_carlo::dbuct_terminator<value_stack_controller_t,
                                             frame_stack_t,
                                             value_creditor_t,
                                             monte_carlo::in_rollout_flag>;

    atlas_dbuct_manifest(IGetVisits&,
                         ISetVisits&,
                         IGetValue&,
                         ISetValue&,
                         IWalker&,
                         IRolloutChoose&,
                         IGetExplorationConstant&,
                         IGetValueDelta&,
                         IFloat grant_k,
                         INodeHandle root);

    IWalker&                         walker;
    IRolloutChoose&                  rollout;
    IGetExplorationConstant&         exploration_constant;
    IGetValueDelta&                  delta;
    grant_t                          grant;
    frame_stack_t                    frame_stack;
    visit_adder_t                    visit_adder;
    frame_stack_controller_t         frame_stack_controller;
    value_stack_t                    value_stack;
    value_adder_t                    value_adder;
    value_stack_controller_t         value_stack_controller;
    visit_creditor_t                 visit_creditor;
    value_creditor_t                 value_creditor;
    policy_t                         policy;
    monte_carlo::in_rollout_flag     in_rollout;
    chooser_t                        chooser;
    terminator_t                     terminator;
};

template<typename INH, typename IC, typename IF,
         typename IGVis, typename ISVis, typename IGVal, typename ISVal,
         typename IW, typename IGCC, typename IGCA,
         typename IRC, typename IGEC, typename IGVD>
atlas_dbuct_manifest<INH, IC, IF, IGVis, ISVis, IGVal, ISVal, IW, IGCC, IGCA, IRC, IGEC, IGVD>::atlas_dbuct_manifest(
        IGVis& get_visits,
        ISVis& set_visits,
        IGVal& get_value,
        ISVal& set_value,
        IW&    walker,
        IRC&   rollout,
        IGEC&  exploration_constant,
        IGVD&  delta,
        IF     grant_k,
        INH    root)
    : walker(walker)
    , rollout(rollout)
    , exploration_constant(exploration_constant)
    , delta(delta)
    , grant(get_visits, grant_k)
    , frame_stack(monte_carlo::dbuct_frame<INH>(root, std::numeric_limits<size_t>::max()))
    , visit_adder(frame_stack, get_visits, set_visits)
    , frame_stack_controller(frame_stack, frame_stack, frame_stack, visit_adder)
    , value_stack(monte_carlo::dbuct_value_frame<INH, IF>(root))
    , value_adder(value_stack, get_value, set_value)
    , value_stack_controller(frame_stack_controller, frame_stack_controller,
                             value_stack, value_stack, value_stack, value_adder)
    , visit_creditor(visit_adder)
    , value_creditor(visit_creditor, value_stack, value_adder, this->delta)
    , policy(get_visits, get_value, this->walker, this->exploration_constant)
    , in_rollout()
    , chooser(get_visits, grant,
              value_stack_controller, frame_stack,
              this->walker, policy, this->rollout,
              in_rollout, in_rollout)
    , terminator(value_stack_controller, frame_stack, value_creditor, in_rollout)
{}

#endif
