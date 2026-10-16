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

// Resolves the acceptor's call site goal against a candidate: the candidate's root head and
// path substitutions are replayed at the acceptor's variable count, and the acceptor
// representatives this touches become the child's substitutions. Everything the child stores
// is fully normalized in the child's environment, its fresh variables numbered from the
// acceptor's variable count.
template<
    typename BindMap,
    typename Normalizer,
    typename Unifier,
    typename Logger,
    typename TranslatingNormalizer,
    typename IFindNodePath,
    typename IFindOpenBodyGoals,
    typename IGetNodeHead,
    typename IGetNodeAddedSubstitutions,
    typename IGlobalize,
    typename IMakeVar,
    typename IMakeFunctor,
    typename IMakeNormalizer>
struct pud_acceptor {
    pud_acceptor(
        IFindNodePath& find_node_path,
        IFindOpenBodyGoals& find_open_body_goals,
        IGetNodeHead& get_node_head,
        IGetNodeAddedSubstitutions& get_node_added_substitutions,
        IGlobalize& globalize,
        IMakeVar& make_var,
        IMakeFunctor& make_functor,
        IMakeNormalizer& make_normalizer);
    pud_child_delta accept(
        const pud_ubs_state& acceptor_ubs_state,
        uint32_t acceptor_lvc,
        const expr* call_site_goal,
        pud_node_id candidate);
private:
    IFindNodePath& find_node_path_;
    IFindOpenBodyGoals& find_open_body_goals_;
    IGetNodeHead& get_node_head_;
    IGetNodeAddedSubstitutions& get_node_added_substitutions_;
    IGlobalize& globalize_;
    IMakeVar& make_var_;
    IMakeFunctor& make_functor_;
    IMakeNormalizer& make_normalizer_;
};

#define PUD_ACCEPTOR_TEMPLATE                                                              \
    template<typename BindMap, typename Normalizer, typename Unifier, typename Logger,     \
             typename TranslatingNormalizer, typename IFindNodePath,                       \
             typename IFindOpenBodyGoals, typename IGetNodeHead,                           \
             typename IGetNodeAddedSubstitutions, typename IGlobalize, typename IMakeVar,  \
             typename IMakeFunctor, typename IMakeNormalizer>
#define PUD_ACCEPTOR                                                                       \
    pud_acceptor<BindMap, Normalizer, Unifier, Logger, TranslatingNormalizer,              \
        IFindNodePath, IFindOpenBodyGoals, IGetNodeHead, IGetNodeAddedSubstitutions,       \
        IGlobalize, IMakeVar, IMakeFunctor, IMakeNormalizer>

PUD_ACCEPTOR_TEMPLATE
PUD_ACCEPTOR::pud_acceptor(
    IFindNodePath& find_node_path,
    IFindOpenBodyGoals& find_open_body_goals,
    IGetNodeHead& get_node_head,
    IGetNodeAddedSubstitutions& get_node_added_substitutions,
    IGlobalize& globalize,
    IMakeVar& make_var,
    IMakeFunctor& make_functor,
    IMakeNormalizer& make_normalizer)
    : find_node_path_(find_node_path)
    , find_open_body_goals_(find_open_body_goals)
    , get_node_head_(get_node_head)
    , get_node_added_substitutions_(get_node_added_substitutions)
    , globalize_(globalize)
    , make_var_(make_var)
    , make_functor_(make_functor)
    , make_normalizer_(make_normalizer) {}

PUD_ACCEPTOR_TEMPLATE
pud_child_delta PUD_ACCEPTOR::accept(
    const pud_ubs_state& acceptor_ubs_state,
    uint32_t acceptor_lvc,
    const expr* call_site_goal,
    pud_node_id candidate) {
    const std::vector<pud_node_id> path = find_node_path_.find_path(candidate);
    const std::vector<const expr*> open_body_goals = find_open_body_goals_.find_open_body_goals(path);

    pud_transient_ubs_state transient_ubs_state{acceptor_ubs_state};
    BindMap bind_map = transient_ubs_state.make_bind_map(make_var_, make_functor_);
    Normalizer normalizer = make_normalizer_.make(bind_map);
    std::vector<uint32_t> changed_reps;
    Logger logger{bind_map, bind_map, changed_reps};
    Unifier unifier{globalize_, normalizer, bind_map, logger, logger};

    const bool resolved = unifier.unify(
        framed_expr{call_site_goal, 0},
        framed_expr{get_node_head_.get(path.front()), acceptor_lvc});
    DEBUG_ASSERT(resolved);
    for (size_t path_index = 1; path_index < path.size(); ++path_index) {
        for (const pud_substitution& substitution : get_node_added_substitutions_.get(path.at(path_index))) {
            const bool replayed = unifier.unify(
                framed_expr{make_var_.make_var(substitution.var_idx), acceptor_lvc},
                framed_expr{substitution.value, acceptor_lvc});
            DEBUG_ASSERT(replayed);
        }
    }

    std::sort(changed_reps.begin(), changed_reps.end());
    changed_reps.erase(std::unique(changed_reps.begin(), changed_reps.end()), changed_reps.end());

    TranslatingNormalizer translating_normalizer{globalize_, make_functor_, make_var_, bind_map};
    std::unordered_map<uint32_t, uint32_t> translation;
    pud_child_delta delta{.substitutions = {}, .body_goals = {}, .var_count = 0};
    for (uint32_t changed_rep : changed_reps) {
        if (changed_rep >= acceptor_lvc)
            continue;
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
