#ifndef PUD_LODESTAR_ROUND_ROBIN_HPP
#define PUD_LODESTAR_ROUND_ROBIN_HPP

#include <limits>
#include <optional>
#include <set>
#include "value_objects/pud_lodestar_choice.hpp"
#include "value_objects/pud_lodestar_slot_id.hpp"

// Deals one ring's choices one at a time: the first slot after the last one dealt, wrapping to the front.
// The ring is kept sorted elsewhere, so each deal is one lookup past the cursor.
template<typename IGetRing>
struct pud_lodestar_round_robin {
    pud_lodestar_round_robin(IGetRing& get_ring, bool focus_ring);
    std::optional<pud_lodestar_choice> next();
private:
    IGetRing& get_ring_;
    bool focus_ring_;
    std::optional<pud_lodestar_slot_id> cursor_;
};

#define PUD_LODESTAR_ROUND_ROBIN_TEMPLATE template<typename IGetRing>
#define PUD_LODESTAR_ROUND_ROBIN          pud_lodestar_round_robin<IGetRing>

PUD_LODESTAR_ROUND_ROBIN_TEMPLATE
PUD_LODESTAR_ROUND_ROBIN::pud_lodestar_round_robin(IGetRing& get_ring, bool focus_ring)
    : get_ring_(get_ring)
    , focus_ring_(focus_ring)
    , cursor_(std::nullopt) {}

PUD_LODESTAR_ROUND_ROBIN_TEMPLATE
std::optional<pud_lodestar_choice> PUD_LODESTAR_ROUND_ROBIN::next() {
    const std::multiset<pud_lodestar_choice>& ring = get_ring_.ring(focus_ring_);
    if (ring.empty())
        return std::nullopt;
    auto dealt = ring.begin();
    if (cursor_.has_value()) {
        // The last choice that shares the cursor's slot sorts at or before this one.
        const pud_lodestar_choice last_at_cursor{
            .slot = cursor_.value(),
            .leaf = pud_global_node_id{
                .module = std::numeric_limits<pud_module_id>::max(),
                .node = std::numeric_limits<pud_node_id>::max(),
            },
            .body_goal = std::numeric_limits<pud_body_goal_id>::max(),
        };
        const auto past_cursor = ring.upper_bound(last_at_cursor);
        if (past_cursor != ring.end())
            dealt = past_cursor;
    }
    cursor_ = dealt->slot;
    return *dealt;
}

#undef PUD_LODESTAR_ROUND_ROBIN
#undef PUD_LODESTAR_ROUND_ROBIN_TEMPLATE

#endif
