#ifndef PUD_ROOT_DESCENT_MAKER_HPP
#define PUD_ROOT_DESCENT_MAKER_HPP

#include <vector>
#include "value_objects/expr.hpp"
#include "value_objects/pud_mhsw_head_id.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_ubs_state.hpp"

// Builds the base descent at a root, which opens a query for each of the root's body goals.
template<
    typename BaseDescent,
    typename BindingDescent,
    typename LvcDescent,
    typename NextBodyGoalIdDescent,
    typename BodyGoalExprDescent,
    typename StructureWatchDescent,
    typename BodyGoalPqDescent,
    typename IGetNodeAddedSubstitutions,
    typename IMakeVar,
    typename IMakeFunctor,
    typename IGetNodeAddedVarCount,
    typename IGetNodeAddedBodyGoals,
    typename IGetCallSite,
    typename IOpenQuery,
    typename ITryAddQueryHead,
    typename ITryForkQueryHead,
    typename IRemoveQueryHead,
    typename IStoreQueryHeadNode>
struct pud_root_descent_maker {
    pud_root_descent_maker(
        IGetNodeAddedSubstitutions& get_node_added_substitutions,
        IMakeVar& make_var,
        IMakeFunctor& make_functor,
        IGetNodeAddedVarCount& get_node_added_var_count,
        IGetNodeAddedBodyGoals& get_node_added_body_goals,
        IGetCallSite& get_call_site,
        IOpenQuery& open_query,
        ITryAddQueryHead& try_add_query_head,
        ITryForkQueryHead& try_fork_query_head,
        IRemoveQueryHead& remove_query_head,
        IStoreQueryHeadNode& store_query_head_node);
    BaseDescent make(pud_node_id root);
private:
    IGetNodeAddedSubstitutions& get_node_added_substitutions_;
    IMakeVar& make_var_;
    IMakeFunctor& make_functor_;
    IGetNodeAddedVarCount& get_node_added_var_count_;
    IGetNodeAddedBodyGoals& get_node_added_body_goals_;
    IGetCallSite& get_call_site_;
    IOpenQuery& open_query_;
    ITryAddQueryHead& try_add_query_head_;
    ITryForkQueryHead& try_fork_query_head_;
    IRemoveQueryHead& remove_query_head_;
    IStoreQueryHeadNode& store_query_head_node_;
};

#define PUD_ROOT_DESCENT_MAKER_TEMPLATE                                                    \
    template<typename BaseDescent, typename BindingDescent, typename LvcDescent,          \
             typename NextBodyGoalIdDescent, typename BodyGoalExprDescent,                  \
             typename StructureWatchDescent, typename BodyGoalPqDescent,                    \
             typename IGetNodeAddedSubstitutions, typename IMakeVar, typename IMakeFunctor, \
             typename IGetNodeAddedVarCount, typename IGetNodeAddedBodyGoals,               \
             typename IGetCallSite, typename IOpenQuery, typename ITryAddQueryHead,         \
             typename ITryForkQueryHead, typename IRemoveQueryHead,                         \
             typename IStoreQueryHeadNode>
#define PUD_ROOT_DESCENT_MAKER                                                             \
    pud_root_descent_maker<BaseDescent, BindingDescent, LvcDescent, NextBodyGoalIdDescent, \
        BodyGoalExprDescent, StructureWatchDescent, BodyGoalPqDescent,                     \
        IGetNodeAddedSubstitutions, IMakeVar, IMakeFunctor, IGetNodeAddedVarCount,          \
        IGetNodeAddedBodyGoals, IGetCallSite, IOpenQuery, ITryAddQueryHead,                 \
        ITryForkQueryHead, IRemoveQueryHead, IStoreQueryHeadNode>

PUD_ROOT_DESCENT_MAKER_TEMPLATE
PUD_ROOT_DESCENT_MAKER::pud_root_descent_maker(
    IGetNodeAddedSubstitutions& get_node_added_substitutions,
    IMakeVar& make_var,
    IMakeFunctor& make_functor,
    IGetNodeAddedVarCount& get_node_added_var_count,
    IGetNodeAddedBodyGoals& get_node_added_body_goals,
    IGetCallSite& get_call_site,
    IOpenQuery& open_query,
    ITryAddQueryHead& try_add_query_head,
    ITryForkQueryHead& try_fork_query_head,
    IRemoveQueryHead& remove_query_head,
    IStoreQueryHeadNode& store_query_head_node)
    : get_node_added_substitutions_(get_node_added_substitutions)
    , make_var_(make_var)
    , make_functor_(make_functor)
    , get_node_added_var_count_(get_node_added_var_count)
    , get_node_added_body_goals_(get_node_added_body_goals)
    , get_call_site_(get_call_site)
    , open_query_(open_query)
    , try_add_query_head_(try_add_query_head)
    , try_fork_query_head_(try_fork_query_head)
    , remove_query_head_(remove_query_head)
    , store_query_head_node_(store_query_head_node) {}

PUD_ROOT_DESCENT_MAKER_TEMPLATE
BaseDescent PUD_ROOT_DESCENT_MAKER::make(pud_node_id root) {
    const LvcDescent lvc{get_node_added_var_count_, root};
    const BodyGoalExprDescent body_goal_expr{get_node_added_body_goals_, get_call_site_, root};
    const std::vector<const expr*>& root_body_goals = get_node_added_body_goals_.get(root);
    std::vector<pud_mhsw_head_id> root_watch_head_ids;
    for (pud_mhsw_head_id watch_head_id = 0; watch_head_id < root_body_goals.size(); ++watch_head_id)
        root_watch_head_ids.push_back(watch_head_id);
    return BaseDescent{
        BindingDescent{get_node_added_substitutions_, make_var_, make_functor_},
        lvc,
        NextBodyGoalIdDescent{get_node_added_body_goals_, root},
        body_goal_expr,
        StructureWatchDescent{get_node_added_substitutions_, root_body_goals},
        BodyGoalPqDescent{
            open_query_,
            try_add_query_head_,
            try_fork_query_head_,
            remove_query_head_,
            get_call_site_,
            store_query_head_node_,
            root,
            body_goal_expr.body_goal_exprs(),
            pud_ubs_state{},
            lvc.lvc(),
            root_watch_head_ids},
    };
}

#undef PUD_ROOT_DESCENT_MAKER
#undef PUD_ROOT_DESCENT_MAKER_TEMPLATE

#endif
