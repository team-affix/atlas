#ifndef MCTS_SIM_HPP
#define MCTS_SIM_HPP

#include <map>
#include <optional>
#include <random>
#include <set>
#include <vector>
#include "value_objects/mcts_choice.hpp"
#include "value_objects/lineage.hpp"
#include "mcts.hpp"

template<typename ISetUpSim, typename ITearDownSim, typename IComputeMctsReward,
         typename IMakeResolutionLineage>
struct mcts_sim {
    mcts_sim(ISetUpSim&, ITearDownSim&, IComputeMctsReward&,
             IMakeResolutionLineage&, std::mt19937&, double exploration_constant);

    void set_up();
    void tear_down();
    mcts_choice choose(const std::vector<mcts_choice>&);

private:
    // ── node handle ──────────────────────────────────────────────────────────
    // Each node is a pair:
    //   first  — ordered set of resolution decisions committed so far
    //   second — goal currently being targeted (nullptr between decisions)
    //
    // Transitions:
    //   goal choice (const goal_lineage* gl) → {same set,  gl}
    //   rule choice (rule_id r)              → {set ∪ {rl}, nullptr}
    //     where rl = make_resolution_lineage(node.second, r)
    //
    // Every (decision-set, goal) pair is a distinct node, so MCTS can learn
    // goal-selection policy.  Order-independence is preserved: two episodes
    // reaching the same decision set in different orders both share the same
    // {set, nullptr} node between decisions.
    using decision_set_t = std::set<const resolution_lineage*>;
    using node_id_t      = std::pair<decision_set_t, const goal_lineage*>;

    // ── walker ───────────────────────────────────────────────────────────────
    // Pure function — no mutable state; all context is in the node handle.
    struct walker {
        IMakeResolutionLineage& make_resolution_lineage_;
        walker(IMakeResolutionLineage&);
        node_id_t walk(const node_id_t&, const mcts_choice&) const;
    };

    // ── type aliases ─────────────────────────────────────────────────────────
    // std::map is used because node_id_t contains std::set<> which has
    // operator< but no std::hash.
    using table_t   = monte_carlo::map_table<node_id_t, double, std::map>;
    using choices_t = std::vector<mcts_choice>;
    using rollout_t = monte_carlo::random_rollout<
                          mcts_choice, std::mt19937, choices_t, choices_t>;
    using mc_sim_t  = monte_carlo::sim<
                          node_id_t,    // INodeHandle
                          mcts_choice, // IChoice
                          double,      // IFloat
                          table_t,     // IGetVisits
                          table_t,     // IGetValue
                          table_t,     // ISetVisits
                          table_t,     // ISetValue
                          walker,      // IWalker
                          choices_t,   // IGetChoiceCount
                          choices_t,   // IGetChoiceAt
                          rollout_t>;  // IRolloutChoose

    ISetUpSim&            set_up_;
    ITearDownSim&         tear_down_;
    IComputeMctsReward&   compute_mcts_reward_;
    double                exploration_constant_;

    // Persistent across all episodes — accumulates statistics over the
    // solver's lifetime so earlier sims inform later UCB1 decisions.
    table_t               table_;

    walker                walker_;
    rollout_t             rollout_;

    // Recreated each episode: terminate() does not reset the sim internals.
    std::optional<mc_sim_t> mc_sim_;
};

// ─── walker member function definitions ──────────────────────────────────────

template<typename ISUS, typename ITDS, typename ICMR, typename IMRL>
mcts_sim<ISUS, ITDS, ICMR, IMRL>::walker::walker(IMRL& mrl)
    : make_resolution_lineage_(mrl) {}

template<typename ISUS, typename ITDS, typename ICMR, typename IMRL>
typename mcts_sim<ISUS, ITDS, ICMR, IMRL>::node_id_t
mcts_sim<ISUS, ITDS, ICMR, IMRL>::walker::walk(
        const node_id_t& node, const mcts_choice& choice) const {
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

// ─── mcts_sim member function definitions ────────────────────────────────────

template<typename ISUS, typename ITDS, typename ICMR, typename IMRL>
mcts_sim<ISUS, ITDS, ICMR, IMRL>::mcts_sim(
        ISUS& sus, ITDS& tds, ICMR& cmr,
        IMRL& mrl, std::mt19937& rng, double ec)
    : set_up_(sus)
    , tear_down_(tds)
    , compute_mcts_reward_(cmr)
    , exploration_constant_(ec)
    , table_()
    , walker_(mrl)
    , rollout_(rng)
    , mc_sim_{}
{}

template<typename ISUS, typename ITDS, typename ICMR, typename IMRL>
void mcts_sim<ISUS, ITDS, ICMR, IMRL>::set_up() {
    mc_sim_.emplace(table_, table_, table_, table_,
                    walker_,
                    rollout_,
                    node_id_t{decision_set_t{}, nullptr},
                    exploration_constant_);
    set_up_.set_up();
}

template<typename ISUS, typename ITDS, typename ICMR, typename IMRL>
void mcts_sim<ISUS, ITDS, ICMR, IMRL>::tear_down() {
    mc_sim_->terminate(compute_mcts_reward_.compute_mcts_reward());
    tear_down_.tear_down();
}

template<typename ISUS, typename ITDS, typename ICMR, typename IMRL>
mcts_choice mcts_sim<ISUS, ITDS, ICMR, IMRL>::choose(
        const std::vector<mcts_choice>& choices) {
    return mc_sim_->choose(choices, choices);
}

#endif
