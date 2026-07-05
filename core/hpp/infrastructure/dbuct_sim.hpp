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

// Delayed-backtracking counterpart of mcts_sim.
//
// Unlike mcts_sim (which recreates monte_carlo::sim per episode and always
// restarts from the root), dbuct_sim constructs a single monte_carlo::dbuct on
// the common MCT root for the entire solve. Episodes camp deep in the tree;
// terminate(reward) reports how many tree-policy choice frames DBUCT unwound,
// and dbuct_sim keeps the caller's checkpoint stack in lockstep:
//   * each tree-policy choose() pushes exactly one checkpoint (mirroring the
//     internal DBUCT frame it pushes),
//   * the transition into the rollout phase captures one transient snapshot,
//   * terminate() drives the matching restoration via end_episode(steps).
//
// The reward is passed straight through to DBUCT (no IGetValueDelta indirection):
// DBUCT backpropagates it up the camped path itself.
template<typename ICheckpointStack, typename IMakeResolutionLineage>
struct dbuct_sim {
    dbuct_sim(ICheckpointStack&, IMakeResolutionLineage&, std::mt19937&,
              double exploration_constant, std::size_t grant_increment_interval);

    mcts_choice choose(const std::vector<mcts_choice>&);
    std::size_t terminate(double reward);
    bool in_rollout() const { return dbuct_->in_rollout(); }

    // Root-frontier snapshot management, mirrored onto the checkpoint stack.
    void mark_root() { checkpoints_.mark_root(); }
    void restore_root() { checkpoints_.restore_root(); }

private:
    using decision_set_t = mcts_node_id::first_type;

    // Pure function — no mutable state; all context is in the node handle.
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
                                   mcts_node_id,        // INodeHandle
                                   mcts_choice,         // IChoice
                                   double,              // IFloat
                                   visits_table_t,      // IGetVisits
                                   value_table_t,       // IGetValue
                                   visits_table_t,      // ISetVisits
                                   value_table_t,       // ISetValue
                                   dispatches_table_t,  // IGetDispatches
                                   dispatches_table_t,  // ISetDispatches
                                   batch_t,             // IComputeBatchSize
                                   walker,              // IWalker
                                   choices_t,           // IGetChoiceCount
                                   choices_t,           // IGetChoiceAt
                                   rollout_t>;          // IRolloutChoose

    ICheckpointStack&      checkpoints_;
    visits_table_t         visits_table_;
    value_table_t          value_table_;
    dispatches_table_t     dispatches_table_;
    batch_t                batch_;
    walker                 walker_;
    rollout_t              rollout_;
    std::optional<dbuct_t> dbuct_;
};

template<typename ICS, typename IMRL>
dbuct_sim<ICS, IMRL>::dbuct_sim(ICS& checkpoints, IMRL& mrl, std::mt19937& rng,
                                double ec, std::size_t grant_increment_interval)
    : checkpoints_(checkpoints)
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

template<typename ICS, typename IMRL>
mcts_choice dbuct_sim<ICS, IMRL>::choose(const std::vector<mcts_choice>& choices) {
    const bool was_rollout = dbuct_->in_rollout();
    mcts_choice chosen = dbuct_->choose(choices, choices);
    if (!dbuct_->in_rollout()) {
        // Tree-policy choice: DBUCT pushed an internal frame; capture the state
        // as it stands before this choice's downstream mutations.
        checkpoints_.push_tree_policy();
    } else if (!was_rollout) {
        // First step of the rollout phase: capture the transient snapshot once.
        checkpoints_.enter_rollout();
    }
    return chosen;
}

template<typename ICS, typename IMRL>
std::size_t dbuct_sim<ICS, IMRL>::terminate(double reward) {
    // Whether this episode explored any new ground. If it never entered the
    // rollout phase, every node on its path was already expanded: tree policy
    // walked a fully-known path straight to a terminal.
    const bool was_rollout = dbuct_->in_rollout();

    std::size_t steps = dbuct_->terminate(reward);

    // A pure tree-policy episode that DBUCT would keep camped (steps == 0) has
    // exhausted its camped subtree: re-running from that node only re-derives the
    // same terminal — and if that terminal is a conflict, the next episode would
    // re-enter run_sim from an already-collapsed frontier (an active goal with no
    // candidates) and fault. Force at least one backstep so we always leave an
    // exhausted terminal. Repeated terminate() dispatches sink the node's granted
    // budget until it pops; because every extra dispatch scores the same terminal
    // reward the node's mean value is unchanged, so this only (correctly) drives
    // exploration away from the exhausted leaf.
    if (steps == 0 && !was_rollout) {
        do {
            steps = dbuct_->terminate(reward);
        } while (steps == 0);
    }

    checkpoints_.end_episode(steps);
    return steps;
}

#endif
