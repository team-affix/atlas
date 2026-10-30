#ifndef PUD_REPLAYER_HPP
#define PUD_REPLAYER_HPP

#include <cstdint>
#include <optional>
#include <utility>
#include "value_objects/expr.hpp"
#include "value_objects/moduled_expr.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_module_id.hpp"
#include "value_objects/rule.hpp"
#include "value_objects/pud_replay.hpp"
#include "value_objects/pud_substitution.hpp"
#include "value_objects/pud_ubs_state.hpp"

template<
    typename IGetAxiom,
    typename IGetInference,
    typename IMakeVar,
    typename IMakeBindMap,
    typename IMakeCycleDetector,
    typename IMakeUnifier>
struct pud_replayer {
    pud_replayer(
        IGetAxiom& get_axiom,
        IGetInference& get_inference,
        IMakeVar& make_var,
        IMakeBindMap& make_bind_map,
        IMakeCycleDetector& make_cycle_detector,
        IMakeUnifier& make_unifier);
    std::optional<pud_replay> enter(
        const pud_ubs_state& caller_ubs_state,
        const expr* goal,
        uint32_t frame_offset,
        pud_global_node_id axiom,
        pud_module_id caller_module) const;
    std::optional<pud_replay> descend(const pud_replay& replay, pud_global_node_id child) const;
private:
    IGetAxiom& get_axiom_;
    IGetInference& get_inference_;
    IMakeVar& make_var_;
    IMakeBindMap& make_bind_map_;
    IMakeCycleDetector& make_cycle_detector_;
    IMakeUnifier& make_unifier_;
};

#define PUD_REPLAYER_TEMPLATE                                                              \
    template<typename IGetAxiom, typename IGetInference, typename IMakeVar,  \
             typename IMakeBindMap, typename IMakeCycleDetector, typename IMakeUnifier>
#define PUD_REPLAYER                                                                       \
    pud_replayer<IGetAxiom, IGetInference, IMakeVar, IMakeBindMap,         \
        IMakeCycleDetector, IMakeUnifier>

PUD_REPLAYER_TEMPLATE
PUD_REPLAYER::pud_replayer(
    IGetAxiom& get_axiom,
    IGetInference& get_inference,
    IMakeVar& make_var,
    IMakeBindMap& make_bind_map,
    IMakeCycleDetector& make_cycle_detector,
    IMakeUnifier& make_unifier)
    : get_axiom_(get_axiom)
    , get_inference_(get_inference)
    , make_var_(make_var)
    , make_bind_map_(make_bind_map)
    , make_cycle_detector_(make_cycle_detector)
    , make_unifier_(make_unifier) {}

PUD_REPLAYER_TEMPLATE
std::optional<pud_replay> PUD_REPLAYER::enter(
    const pud_ubs_state& caller_ubs_state,
    const expr* goal,
    uint32_t frame_offset,
    pud_global_node_id axiom,
    pud_module_id caller_module) const {
    auto&& bind_map = make_bind_map_.make_bind_map(caller_ubs_state);
    auto&& cycle_detector = make_cycle_detector_.make_cycle_detector(bind_map);
    auto&& unifier = make_unifier_.make_unifier(bind_map, cycle_detector);
    const rule& axiom_rule = get_axiom_.get_axiom(axiom);
    if (!unifier.unify(
            moduled_expr{goal, 0, caller_module},
            moduled_expr{axiom_rule.head, frame_offset, axiom.module}).has_value())
        return std::nullopt;
    return pud_replay{.node = axiom, .frame_offset = frame_offset, .ubs_state = std::move(bind_map).persist()};
}

PUD_REPLAYER_TEMPLATE
std::optional<pud_replay> PUD_REPLAYER::descend(const pud_replay& replay, pud_global_node_id child) const {
    auto&& bind_map = make_bind_map_.make_bind_map(replay.ubs_state);
    auto&& cycle_detector = make_cycle_detector_.make_cycle_detector(bind_map);
    auto&& unifier = make_unifier_.make_unifier(bind_map, cycle_detector);
    for (const pud_substitution& substitution : get_inference_.get_inference(child).substitutions) {
        const moduled_expr variable{make_var_.make_var(substitution.var_idx), replay.frame_offset, child.module};
        const moduled_expr value{substitution.value, replay.frame_offset, child.module};
        if (!unifier.unify(variable, value).has_value())
            return std::nullopt;
    }
    return pud_replay{.node = child, .frame_offset = replay.frame_offset, .ubs_state = std::move(bind_map).persist()};
}

#undef PUD_REPLAYER
#undef PUD_REPLAYER_TEMPLATE

#endif
