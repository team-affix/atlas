#ifndef PUD_LODESTAR_ROUND_ROBIN_HPP
#define PUD_LODESTAR_ROUND_ROBIN_HPP

#include <algorithm>
#include <optional>
#include <tuple>
#include <vector>
#include "value_objects/pud_leaf_goal_query.hpp"
#include "value_objects/pud_lodestar_choice.hpp"
#include "value_objects/pud_lodestar_slot_id.hpp"
#include "debug_assert.hpp"

// Deals one ring's choices one at a time: the first slot after the last one dealt, wrapping to the front.
// The ring is never built whole. Its slots sort by query leaf and goal first and the queries come in that
// order, so the walk starts at the cursor's query, lists choices query by query and stops at the first slot
// past the cursor. Only when nothing lies past it are the queries before the cursor listed, for the front.
template<typename IListChoices>
struct pud_lodestar_round_robin {
    pud_lodestar_round_robin(IListChoices& list_choices, bool focus_ring);
    std::optional<pud_lodestar_choice> next(const std::vector<pud_leaf_goal_query>& queries);
private:
    bool is_before_cursor(const pud_leaf_goal_query& query) const;
    pud_lodestar_choice deal(const pud_lodestar_choice& choice);

    IListChoices& list_choices_;
    bool focus_ring_;
    std::optional<pud_lodestar_slot_id> cursor_;
};

#define PUD_LODESTAR_ROUND_ROBIN_TEMPLATE template<typename IListChoices>
#define PUD_LODESTAR_ROUND_ROBIN          pud_lodestar_round_robin<IListChoices>

PUD_LODESTAR_ROUND_ROBIN_TEMPLATE
PUD_LODESTAR_ROUND_ROBIN::pud_lodestar_round_robin(IListChoices& list_choices, bool focus_ring)
    : list_choices_(list_choices)
    , focus_ring_(focus_ring)
    , cursor_(std::nullopt) {}

PUD_LODESTAR_ROUND_ROBIN_TEMPLATE
std::optional<pud_lodestar_choice> PUD_LODESTAR_ROUND_ROBIN::next(const std::vector<pud_leaf_goal_query>& queries) {
    DEBUG_ASSERT(std::is_sorted(queries.begin(), queries.end()));
    // Only the cursor's own query can have choices without one past the cursor; its first is the front
    // when no earlier query has any.
    std::optional<pud_lodestar_choice> cursor_query_front;
    for (const pud_leaf_goal_query& query : queries) {
        if (is_before_cursor(query))
            continue;
        const std::vector<pud_lodestar_choice> choices = list_choices_.list(query, focus_ring_);
        for (const pud_lodestar_choice& choice : choices) {
            const bool past_cursor = !cursor_.has_value() || cursor_.value() < choice.slot;
            if (!past_cursor)
                continue;
            return deal(choice);
        }
        if (!cursor_query_front.has_value() && !choices.empty())
            cursor_query_front = choices.front();
    }
    for (const pud_leaf_goal_query& query : queries) {
        if (!is_before_cursor(query))
            break;
        const std::vector<pud_lodestar_choice> choices = list_choices_.list(query, focus_ring_);
        if (!choices.empty())
            return deal(choices.front());
    }
    if (!cursor_query_front.has_value())
        return std::nullopt;
    return deal(cursor_query_front.value());
}

PUD_LODESTAR_ROUND_ROBIN_TEMPLATE
bool PUD_LODESTAR_ROUND_ROBIN::is_before_cursor(const pud_leaf_goal_query& query) const {
    if (!cursor_.has_value())
        return false;
    return std::tie(query.node, query.goal) < std::tie(cursor_.value().query_node, cursor_.value().query_goal);
}

PUD_LODESTAR_ROUND_ROBIN_TEMPLATE
pud_lodestar_choice PUD_LODESTAR_ROUND_ROBIN::deal(const pud_lodestar_choice& choice) {
    cursor_ = choice.slot;
    return choice;
}

#undef PUD_LODESTAR_ROUND_ROBIN
#undef PUD_LODESTAR_ROUND_ROBIN_TEMPLATE

#endif
