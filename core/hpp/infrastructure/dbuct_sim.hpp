#ifndef DBUCT_SIM_HPP
#define DBUCT_SIM_HPP

#include <cstddef>
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
// the tree) and keeps three stacks in lockstep, one frame per tree-policy choose
// (plus one transient rollout frame):
//   * the trail's frame stack (rolls back the trail-journalled dbuct_* state),
//   * the CDCL learner's own frame stack (push_frame/pop_frame), and
//   * a plain frame counter that mirrors the learner's stack size for the
//     avoidance-unit-boundary oracle.
// terminate() restores to the frame index DBUCT backtracked TO by popping each
// stack down to that depth; because a CDCL pop_frame emits the still-forced
// eliminations sitting in the popped frame, terminate() (and restore_root())
// drain those and hand them back so the solver can route them at the resume
// frontier. The reward passes straight through to DBUCT.
template<typename ITrail, typename IMakeResolutionLineage,
         typename IFrameControl, typename IFrameCount>
struct dbuct_sim {
    dbuct_sim(ITrail&, IMakeResolutionLineage&, IFrameControl&, IFrameCount&,
              std::mt19937&, double exploration_constant,
              std::size_t grant_increment_interval);

    mcts_choice choose(const std::vector<mcts_choice>&);
    // force_progress: when the episode ended in a conflict, the resume frontier
    // must not be left holding the dead-end simulation work, so we keep reporting
    // the episode (accruing visits until a frame budget is spent) until at least
    // one frame is actually popped. A solved/depth-exceeded episode leaves a valid
    // frontier, so a zero-pop backtrack there is fine.
    std::vector<const resolution_lineage*> terminate(double reward, bool force_progress);
    bool in_rollout() const { return dbuct_->in_rollout(); }

    // Trail frames held below the root for the current camped path (excludes the
    // root frame and any transient rollout frame). Behavioral camp-depth probe.
    std::size_t camp_depth() const {
        const std::size_t base = root_depth_ + 1 + (dbuct_->in_rollout() ? 1 : 0);
        const std::size_t d = trail_.depth();
        return d > base ? d - base : 0;
    }

    void mark_root() { root_depth_ = trail_.depth(); trail_.push(); }
    void restore_root();

private:
    // Pop one camp frame off all three stacks. The trail frame rolls back the
    // trail-journalled state; the CDCL frame pop yields the eliminations still
    // forced past its boundary (appended to `out` when non-null); the counter
    // mirrors the learner's stack size.
    void pop_camp_frame(std::vector<const resolution_lineage*>* out);

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
    IFrameCount&           count_;
    std::size_t            root_depth_ = 0;
    visits_table_t         visits_table_;
    value_table_t          value_table_;
    dispatches_table_t     dispatches_table_;
    batch_t                batch_;
    walker                 walker_;
    rollout_t              rollout_;
    std::optional<dbuct_t> dbuct_;
};

template<typename ITrail, typename IMRL, typename IFC, typename IFN>
dbuct_sim<ITrail, IMRL, IFC, IFN>::dbuct_sim(ITrail& trail, IMRL& mrl, IFC& frames,
                                IFN& count, std::mt19937& rng,
                                double ec, std::size_t grant_increment_interval)
    : trail_(trail)
    , frames_(frames)
    , count_(count)
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
}

template<typename ITrail, typename IMRL, typename IFC, typename IFN>
mcts_choice dbuct_sim<ITrail, IMRL, IFC, IFN>::choose(const std::vector<mcts_choice>& choices) {
    const bool was_rollout = dbuct_->in_rollout();
    mcts_choice chosen = dbuct_->choose(choices, choices);
    // A tree-policy choose opens a tree frame; the first choose that flips into
    // rollout opens a single rollout frame that later rollout chooses reuse. Each
    // opened trail frame gets a matching CDCL frame and counter bump so all three
    // stacks stay 1:1.
    if (!dbuct_->in_rollout() || !was_rollout) {
        trail_.push();
        frames_.push_frame();
        count_.push();
    }
    return chosen;
}

template<typename ITrail, typename IMRL, typename IFC, typename IFN>
void dbuct_sim<ITrail, IMRL, IFC, IFN>::pop_camp_frame(std::vector<const resolution_lineage*>* out) {
    trail_.pop();
    auto sm = frames_.pop_frame();
    while (!sm.done()) {
        sm.resume();
        if (sm.has_yield()) {
            const resolution_lineage* elim = sm.consume_yield();
            if (out)
                out->push_back(elim);
        }
    }
    count_.pop();
}

template<typename ITrail, typename IMRL, typename IFC, typename IFN>
std::vector<const resolution_lineage*> dbuct_sim<ITrail, IMRL, IFC, IFN>::terminate(double reward, bool force_progress) {
    // DBUCT returns the 0-based frame index it backtracked TO (root = 0). The
    // trail holds the root frame plus one frame per tree choose (and possibly a
    // transient rollout frame on top), so restoring is just popping down to the
    // depth that leaves exactly `idx` tree frames. Each popped CDCL frame surfaces
    // the eliminations still forced past its boundary, which we return for routing.
    // Only the FINAL pop -- the one that lands us at the resume depth -- surfaces
    // eliminations valid at the resume frontier. Each intermediate pop emits the
    // conflicts still unit at THAT (deeper) level; as we keep popping past their
    // boundaries CDCL re-arms them as watched clauses (they later fire through
    // constrain), so routing those intermediate emissions at the shallower resume
    // frontier would over-eliminate. Clearing before each pop keeps exactly the
    // resume-level set.
    std::vector<const resolution_lineage*> eliminations;
    const std::size_t start_depth = trail_.depth();
    while (true) {
        const std::size_t idx = dbuct_->terminate(reward);
        while (trail_.depth() > root_depth_ + 1 + idx) {
            eliminations.clear();
            pop_camp_frame(&eliminations);
        }
        // Stop once we've made progress, reached the root, or the caller is fine
        // with a zero-pop backtrack (non-conflict terminal).
        if (!force_progress || trail_.depth() < start_depth || trail_.depth() <= root_depth_ + 1)
            break;
    }
    return eliminations;
}

template<typename ITrail, typename IMRL, typename IFC, typename IFN>
void dbuct_sim<ITrail, IMRL, IFC, IFN>::restore_root() {
    // Tear the whole camped path down: pop every camp frame (draining/ignoring
    // its CDCL eliminations -- the solve is ending), then the bare trail root
    // frame. The root has no CDCL frame (the learner's ctor root stays), so the
    // counter settles back at 1.
    while (trail_.depth() > root_depth_ + 1)
        pop_camp_frame(nullptr);
    if (trail_.depth() > root_depth_)
        trail_.pop();
}

#endif
