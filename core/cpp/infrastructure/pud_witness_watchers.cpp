#include "infrastructure/pud_witness_watchers.hpp"

pud_witness_watchers::pud_witness_watchers()
    : by_witness_()
    , by_query_()
    , empty_() {}

void pud_witness_watchers::watch(const pud_rule_id* witness, pud_query* query) {
    by_witness_[witness].insert(query);
    by_query_[query].insert(witness);
}

void pud_witness_watchers::unwatch(const pud_rule_id* witness, pud_query* query) {
    auto witness_it = by_witness_.find(witness);
    if (witness_it != by_witness_.end()) {
        witness_it->second.erase(query);
        if (witness_it->second.empty())
            by_witness_.erase(witness_it);
    }
    auto query_it = by_query_.find(query);
    if (query_it != by_query_.end()) {
        query_it->second.erase(witness);
        if (query_it->second.empty())
            by_query_.erase(query_it);
    }
}

void pud_witness_watchers::unwatch_query(pud_query* query) {
    auto query_it = by_query_.find(query);
    if (query_it == by_query_.end())
        return;
    const witness_set_t witnesses = query_it->second;
    by_query_.erase(query_it);
    for (const pud_rule_id* witness : witnesses) {
        auto witness_it = by_witness_.find(witness);
        if (witness_it == by_witness_.end())
            continue;
        witness_it->second.erase(query);
        if (witness_it->second.empty())
            by_witness_.erase(witness_it);
    }
}

const std::unordered_set<pud_query*>&
pud_witness_watchers::get(const pud_rule_id* witness) const {
    auto it = by_witness_.find(witness);
    if (it == by_witness_.end())
        return empty_;
    return it->second;
}
