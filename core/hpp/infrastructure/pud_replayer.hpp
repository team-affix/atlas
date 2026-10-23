#ifndef PUD_REPLAYER_HPP
#define PUD_REPLAYER_HPP

#include <cstdint>
#include <optional>
#include <utility>
#include "value_objects/expr.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_replay.hpp"
#include "value_objects/pud_substitution.hpp"
#include "value_objects/pud_ubs_state.hpp"

// Walks a query down the database. Entering an axiom unifies the goal with the axiom's head; stepping
// to a child replays the child's substitutions. Either fails when a unification fails.
template<
    typename IGetRootHead,
    typename IGetNodeAddedSubstitutions,
    typename IMakeVar,
    typename IMakeBindMap,
    typename IMakeCycleDetector,
    typename IMakeUnifier>
struct pud_replayer {
    pud_replayer(
        IGetRootHead& get_root_head,
        IGetNodeAddedSubstitutions& get_node_added_substitutions,
        IMakeVar& make_var,
        IMakeBindMap& make_bind_map,
        IMakeCycleDetector& make_cycle_detector,
        IMakeUnifier& make_unifier);
    std::optional<pud_replay> enter(
        const pud_ubs_state& caller_ubs_state,
        const expr* goal,
        uint32_t frame_offset,
        pud_node_id axiom) const;
    std::optional<pud_replay> descend(const pud_replay& replay, pud_node_id child) const;
private:
    IGetRootHead& get_root_head_;
    IGetNodeAddedSubstitutions& get_node_added_substitutions_;
    IMakeVar& make_var_;
    IMakeBindMap& make_bind_map_;
    IMakeCycleDetector& make_cycle_detector_;
    IMakeUnifier& make_unifier_;
};

#define PUD_REPLAYER_TEMPLATE                                                              \
    template<typename IGetRootHead, typename IGetNodeAddedSubstitutions, typename IMakeVar,  \
             typename IMakeBindMap, typename IMakeCycleDetector, typename IMakeUnifier>
#define PUD_REPLAYER                                                                       \
    pud_replayer<IGetRootHead, IGetNodeAddedSubstitutions, IMakeVar, IMakeBindMap,         \
        IMakeCycleDetector, IMakeUnifier>

PUD_REPLAYER_TEMPLATE
PUD_REPLAYER::pud_replayer(
    IGetRootHead& get_root_head,
    IGetNodeAddedSubstitutions& get_node_added_substitutions,
    IMakeVar& make_var,
    IMakeBindMap& make_bind_map,
    IMakeCycleDetector& make_cycle_detector,
    IMakeUnifier& make_unifier)
    : get_root_head_(get_root_head)
    , get_node_added_substitutions_(get_node_added_substitutions)
    , make_var_(make_var)
    , make_bind_map_(make_bind_map)
    , make_cycle_detector_(make_cycle_detector)
    , make_unifier_(make_unifier) {}

PUD_REPLAYER_TEMPLATE
std::optional<pud_replay> PUD_REPLAYER::enter(
    const pud_ubs_state& caller_ubs_state,
    const expr* goal,
    uint32_t frame_offset,
    pud_node_id axiom) const {
    auto&& bind_map = make_bind_map_.make_bind_map(caller_ubs_state);
    auto&& cycle_detector = make_cycle_detector_.make_cycle_detector(bind_map);
    auto&& unifier = make_unifier_.make_unifier(bind_map, cycle_detector);
    if (!unifier.unify(framed_expr{goal, 0}, framed_expr{get_root_head_.get(axiom), frame_offset}))
        return std::nullopt;
    return pud_replay{.node = axiom, .frame_offset = frame_offset, .ubs_state = std::move(bind_map).persist()};
}

PUD_REPLAYER_TEMPLATE
std::optional<pud_replay> PUD_REPLAYER::descend(const pud_replay& replay, pud_node_id child) const {
    auto&& bind_map = make_bind_map_.make_bind_map(replay.ubs_state);
    auto&& cycle_detector = make_cycle_detector_.make_cycle_detector(bind_map);
    auto&& unifier = make_unifier_.make_unifier(bind_map, cycle_detector);
    for (const pud_substitution& substitution : get_node_added_substitutions_.get(child)) {
        const framed_expr lhs{make_var_.make_var(substitution.var_idx), replay.frame_offset};
        const framed_expr rhs{substitution.value, replay.frame_offset};
        if (!unifier.unify(lhs, rhs))
            return std::nullopt;
    }
    return pud_replay{.node = child, .frame_offset = replay.frame_offset, .ubs_state = std::move(bind_map).persist()};
}

#undef PUD_REPLAYER
#undef PUD_REPLAYER_TEMPLATE

#endif
