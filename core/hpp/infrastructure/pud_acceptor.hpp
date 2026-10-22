#ifndef PUD_ACCEPTOR_HPP
#define PUD_ACCEPTOR_HPP

#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <vector>
#include "infrastructure/pud_transient_ubs_state.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/pud_child_delta.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_substitution.hpp"
#include "value_objects/pud_ubs_state.hpp"
#include "debug_assert.hpp"

template<
    typename BindMap,
    typename CycleDetector,
    typename Unifier,
    typename Logger,
    typename TranslatingNormalizer,
    typename IFindOpenBodyGoals,
    typename IGetNodeHead,
    typename IGetNodeAddedSubstitutions,
    typename IGlobalize,
    typename IMakeVar,
    typename IMakeFunctor,
    typename IMakeCycleDetector>
struct pud_acceptor {
    pud_acceptor(
        IFindOpenBodyGoals& find_open_body_goals,
        IGetNodeHead& get_node_head,
        IGetNodeAddedSubstitutions& get_node_added_substitutions,
        IGlobalize& globalize,
        IMakeVar& make_var,
        IMakeFunctor& make_functor,
        IMakeCycleDetector& make_cycle_detector);
    pud_child_delta accept(
        const pud_ubs_state& acceptor_ubs_state,
        uint32_t acceptor_lvc,
        const expr* call_site_goal,
        const std::vector<pud_node_id>& candidate_path);
private:
    IFindOpenBodyGoals& find_open_body_goals_;
    IGetNodeHead& get_node_head_;
    IGetNodeAddedSubstitutions& get_node_added_substitutions_;
    IGlobalize& globalize_;
    IMakeVar& make_var_;
    IMakeFunctor& make_functor_;
    IMakeCycleDetector& make_cycle_detector_;
};

#define PUD_ACCEPTOR_TEMPLATE                                                              \
    template<typename BindMap, typename CycleDetector, typename Unifier, typename Logger,     \
             typename TranslatingNormalizer, typename IFindOpenBodyGoals,                  \
             typename IGetNodeHead,                                                         \
             typename IGetNodeAddedSubstitutions, typename IGlobalize, typename IMakeVar,  \
             typename IMakeFunctor, typename IMakeCycleDetector>
#define PUD_ACCEPTOR                                                                       \
    pud_acceptor<BindMap, CycleDetector, Unifier, Logger, TranslatingNormalizer,              \
        IFindOpenBodyGoals, IGetNodeHead, IGetNodeAddedSubstitutions,       \
        IGlobalize, IMakeVar, IMakeFunctor, IMakeCycleDetector>

PUD_ACCEPTOR_TEMPLATE
PUD_ACCEPTOR::pud_acceptor(
    IFindOpenBodyGoals& find_open_body_goals,
    IGetNodeHead& get_node_head,
    IGetNodeAddedSubstitutions& get_node_added_substitutions,
    IGlobalize& globalize,
    IMakeVar& make_var,
    IMakeFunctor& make_functor,
    IMakeCycleDetector& make_cycle_detector)
    : find_open_body_goals_(find_open_body_goals)
    , get_node_head_(get_node_head)
    , get_node_added_substitutions_(get_node_added_substitutions)
    , globalize_(globalize)
    , make_var_(make_var)
    , make_functor_(make_functor)
    , make_cycle_detector_(make_cycle_detector) {}

PUD_ACCEPTOR_TEMPLATE
pud_child_delta PUD_ACCEPTOR::accept(
    const pud_ubs_state& acceptor_ubs_state,
    uint32_t acceptor_lvc,
    const expr* call_site_goal,
    const std::vector<pud_node_id>& candidate_path) {
    const std::vector<const expr*> open_body_goals = find_open_body_goals_.find_open_body_goals(candidate_path);

    pud_transient_ubs_state parent_ubs_state{acceptor_ubs_state};
    BindMap parent_bind_map = parent_ubs_state.make_bind_map();
    pud_transient_ubs_state transient_ubs_state{acceptor_ubs_state};
    BindMap bind_map = transient_ubs_state.make_bind_map();
    CycleDetector cycle_detector = make_cycle_detector_.make(bind_map);
    std::vector<uint32_t> touched_vars;
    Logger logger{bind_map, bind_map, touched_vars};
    Unifier unifier{globalize_, bind_map, bind_map, logger, logger, cycle_detector};

    const bool resolved = unifier.unify(
        framed_expr{call_site_goal, 0},
        framed_expr{get_node_head_.get(candidate_path.front()), acceptor_lvc});
    DEBUG_ASSERT(resolved);
    for (size_t path_index = 1; path_index < candidate_path.size(); ++path_index) {
        for (const pud_substitution& substitution : get_node_added_substitutions_.get(candidate_path.at(path_index))) {
            const bool replayed = unifier.unify(
                framed_expr{make_var_.make_var(substitution.var_idx), acceptor_lvc},
                framed_expr{substitution.value, acceptor_lvc});
            DEBUG_ASSERT(replayed);
        }
    }

    std::vector<uint32_t> changed_reps;
    for (uint32_t touched_var : touched_vars) {
        if (touched_var >= acceptor_lvc)
            continue;
        for (uint32_t candidate : {parent_bind_map.find_rep(touched_var), bind_map.find_rep(touched_var)}) {
            const bool was_bound = parent_bind_map.find_value(candidate).has_value();
            const bool changed = bind_map.find_rep(candidate) != candidate || bind_map.find_value(candidate).has_value();
            if (was_bound || !changed)
                continue;
            changed_reps.push_back(candidate);
        }
    }
    std::sort(changed_reps.begin(), changed_reps.end());
    changed_reps.erase(std::unique(changed_reps.begin(), changed_reps.end()), changed_reps.end());

    TranslatingNormalizer translating_normalizer{globalize_, make_functor_, make_var_, bind_map, bind_map};
    std::unordered_map<uint32_t, uint32_t> translation;
    pud_child_delta delta{.substitutions = {}, .body_goals = {}, .var_count = 0};
    for (uint32_t changed_rep : changed_reps) {
        delta.substitutions.push_back(pud_substitution{
            .var_idx = changed_rep,
            .value = translating_normalizer.normalize(
                framed_expr{make_var_.make_var(changed_rep), 0}, acceptor_lvc, translation),
        });
    }
    for (const expr* body_goal : open_body_goals)
        delta.body_goals.push_back(
            translating_normalizer.normalize(framed_expr{body_goal, acceptor_lvc}, acceptor_lvc, translation));
    delta.var_count = static_cast<uint32_t>(translation.size());
    return delta;
}

#undef PUD_ACCEPTOR
#undef PUD_ACCEPTOR_TEMPLATE

#endif
