#ifndef DBUCT_SIM_HPP
#define DBUCT_SIM_HPP

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <random>
#include <vector>
#include "value_objects/lineage.hpp"
#include "value_objects/mcts_choice.hpp"
#include "value_objects/mcts_node_id.hpp"
#include "dbuct.hpp"
#include "visits_table.hpp"
#include "value_table.hpp"
#include "dispatches_table.hpp"
#include "linear_batch_increment.hpp"
#include "random_rollout.hpp"

// Delayed-backtracking counterpart of mcts_sim. Constructs a single
// monte_carlo::dbuct on the MCT root for the whole solve (episodes camp deep in
// the tree) and keeps the trail's frame stack and the CDCL learner's own frame
// stack in lockstep: exactly one frame per MCTS choose, whether that choose is a
// tree-policy step or a rollout step. dbuct itself does NOT grow during rollout,
// so an episode ends with the trail/CDCL deeper than dbuct; terminate() simply
// pops the excess back down to the stack size dbuct reports. Because both counters
// are pushed and popped together and both start at 1 (the base frame here, the
// learner's ctor root frame there), trail.depth() equals the learner's frame
// depth at all times -- which is why the avoidance-unit-boundary oracle can read
// trail.depth() directly. Nothing here inspects or cares whether dbuct is in
// rollout.
template<typename ITrail, typename IMakeResolutionLineage, typename IFrameControl>
struct dbuct_sim {
    dbuct_sim(ITrail&, IMakeResolutionLineage&, IFrameControl&,
              std::mt19937&, double exploration_constant,
              std::size_t grant_increment_interval);

    mcts_choice choose(const std::vector<mcts_choice>&);
    // A terminal must not leave its spent frontier in place -- the next episode
    // would re-detect it verbatim -- so terminate always backtracks at least one
    // frame, asking dbuct to force a backstep (max_return_depth = current depth
    // minus one) unless we are already at the root, where nothing is left to pop.
    // Returns the forced eliminations the final CDCL frame pop surfaced at the
    // resume frontier.
    std::vector<const resolution_lineage*> terminate(double reward);

    // Whether the camped path is back at the root: the base frame is the only
    // frame left. dbuct_sim owns this convention -- it pushed the single base
    // frame in its ctor -- so it, rather than the caller, maps trail depth to
    // "at root". The solver uses this both to bound its backstep loops (the root
    // is the floor) and to gate refutation (a zero-decision run only proves the
    // search exhausted when it started at the root).
    bool at_root() const { return trail_.depth() == 1; }

    // Undo all trail-journalled work back to the pristine pre-activation state.
    // The solver calls this once, on refutation, so an exhausted solve leaves the
    // clean initial state a restarting solver would (a refuted session normalizes
    // its query vars back to unbound, including bindings from root-frontier unit
    // resolutions that never opened a choose frame). CDCL's own ctor root frame is
    // left intact -- the base trail frame mirrors it but was pushed without a
    // matching push_frame, so it is popped trail-only.
    void unwind_to_root();

private:
    using decision_set_t = mcts_node_id::first_type;

    struct walker {
        IMakeResolutionLineage& make_resolution_lineage_;
        explicit walker(IMakeResolutionLineage& mrl) : make_resolution_lineage_(mrl) {}
        mcts_node_id walk(const mcts_node_id& node, const mcts_choice& choice) const {
            if (const goal_lineage* const* gl_ptr =
                    std::get_if<const goal_lineage*>(&choice)) {
                return {node.first, *gl_ptr};
            }
            const resolution_lineage* rl =
                make_resolution_lineage_.make_resolution_lineage(
                    node.second, std::get<rule_id>(choice));
            decision_set_t next_set = node.first;
            next_set.insert(rl);
            return {std::move(next_set), nullptr};
        }
    };

    using visits_table_t     = monte_carlo::visits_table<mcts_node_id, std::map>;
    using value_table_t      = monte_carlo::value_table<mcts_node_id, double, std::map>;
    using dispatches_table_t = monte_carlo::dispatches_table<mcts_node_id, std::map>;
    using choices_t          = std::vector<mcts_choice>;
    using rollout_t          = monte_carlo::random_rollout<
                                   mcts_choice, std::mt19937, choices_t, choices_t>;
    using batch_t            = monte_carlo::linear_batch_increment;
    using dbuct_t            = monte_carlo::dbuct<
                                   mcts_node_id,
                                   mcts_choice,
                                   double,
                                   visits_table_t,
                                   value_table_t,
                                   visits_table_t,
                                   value_table_t,
                                   dispatches_table_t,
                                   dispatches_table_t,
                                   batch_t,
                                   walker,
                                   choices_t,
                                   choices_t,
                                   rollout_t>;

    ITrail&                trail_;
    IFrameControl&         frames_;
    visits_table_t         visits_table_;
    value_table_t          value_table_;
    dispatches_table_t     dispatches_table_;
    batch_t                batch_;
    walker                 walker_;
    rollout_t              rollout_;
    std::optional<dbuct_t> dbuct_;
};

template<typename ITrail, typename IMRL, typename IFC>
dbuct_sim<ITrail, IMRL, IFC>::dbuct_sim(ITrail& trail, IMRL& mrl, IFC& frames,
                                std::mt19937& rng,
                                double ec, std::size_t grant_increment_interval)
    : trail_(trail)
    , frames_(frames)
    , visits_table_()
    , value_table_()
    , dispatches_table_()
    , batch_(grant_increment_interval)
    , walker_(mrl)
    , rollout_(rng)
    , dbuct_(std::nullopt)
{
    dbuct_.emplace(visits_table_, value_table_, visits_table_, value_table_,
                   dispatches_table_, dispatches_table_, batch_,
                   walker_, rollout_,
                   mcts_node_id{decision_set_t{}, nullptr},
                   ec);
    // Base frame: the trail's mirror of the permanent root that dbuct's stack and
    // the CDCL learner's frame stack already hold. It keeps trail.depth() equal to
    // the learner's frame depth (both start at 1) and is never popped during a
    // solve, so trail.depth() == 1 means "at root".
    trail_.push();
}

template<typename ITrail, typename IMRL, typename IFC>
mcts_choice dbuct_sim<ITrail, IMRL, IFC>::choose(const std::vector<mcts_choice>& choices) {
    mcts_choice chosen = dbuct_->choose(choices, choices);
    // One trail frame + one matching CDCL frame per choose, unconditionally. Any
    // excess (rollout steps, which dbuct does not track) is reconciled away in
    // terminate() by popping down to the stack size dbuct reports.
    trail_.push();
    frames_.push_frame();
    return chosen;
}

template<typename ITrail, typename IMRL, typename IFC>
std::vector<const resolution_lineage*> dbuct_sim<ITrail, IMRL, IFC>::terminate(double reward) {
    // Force at least one backstep: cap dbuct's return at one below the current
    // depth so it must pop a frame, unless we are already at the root (depth 1),
    // where there is nothing left to pop -- passing SIZE_MAX there disables the
    // cap (and avoids the undefined depth-1 == 0 argument). The trail depth is
    // never below dbuct's own stack (rollout pushes trail-only excess), so the
    // cap only bites on a no-rollout terminal; a rollout terminal already resumes
    // shallower once its throwaway frames pop.
    const std::size_t max_return_depth =
        trail_.depth() > 1 ? trail_.depth() - 1 : SIZE_MAX;
    // dbuct returns the frame stack size it backtracked TO (root only == 1). The
    // trail's depth mirrors that stack -- both start at 1 (the base frame here,
    // dbuct's root there) -- so restoring is just popping the trail down to that
    // size directly, no off-by-one. Each popped CDCL frame surfaces the
    // eliminations still forced past its boundary. Only the FINAL pop -- the one
    // landing at the resume depth -- yields eliminations valid at the resume
    // frontier; earlier pops emit conflicts still unit at THAT (deeper) level
    // which CDCL re-arms as watched clauses as we pop past their boundaries, so
    // routing them at the shallower resume frontier would over-eliminate.
    // Clearing before each pop keeps exactly the resume-level set.
    const std::size_t stack_size = dbuct_->terminate(reward, max_return_depth);
    std::vector<const resolution_lineage*> eliminations;
    while (trail_.depth() > stack_size) {
        eliminations.clear();
        trail_.pop();
        auto sm = frames_.pop_frame();
        while (!sm.done()) {
            sm.resume();
            if (sm.has_yield())
                eliminations.push_back(sm.consume_yield());
        }
    }
    return eliminations;
}

template<typename ITrail, typename IMRL, typename IFC>
void dbuct_sim<ITrail, IMRL, IFC>::unwind_to_root() {
    // Pop every episode frame (each paired with its matching CDCL frame).
    while (trail_.depth() > 1) {
        trail_.pop();
        auto sm = frames_.pop_frame();
        while (!sm.done()) {
            sm.resume();
            if (sm.has_yield())
                sm.consume_yield();
        }
    }
    // Then the base trail frame itself, undoing the initial-goal activation and
    // any root-frontier unit resolutions. It has no matching CDCL frame (it
    // mirrors CDCL's permanent ctor root, which is left in place), so pop the
    // trail only.
    if (trail_.depth() > 0)
        trail_.pop();
}

#endif
