#include "infrastructure/pud_query_forks.hpp"

pud_query_forks::pud_query_forks()
    : forked_heads_() {}

bool pud_query_forks::check_query_forked(pud_node_id node, pud_mhq_head_id head_id) const {
    const auto found = forked_heads_.find(node);
    return found != forked_heads_.end() && found->second.contains(head_id);
}

void pud_query_forks::mark_query_forked(pud_node_id node, pud_mhq_head_id head_id) {
    forked_heads_[node].insert(head_id);
}
