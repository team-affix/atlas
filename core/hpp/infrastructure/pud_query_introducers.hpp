#ifndef PUD_QUERY_INTRODUCERS_HPP
#define PUD_QUERY_INTRODUCERS_HPP

#include <map>
#include <optional>
#include <unordered_map>
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_mhq_head_id.hpp"
#include "value_objects/pud_query_introduction.hpp"

// The node each live query was introduced at, by opening or forking it. Descendants inherit a query through
// the tree without being recorded here. A query left without a candidate is also kept among the exhausted ones
// until it is withdrawn, so a node activated under one can tell it inherited it.
struct pud_query_introducers {
    pud_query_introducers();
    void introduce(pud_mhq_head_id head_id, pud_global_node_id node, pud_body_goal_id goal);
    void mark_exhausted(pud_mhq_head_id head_id);
    void withdraw(pud_mhq_head_id head_id);
    std::optional<pud_query_introduction> find_introduction(pud_mhq_head_id head_id) const;
    const std::map<pud_mhq_head_id, pud_query_introduction>& exhausted() const;
private:
    std::unordered_map<pud_mhq_head_id, pud_query_introduction> introductions_;
    std::map<pud_mhq_head_id, pud_query_introduction> exhausted_;
};

#endif
