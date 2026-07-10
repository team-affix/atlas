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

template<typename IFrameHub, typename IMakeResolutionLineage, typename IFrameControl>
struct dbuct_sim {
    dbuct_sim(IFrameHub&, IMakeResolutionLineage&, IFrameControl&,
              std::mt19937&, double exploration_constant,
              std::size_t grant_increment_interval);

    mcts_choice choose(const std::vector<mcts_choice>&);
    std::vector<const resolution_lineage*> terminate(double reward);
    bool at_root() const { return hub_.depth() == 1; }
    void unwind_to_root();
    void push_base_frame();

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

    IFrameHub&             hub_;
    IFrameControl&         frames_;
    visits_table_t         visits_table_;
    value_table_t          value_table_;
    dispatches_table_t     dispatches_table_;
    batch_t                batch_;
    walker                 walker_;
    rollout_t              rollout_;
    std::optional<dbuct_t> dbuct_;
};

template<typename IFrameHub, typename IMRL, typename IFC>
dbuct_sim<IFrameHub, IMRL, IFC>::dbuct_sim(IFrameHub& hub, IMRL& mrl, IFC& frames,
                                std::mt19937& rng,
                                double ec, std::size_t grant_increment_interval)
    : hub_(hub)
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
}

template<typename IFrameHub, typename IMRL, typename IFC>
void dbuct_sim<IFrameHub, IMRL, IFC>::push_base_frame() {
    hub_.push_frame();
}

template<typename IFrameHub, typename IMRL, typename IFC>
mcts_choice dbuct_sim<IFrameHub, IMRL, IFC>::choose(const std::vector<mcts_choice>& choices) {
    mcts_choice chosen = dbuct_->choose(choices, choices);
    hub_.push_frame();
    frames_.push_frame();
    return chosen;
}

template<typename IFrameHub, typename IMRL, typename IFC>
std::vector<const resolution_lineage*> dbuct_sim<IFrameHub, IMRL, IFC>::terminate(double reward) {
    // Force at least one backstep by capping dbuct's return one below the current
    // depth, unless already at root (depth 1) where nothing can be popped
    // (depth-1 == 0 is undefined for max_return_depth, so pass SIZE_MAX there).
    const std::size_t max_return_depth =
        hub_.depth() > 1 ? hub_.depth() - 1 : SIZE_MAX;
    const std::size_t stack_size = dbuct_->terminate(reward, max_return_depth);
    std::vector<const resolution_lineage*> eliminations;
    while (hub_.depth() > stack_size) {
        eliminations.clear();
        hub_.pop_frame();
        auto sm = frames_.pop_frame();
        while (!sm.done()) {
            sm.resume();
            if (sm.has_yield())
                eliminations.push_back(sm.consume_yield());
        }
    }
    return eliminations;
}

template<typename IFrameHub, typename IMRL, typename IFC>
void dbuct_sim<IFrameHub, IMRL, IFC>::unwind_to_root() {
    while (hub_.depth() > 1) {
        hub_.pop_frame();
        auto sm = frames_.pop_frame();
        while (!sm.done()) {
            sm.resume();
            if (sm.has_yield())
                sm.consume_yield();
        }
    }
    if (hub_.depth() > 0)
        hub_.pop_frame();
}

#endif
