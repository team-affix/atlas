#ifndef ATLAS_UCT_MANIFEST_HPP
#define ATLAS_UCT_MANIFEST_HPP

#include "infrastructure/in_rollout_flag.hpp"
#include "infrastructure/ucb1.hpp"
#include "infrastructure/uct_backprop_path.hpp"
#include "infrastructure/uct_chooser.hpp"
#include "infrastructure/uct_cursor.hpp"
#include "infrastructure/uct_terminator.hpp"
#include "infrastructure/uct_value_creditor.hpp"
#include "infrastructure/uct_visit_creditor.hpp"
#include "infrastructure/uniform_value_update.hpp"

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
struct atlas_uct_manifest
{
    using value_update_t   = monte_carlo::uniform_value_update<INodeHandle, IGetValue, ISetValue, IGetValueDelta>;
    using cursor_t         = monte_carlo::uct_cursor<INodeHandle>;
    using path_t           = monte_carlo::uct_backprop_path<INodeHandle>;
    using visit_creditor_t = monte_carlo::uct_visit_creditor<INodeHandle, path_t, IGetVisits, ISetVisits>;
    using value_creditor_t = monte_carlo::uct_value_creditor<visit_creditor_t, path_t, value_update_t>;
    using policy_t         = monte_carlo::ucb1<INodeHandle, IChoice, IFloat,
                                  IGetVisits, IGetValue, IWalker,
                                  IGetExplorationConstant, IGetChoiceCount, IGetChoiceAt>;
    using chooser_t        = monte_carlo::uct_chooser<INodeHandle, IChoice, IGetVisits, IWalker,
                                         IGetChoiceCount, IGetChoiceAt,
                                         policy_t, IRolloutChoose,
                                         cursor_t, cursor_t, path_t,
                                         monte_carlo::in_rollout_flag, monte_carlo::in_rollout_flag>;
    using terminator_t     = monte_carlo::uct_terminator<INodeHandle,
                                            path_t,
                                            value_creditor_t,
                                            path_t, path_t,
                                            cursor_t,
                                            monte_carlo::in_rollout_flag>;

    atlas_uct_manifest(IGetVisits&,
                       ISetVisits&,
                       IGetValue&,
                       ISetValue&,
                       IWalker&,
                       IRolloutChoose&,
                       IGetExplorationConstant&,
                       IGetValueDelta&,
                       INodeHandle root);

    IWalker&                         walker;
    IRolloutChoose&                  rollout;
    IGetExplorationConstant&         exploration_constant;
    IGetValueDelta&                  delta;
    value_update_t                   value_update;
    cursor_t                         cursor;
    path_t                           backprop_path;
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
atlas_uct_manifest<INH, IC, IF, IGVis, ISVis, IGVal, ISVal, IW, IGCC, IGCA, IRC, IGEC, IGVD>::atlas_uct_manifest(
        IGVis& get_visits,
        ISVis& set_visits,
        IGVal& get_value,
        ISVal& set_value,
        IW&    walker,
        IRC&   rollout,
        IGEC&  exploration_constant,
        IGVD&  delta,
        INH    root)
    : walker(walker)
    , rollout(rollout)
    , exploration_constant(exploration_constant)
    , delta(delta)
    , value_update(get_value, set_value, this->delta)
    , cursor(root)
    , backprop_path(root)
    , visit_creditor(backprop_path, get_visits, set_visits)
    , value_creditor(visit_creditor, backprop_path, value_update)
    , policy(get_visits, get_value, this->walker, this->exploration_constant)
    , in_rollout()
    , chooser(get_visits, this->walker, policy, this->rollout,
              cursor, cursor, backprop_path,
              in_rollout, in_rollout)
    , terminator(backprop_path, value_creditor, backprop_path, backprop_path,
                 cursor, in_rollout, root)
{}

#endif
