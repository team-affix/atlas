#ifndef MCTS_SIM_HPP
#define MCTS_SIM_HPP

#include <map>
#include <random>
#include <set>
#include <vector>
#include "value_objects/mcts_choice.hpp"
#include "value_objects/lineage.hpp"
#include "infrastructure/lineage_pool.hpp"
#include "mcts.hpp"

// ─── node handle ─────────────────────────────────────────────────────────────
// The MCTS table is keyed by the *set* of resolution decisions committed so far
// in the current sim.  Two episodes that committed the same decisions in
// different orders produce identical sets and therefore share the same node
// statistics.  This gives the tree order-independence across sims (matching
// Atlas's semantics: rule/body-atom order is not part of the solution contract).
using mcts_node_id = std::set<const resolution_lineage*>;

// ─── walker ───────────────────────────────────────────────────────────────────
// Computes the child node handle given a parent and a choice.
//
// Goal choices (const goal_lineage*) select *which* goal to resolve next.
// They are recorded in last_goal_ for use by the immediately following rule
// choice, but do not advance the decision set — the node is unchanged.
//
// Rule choices (rule_id) commit a (goal, rule) decision.  The walker builds
// the canonical resolution_lineage* via the lineage pool (cons-hashed, so the
// same (goal_lineage*, rule_id) always returns the same pointer) and inserts it
// into the set, producing a new child node.
struct decision_set_walker {
    lineage_pool& lp_;
    mutable const goal_lineage* last_goal_ = nullptr;

    explicit decision_set_walker(lineage_pool& lp) : lp_(lp) {}

    mcts_node_id walk(const mcts_node_id& node, mcts_choice choice) const {
        if (const goal_lineage* const* gl_ptr =
                std::get_if<const goal_lineage*>(&choice)) {
            last_goal_ = *gl_ptr;
            return node;
        }
        const resolution_lineage* rl =
            lp_.make_resolution_lineage(last_goal_, std::get<rule_id>(choice));
        mcts_node_id next = node;
        next.insert(rl);
        return next;
    }
};

// ─── arrival reward ───────────────────────────────────────────────────────────
// All per-edge immediate rewards are zero.  The episode reward is delivered
// once, wholesale, via step_reward() at the end of tear_down.
struct zero_arrival_reward {
    double get_arrival_reward(const mcts_node_id&, mcts_choice) const {
        return 0.0;
    }
};

// ─── choice adapter ───────────────────────────────────────────────────────────
// Adapts a std::vector<mcts_choice> to satisfy both IGetChoiceCount (.size())
// and IGetChoiceAt (.at(i)) as expected by monte_carlo::sim::choose().
struct choice_vector_ref {
    const std::vector<mcts_choice>* v = nullptr;
    size_t size() const { return v->size(); }
    mcts_choice at(size_t i) const { return (*v)[i]; }
};

// ─── mcts_sim ─────────────────────────────────────────────────────────────────
// Wraps the monte_carlo::sim episode loop and the two persistent stats tables.
// One instance lives on the manifest and spans all CHC sim episodes; the tables
// accumulate statistics across every solver::solve() cycle.
//
// ISetUpSim          — set_up()   pushes a trail frame
// ITearDownSim       — tear_down() clears per-sim state and pops the frame
// IComputeMctsReward — compute_mcts_reward() → double, evaluated at tear_down

template<typename ISetUpSim, typename ITearDownSim, typename IComputeMctsReward>
struct mcts_sim {
    mcts_sim(ISetUpSim&, ITearDownSim&, IComputeMctsReward&,
             lineage_pool&, std::mt19937&, double exploration_constant);

    void set_up();
    void tear_down();
    mcts_choice choose(const std::vector<mcts_choice>&);

private:
    // std::map is used (not std::unordered_map) because mcts_node_id =
    // std::set<const resolution_lineage*> has operator< but no std::hash.
    using table_t   = monte_carlo::map_table<mcts_node_id, double, std::map>;
    using edge_t    = monte_carlo::edge_map_table<mcts_node_id, std::map>;
    using rollout_t = monte_carlo::random_rollout<
                          mcts_choice, std::mt19937,
                          choice_vector_ref, choice_vector_ref>;
    using mc_sim_t  = monte_carlo::sim<
                          mcts_node_id,        // INodeHandle
                          mcts_choice,         // IChoice
                          double,              // IFloat
                          table_t,             // IGetVisits
                          table_t,             // IGetValue
                          table_t,             // ISetVisits
                          table_t,             // ISetValue
                          decision_set_walker, // IWalker
                          zero_arrival_reward, // IGetArrivalReward
                          edge_t,              // IGetEdgeVisits
                          edge_t,              // ISetEdgeVisits
                          choice_vector_ref,   // IGetChoiceCount
                          choice_vector_ref,   // IGetChoiceAt
                          rollout_t>;          // IRolloutChoose

    ISetUpSim&            set_up_;
    ITearDownSim&         tear_down_;
    IComputeMctsReward&   compute_mcts_reward_;
    double                exploration_constant_;

    // Persistent across all episodes — accumulate statistics over the solver's
    // lifetime so earlier episodes inform later UCB1 decisions.
    table_t               table_;
    edge_t                edge_table_;

    decision_set_walker   walker_;
    zero_arrival_reward   arrival_reward_;
    rollout_t             rollout_;
    choice_vector_ref     choice_ref_;

    // mc_sim_ is persistent; terminate() resets it to the root (empty set)
    // so it is ready for the next episode without re-construction.
    mc_sim_t              mc_sim_;
};

// ─── member function definitions ─────────────────────────────────────────────

template<typename ISUS, typename ITDS, typename ICMR>
mcts_sim<ISUS, ITDS, ICMR>::mcts_sim(
        ISUS& sus, ITDS& tds, ICMR& cmr,
        lineage_pool& lp, std::mt19937& rng, double ec)
    : set_up_(sus)
    , tear_down_(tds)
    , compute_mcts_reward_(cmr)
    , exploration_constant_(ec)
    , table_()
    , edge_table_()
    , walker_(lp)
    , arrival_reward_{}
    , rollout_(rng)
    , choice_ref_{}
    , mc_sim_(table_, table_, table_, table_,
              walker_, arrival_reward_,
              edge_table_, edge_table_,
              rollout_,
              mcts_node_id{},   // root = empty decision set
              ec)
{}

template<typename ISUS, typename ITDS, typename ICMR>
void mcts_sim<ISUS, ITDS, ICMR>::set_up() {
    // mc_sim_ was reset to root by the previous terminate(); nothing to rebuild.
    set_up_.set_up();
}

template<typename ISUS, typename ITDS, typename ICMR>
void mcts_sim<ISUS, ITDS, ICMR>::tear_down() {
    const double reward = compute_mcts_reward_.compute_mcts_reward();
    mc_sim_.step_reward(reward);
    mc_sim_.step_terminal();
    mc_sim_.terminate();
    tear_down_.tear_down();
}

template<typename ISUS, typename ITDS, typename ICMR>
mcts_choice mcts_sim<ISUS, ITDS, ICMR>::choose(
        const std::vector<mcts_choice>& choices) {
    choice_ref_.v = &choices;
    return mc_sim_.choose(choice_ref_, choice_ref_);
}

#endif
