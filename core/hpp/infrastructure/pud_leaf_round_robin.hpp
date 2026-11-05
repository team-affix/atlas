#ifndef PUD_LEAF_ROUND_ROBIN_HPP
#define PUD_LEAF_ROUND_ROBIN_HPP

#include <cstddef>
#include <optional>
#include <set>
#include "value_objects/pud_global_node_id.hpp"

// Deals the live leaves one at a time in id order, starting after the last one dealt and wrapping around, so every
// unfoldable leaf gets its turn. Leaves that may not be unfolded are passed over; when none may, nothing is dealt.
template<typename IGetLiveLeaves, typename IIsUnfoldable>
struct pud_leaf_round_robin {
    pud_leaf_round_robin(IGetLiveLeaves& get_live_leaves, IIsUnfoldable& is_unfoldable);
    std::optional<pud_global_node_id> next();
private:
    IGetLiveLeaves& get_live_leaves_;
    IIsUnfoldable& is_unfoldable_;
    std::optional<pud_global_node_id> last_dealt_;
};

template<typename IGetLiveLeaves, typename IIsUnfoldable>
pud_leaf_round_robin<IGetLiveLeaves, IIsUnfoldable>::pud_leaf_round_robin(
    IGetLiveLeaves& get_live_leaves, IIsUnfoldable& is_unfoldable)
    : get_live_leaves_(get_live_leaves)
    , is_unfoldable_(is_unfoldable)
    , last_dealt_(std::nullopt) {}

template<typename IGetLiveLeaves, typename IIsUnfoldable>
std::optional<pud_global_node_id> pud_leaf_round_robin<IGetLiveLeaves, IIsUnfoldable>::next() {
    const std::set<pud_global_node_id>& live = get_live_leaves_.live();
    auto turn = last_dealt_.has_value() ? live.upper_bound(last_dealt_.value()) : live.begin();
    for (size_t passed = 0; passed < live.size(); ++passed, ++turn) {
        if (turn == live.end())
            turn = live.begin();
        if (!is_unfoldable_.is_unfoldable(*turn))
            continue;
        last_dealt_ = *turn;
        return *turn;
    }
    return std::nullopt;
}

#endif
