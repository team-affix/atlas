#ifndef MCTS_SIM_HPP
#define MCTS_SIM_HPP

#include <map>
#include <optional>
#include <random>
#include <vector>
#include "value_objects/mcts_choice.hpp"
#include "value_objects/mcts_node_id.hpp"
#include "mcts.hpp"

// Owns the Monte Carlo simulation object for one solver lifetime. set_up() and
// tear_down() only construct and terminate that object; solver-specific outer
// lifecycle components coordinate those calls with reward and state cleanup.
template<typename IGetValueDelta, typename IMakeResolutionLineage>
struct mcts_sim {
    mcts_sim(IGetValueDelta&, IMakeResolutionLineage&, std::mt19937&, double exploration_constant);

    void set_up();
    void tear_down();
    mcts_choice choose(const std::vector<mcts_choice>&, bool is_rule_choice);

private:
    using set_t = std::set<const resolution_lineage*>;
    using vec_t = std::vector<mcts_choice>;

    // ── walker ───────────────────────────────────────────────────────────────
    // Pure function — no mutable state; all context is in the node handle.
    struct walker {
        IMakeResolutionLineage& make_resolution_lineage_;
        walker(IMakeResolutionLineage&);
        mcts_node_id walk(const mcts_node_id&, const mcts_choice&) const;
    };

    // ── type aliases ─────────────────────────────────────────────────────────
    // std::map is used because mcts_node_id contains std::set<> which has
    // operator< but no std::hash.
    using visits_table_t = monte_carlo::visits_table<mcts_node_id, std::map>;
    using value_table_t  = monte_carlo::value_table<mcts_node_id, double, std::map>;
    using rollout_t = monte_carlo::random_rollout<
                          mcts_choice, std::mt19937, vec_t, vec_t>;
    using mc_sim_t  = monte_carlo::sim<
                          mcts_node_id,    // INodeHandle
                          mcts_choice,     // IChoice
                          double,          // IFloat
                          visits_table_t,  // IGetVisits
                          value_table_t,   // IGetValue
                          visits_table_t,  // ISetVisits
                          value_table_t,   // ISetValue
                          walker,          // IWalker
                          vec_t,           // IGetChoiceCount
                          vec_t,           // IGetChoiceAt
                          rollout_t,       // IRolloutChoose
                          IGetValueDelta>; // IGetValueDelta

    IGetValueDelta&       value_delta_;
    double                exploration_constant_;

    // Persistent across all episodes — accumulates statistics over the
    // solver's lifetime so earlier sims inform later UCB1 decisions.
    visits_table_t        visits_table_;
    value_table_t         value_table_;

    walker                walker_;
    rollout_t             rollout_;

    // Recreated each episode: terminate() does not reset the sim internals.
    std::optional<mc_sim_t> mc_sim_;
};

// ─── walker member function definitions ──────────────────────────────────────

template<typename IGVD, typename IMRL>
mcts_sim<IGVD, IMRL>::walker::walker(IMRL& mrl)
    : make_resolution_lineage_(mrl) {}

template<typename IGVD, typename IMRL>
mcts_node_id
mcts_sim<IGVD, IMRL>::walker::walk(
        const mcts_node_id& node, const mcts_choice& choice) const {
    if (const goal_lineage* const* gl_ptr =
            std::get_if<const goal_lineage*>(&choice)) {
        return {node.first, *gl_ptr};
    }
    const resolution_lineage* rl =
        make_resolution_lineage_.make_resolution_lineage(
            node.second, std::get<rule_id>(choice));
    set_t next_set = node.first;
    next_set.insert(rl);
    return {std::move(next_set), nullptr};
}

// ─── mcts_sim member function definitions ────────────────────────────────────

template<typename IGVD, typename IMRL>
mcts_sim<IGVD, IMRL>::mcts_sim(
        IGVD& gvd, IMRL& mrl, std::mt19937& rng, double ec)
    : value_delta_(gvd)
    , exploration_constant_(ec)
    , visits_table_()
    , value_table_()
    , walker_(mrl)
    , rollout_(rng)
    , mc_sim_{}
{}

template<typename IGVD, typename IMRL>
void mcts_sim<IGVD, IMRL>::set_up() {
    mc_sim_.emplace(visits_table_, value_table_, visits_table_, value_table_,
                    walker_,
                    rollout_,
                    value_delta_,
                    mcts_node_id{set_t{}, nullptr},
                    exploration_constant_);
}

template<typename IGVD, typename IMRL>
void mcts_sim<IGVD, IMRL>::tear_down() {
    mc_sim_->terminate();
}

template<typename IGVD, typename IMRL>
mcts_choice mcts_sim<IGVD, IMRL>::choose(
        const std::vector<mcts_choice>& choices, bool is_rule_choice) {
    (void)is_rule_choice;
    return mc_sim_->choose(choices, choices);
}

#endif
