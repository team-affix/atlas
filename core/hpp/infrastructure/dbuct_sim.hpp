#ifndef DBUCT_SIM_HPP
#define DBUCT_SIM_HPP

#include <cstddef>
#include <map>
#include <optional>
#include <random>
#include <unordered_map>
#include <vector>
#include "value_objects/lineage.hpp"
#include "value_objects/mcts_choice.hpp"
#include "dbuct.hpp"
#include "visits_table.hpp"
#include "value_table.hpp"
#include "dispatches_table.hpp"
#include "linear_batch_increment.hpp"
#include "random_rollout.hpp"

template<typename NodeHandle, bool UseUnordered>
struct dbuct_visits_table_selector;

template<typename NodeHandle>
struct dbuct_visits_table_selector<NodeHandle, false> {
    using type = monte_carlo::visits_table<NodeHandle, std::map>;
};

template<typename NodeHandle>
struct dbuct_visits_table_selector<NodeHandle, true> {
    using type = monte_carlo::visits_table<NodeHandle, std::unordered_map>;
};

template<typename NodeHandle, bool UseUnordered>
struct dbuct_value_table_selector;

template<typename NodeHandle>
struct dbuct_value_table_selector<NodeHandle, false> {
    using type = monte_carlo::value_table<NodeHandle, double, std::map>;
};

template<typename NodeHandle>
struct dbuct_value_table_selector<NodeHandle, true> {
    using type = monte_carlo::value_table<NodeHandle, double, std::unordered_map>;
};

template<typename NodeHandle, bool UseUnordered>
struct dbuct_dispatches_table_selector;

template<typename NodeHandle>
struct dbuct_dispatches_table_selector<NodeHandle, false> {
    using type = monte_carlo::dispatches_table<NodeHandle, std::map>;
};

template<typename NodeHandle>
struct dbuct_dispatches_table_selector<NodeHandle, true> {
    using type = monte_carlo::dispatches_table<NodeHandle, std::unordered_map>;
};

template<typename IFrameHub, typename IFrameControl, typename IWalk>
struct dbuct_sim {
    dbuct_sim(IFrameHub&, IFrameControl&, IWalk&,
              std::mt19937&, double exploration_constant,
              std::size_t grant_increment_interval);

    mcts_choice choose(const std::vector<mcts_choice>&);
    std::vector<const resolution_lineage*> terminate(double reward);
    bool at_root() const;
    void unwind_to_root();
    void push_base_frame();

private:
    using node_handle_t = decltype(IWalk::make_root());
    using visits_table_t = typename dbuct_visits_table_selector<
        node_handle_t, IWalk::use_unordered_tables>::type;
    using value_table_t = typename dbuct_value_table_selector<
        node_handle_t, IWalk::use_unordered_tables>::type;
    using dispatches_table_t = typename dbuct_dispatches_table_selector<
        node_handle_t, IWalk::use_unordered_tables>::type;
    using choices_t          = std::vector<mcts_choice>;
    using rollout_t          = monte_carlo::random_rollout<
                                   mcts_choice, std::mt19937, choices_t, choices_t>;
    using batch_t            = monte_carlo::linear_batch_increment;
    using dbuct_t            = monte_carlo::dbuct<
                                   node_handle_t,
                                   mcts_choice,
                                   double,
                                   visits_table_t,
                                   value_table_t,
                                   visits_table_t,
                                   value_table_t,
                                   dispatches_table_t,
                                   dispatches_table_t,
                                   batch_t,
                                   IWalk,
                                   choices_t,
                                   choices_t,
                                   rollout_t>;

    IFrameHub&             hub_;
    IFrameControl&         frames_;
    IWalk&                 walk_;
    visits_table_t         visits_table_;
    value_table_t          value_table_;
    dispatches_table_t     dispatches_table_;
    batch_t                batch_;
    rollout_t              rollout_;
    std::optional<dbuct_t> dbuct_;
};

template<typename IFrameHub, typename IFrameControl, typename IWalk>
bool dbuct_sim<IFrameHub, IFrameControl, IWalk>::at_root() const {
    return hub_.depth() == 1;
}

template<typename IFrameHub, typename IFrameControl, typename IWalk>
dbuct_sim<IFrameHub, IFrameControl, IWalk>::dbuct_sim(
    IFrameHub& hub, IFrameControl& frames, IWalk& walk,
    std::mt19937& rng, double ec, std::size_t grant_increment_interval)
    : hub_(hub)
    , frames_(frames)
    , walk_(walk)
    , visits_table_()
    , value_table_()
    , dispatches_table_()
    , batch_(grant_increment_interval)
    , rollout_(rng)
    , dbuct_(std::nullopt) {
    dbuct_.emplace(visits_table_, value_table_, visits_table_, value_table_,
                   dispatches_table_, dispatches_table_, batch_,
                   walk_, rollout_,
                   IWalk::make_root(),
                   ec);
}

template<typename IFrameHub, typename IFrameControl, typename IWalk>
void dbuct_sim<IFrameHub, IFrameControl, IWalk>::push_base_frame() {
    hub_.push_frame();
}

template<typename IFrameHub, typename IFrameControl, typename IWalk>
mcts_choice dbuct_sim<IFrameHub, IFrameControl, IWalk>::choose(
    const std::vector<mcts_choice>& choices) {
    const bool was_in_rollout = dbuct_->in_rollout();
    mcts_choice chosen = dbuct_->choose(choices, choices);
    if (was_in_rollout)
        return chosen;
    hub_.push_frame();
    frames_.push_frame();
    return chosen;
}

template<typename IFrameHub, typename IFrameControl, typename IWalk>
std::vector<const resolution_lineage*> dbuct_sim<IFrameHub, IFrameControl, IWalk>::terminate(
    double reward) {
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

template<typename IFrameHub, typename IFrameControl, typename IWalk>
void dbuct_sim<IFrameHub, IFrameControl, IWalk>::unwind_to_root() {
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
