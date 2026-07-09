#ifndef DBUCT_SIM_HPP
#define DBUCT_SIM_HPP

#include <cstddef>
#include <map>
#include <optional>
#include <random>
#include <vector>
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
// the tree) and keeps the trail's frame stack in lockstep with DBUCT's own: each
// tree-policy choose() pushes one trail frame, entering rollout pushes one, and
// terminate() restores the trail to the frame index DBUCT backtracked TO by
// popping down to that depth. Because DBUCT reports the target index directly,
// no per-episode frame counting is needed -- trail.depth() is the single source
// of truth. The reward passes straight through to DBUCT, which backpropagates it
// up the camped path.
template<typename ITrail, typename IMakeResolutionLineage>
struct dbuct_sim {
    dbuct_sim(ITrail&, IMakeResolutionLineage&, std::mt19937&,
              double exploration_constant, std::size_t grant_increment_interval);

    mcts_choice choose(const std::vector<mcts_choice>&);
    std::size_t terminate(double reward);
    bool in_rollout() const { return dbuct_->in_rollout(); }

    // Trail frames held below the root for the current camped path (excludes the
    // root frame and any transient rollout frame). Behavioral camp-depth probe.
    std::size_t camp_depth() const {
        const std::size_t base = root_depth_ + 1 + (dbuct_->in_rollout() ? 1 : 0);
        const std::size_t d = trail_.depth();
        return d > base ? d - base : 0;
    }

    void mark_root() { root_depth_ = trail_.depth(); trail_.push(); }
    void restore_root() { while (trail_.depth() > root_depth_) trail_.pop(); }

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
    std::size_t            root_depth_ = 0;
    visits_table_t         visits_table_;
    value_table_t          value_table_;
    dispatches_table_t     dispatches_table_;
    batch_t                batch_;
    walker                 walker_;
    rollout_t              rollout_;
    std::optional<dbuct_t> dbuct_;
};

template<typename ITrail, typename IMRL>
dbuct_sim<ITrail, IMRL>::dbuct_sim(ITrail& trail, IMRL& mrl, std::mt19937& rng,
                                double ec, std::size_t grant_increment_interval)
    : trail_(trail)
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

template<typename ITrail, typename IMRL>
mcts_choice dbuct_sim<ITrail, IMRL>::choose(const std::vector<mcts_choice>& choices) {
    const bool was_rollout = dbuct_->in_rollout();
    mcts_choice chosen = dbuct_->choose(choices, choices);
    // A tree-policy choose opens a tree frame; the first choose that flips into
    // rollout opens a single rollout frame that later rollout chooses reuse.
    if (!dbuct_->in_rollout() || !was_rollout) {
        trail_.push();
    }
    return chosen;
}

template<typename ITrail, typename IMRL>
std::size_t dbuct_sim<ITrail, IMRL>::terminate(double reward) {
    // DBUCT returns the 0-based frame index it backtracked TO (root = 0). The
    // trail holds the root frame plus one frame per tree choose (and possibly a
    // transient rollout frame on top), so restoring is just popping down to the
    // depth that leaves exactly `idx` tree frames.
    const std::size_t idx = dbuct_->terminate(reward);
    while (trail_.depth() > root_depth_ + 1 + idx)
        trail_.pop();
    return idx;
}

#endif
