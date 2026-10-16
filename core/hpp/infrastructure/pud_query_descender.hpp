#ifndef PUD_QUERY_DESCENDER_HPP
#define PUD_QUERY_DESCENDER_HPP

#include <optional>
#include <utility>
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_query_descent.hpp"

template<typename ReplayDescent>
struct pud_query_descender {
    std::optional<pud_query_descent<ReplayDescent>> descend(
        const pud_query_descent<ReplayDescent>& parent,
        pud_node_id child) const;
};

template<typename ReplayDescent>
std::optional<pud_query_descent<ReplayDescent>> pud_query_descender<ReplayDescent>::descend(
    const pud_query_descent<ReplayDescent>& parent,
    pud_node_id child) const {
    std::optional<ReplayDescent> child_replay = parent.replay.descend(child);
    if (!child_replay.has_value())
        return std::nullopt;
    return pud_query_descent<ReplayDescent>{child, std::move(child_replay.value())};
}

#endif
