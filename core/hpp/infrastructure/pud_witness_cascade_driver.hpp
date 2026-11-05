#ifndef PUD_WITNESS_CASCADE_DRIVER_HPP
#define PUD_WITNESS_CASCADE_DRIVER_HPP

#include <cstddef>
#include <deque>
#include <optional>
#include <vector>
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_eager_candidate.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_leaf_evaluation.hpp"
#include "value_objects/pud_unfold_outcome.hpp"

// Makes one unfold per call by witness cascade. A focus leaf dealt by the round robin is unfolded on its chosen goal,
// then each witness leaf of that goal's candidates on its own chosen goal, then theirs, down to the cascade depth:
// depth 0 unfolds focus leaves only. Every leaf is evaluated when it is visited; a leaf with a goal that has no
// candidate is refuted instead of unfolded, and the next leaf is visited. Queued leaves that can no longer be
// unfolded are passed over.
template<
    typename INextFocusLeaf,
    typename IIsUnfoldable,
    typename IEvaluateLeaf,
    typename IStoreEvaluation,
    typename IRefuteLeaf,
    typename IEraseRefuted,
    typename IChooseGoal,
    typename IUnfoldLeaf>
struct pud_witness_cascade_driver {
    pud_witness_cascade_driver(
        INextFocusLeaf& next_focus_leaf,
        IIsUnfoldable& is_unfoldable,
        IEvaluateLeaf& evaluate_leaf,
        IStoreEvaluation& store_evaluation,
        IRefuteLeaf& refute_leaf,
        IEraseRefuted& erase_refuted,
        IChooseGoal& choose_goal,
        IUnfoldLeaf& unfold_leaf,
        size_t cascade_depth);
    pud_unfold_outcome unfold_one();
private:
    struct cascade_visit {
        pud_global_node_id leaf;
        size_t level;
    };

    std::optional<cascade_visit> find_next_visit();

    INextFocusLeaf& next_focus_leaf_;
    IIsUnfoldable& is_unfoldable_;
    IEvaluateLeaf& evaluate_leaf_;
    IStoreEvaluation& store_evaluation_;
    IRefuteLeaf& refute_leaf_;
    IEraseRefuted& erase_refuted_;
    IChooseGoal& choose_goal_;
    IUnfoldLeaf& unfold_leaf_;
    size_t cascade_depth_;
    std::deque<cascade_visit> cascade_;
};

#define PUD_WITNESS_CASCADE_DRIVER_TEMPLATE                                                              \
    template<typename INextFocusLeaf, typename IIsUnfoldable, typename IEvaluateLeaf,                    \
             typename IStoreEvaluation, typename IRefuteLeaf, typename IEraseRefuted, typename IChooseGoal, \
             typename IUnfoldLeaf>
#define PUD_WITNESS_CASCADE_DRIVER                                                                       \
    pud_witness_cascade_driver<INextFocusLeaf, IIsUnfoldable, IEvaluateLeaf, IStoreEvaluation,           \
        IRefuteLeaf, IEraseRefuted, IChooseGoal, IUnfoldLeaf>

PUD_WITNESS_CASCADE_DRIVER_TEMPLATE
PUD_WITNESS_CASCADE_DRIVER::pud_witness_cascade_driver(
    INextFocusLeaf& next_focus_leaf,
    IIsUnfoldable& is_unfoldable,
    IEvaluateLeaf& evaluate_leaf,
    IStoreEvaluation& store_evaluation,
    IRefuteLeaf& refute_leaf,
    IEraseRefuted& erase_refuted,
    IChooseGoal& choose_goal,
    IUnfoldLeaf& unfold_leaf,
    size_t cascade_depth)
    : next_focus_leaf_(next_focus_leaf)
    , is_unfoldable_(is_unfoldable)
    , evaluate_leaf_(evaluate_leaf)
    , store_evaluation_(store_evaluation)
    , refute_leaf_(refute_leaf)
    , erase_refuted_(erase_refuted)
    , choose_goal_(choose_goal)
    , unfold_leaf_(unfold_leaf)
    , cascade_depth_(cascade_depth)
    , cascade_() {}

PUD_WITNESS_CASCADE_DRIVER_TEMPLATE
pud_unfold_outcome PUD_WITNESS_CASCADE_DRIVER::unfold_one() {
    for (std::optional<cascade_visit> visit = find_next_visit(); visit.has_value(); visit = find_next_visit()) {
        const pud_leaf_evaluation evaluation = evaluate_leaf_.evaluate(visit->leaf);
        if (evaluation.refuted) {
            erase_refuted_.erase_refuted(refute_leaf_.refute(visit->leaf));
            continue;
        }
        store_evaluation_.store(evaluation);
        const pud_body_goal_id goal = choose_goal_.choose(visit->leaf);
        const pud_unfold_outcome outcome = unfold_leaf_.unfold(visit->leaf, goal);
        if (visit->level >= cascade_depth_)
            return outcome;
        for (const pud_eager_candidate& candidate : evaluation.goals.at(goal))
            for (const pud_global_node_id witness : candidate.witness_leaves)
                cascade_.push_back(cascade_visit{.leaf = witness, .level = visit->level + 1});
        return outcome;
    }
    return pud_unfold_outcome::none;
}

// The next queued witness that can still be unfolded, else the next focus leaf, which starts a new cascade.
PUD_WITNESS_CASCADE_DRIVER_TEMPLATE
std::optional<typename PUD_WITNESS_CASCADE_DRIVER::cascade_visit> PUD_WITNESS_CASCADE_DRIVER::find_next_visit() {
    while (!cascade_.empty()) {
        const cascade_visit queued = cascade_.front();
        cascade_.pop_front();
        if (!is_unfoldable_.is_unfoldable(queued.leaf))
            continue;
        return queued;
    }
    const std::optional<pud_global_node_id> focus_leaf = next_focus_leaf_.next();
    if (!focus_leaf.has_value())
        return std::nullopt;
    return cascade_visit{.leaf = focus_leaf.value(), .level = 0};
}

#undef PUD_WITNESS_CASCADE_DRIVER
#undef PUD_WITNESS_CASCADE_DRIVER_TEMPLATE

#endif
