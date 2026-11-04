#include "infrastructure/pud_query_introducers.hpp"
#include "debug_assert.hpp"

pud_query_introducers::pud_query_introducers()
    : introductions_()
    , exhausted_() {}

void pud_query_introducers::introduce(pud_mhq_head_id head_id, pud_global_node_id node, pud_body_goal_id goal) {
    [[maybe_unused]] const bool introduced =
        introductions_.emplace(head_id, pud_query_introduction{.node = node, .goal = goal}).second;
    DEBUG_ASSERT(introduced);
}

void pud_query_introducers::mark_exhausted(pud_mhq_head_id head_id) {
    exhausted_.emplace(head_id, introductions_.at(head_id));
}

void pud_query_introducers::withdraw(pud_mhq_head_id head_id) {
    exhausted_.erase(head_id);
    [[maybe_unused]] const size_t withdrawn = introductions_.erase(head_id);
    DEBUG_ASSERT(withdrawn == 1);
}

std::optional<pud_query_introduction> pud_query_introducers::find_introduction(pud_mhq_head_id head_id) const {
    const auto introduced = introductions_.find(head_id);
    if (introduced == introductions_.end())
        return std::nullopt;
    return introduced->second;
}

const std::map<pud_mhq_head_id, pud_query_introduction>& pud_query_introducers::exhausted() const {
    return exhausted_;
}
