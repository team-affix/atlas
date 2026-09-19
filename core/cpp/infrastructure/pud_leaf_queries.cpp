#include "infrastructure/pud_leaf_queries.hpp"

pud_leaf_queries::pud_leaf_queries()
    : owned_()
    , ptrs_() {}

void pud_leaf_queries::replace_leaf_queries(const pud_rule_id* leaf,
                                            std::vector<pud_query> queries) {
    owned_t owned;
    ptrs_t ptrs;
    owned.reserve(queries.size());
    ptrs.reserve(queries.size());
    for (pud_query& query : queries) {
        owned.push_back(std::make_unique<pud_query>(std::move(query)));
        ptrs.push_back(owned.back().get());
    }
    owned_[leaf] = std::move(owned);
    ptrs_[leaf] = std::move(ptrs);
}

const std::vector<pud_query*>& pud_leaf_queries::get(const pud_rule_id* leaf) const {
    return ptrs_.at(leaf);
}

void pud_leaf_queries::clear_leaf_queries(const pud_rule_id* leaf) {
    owned_.erase(leaf);
    ptrs_.erase(leaf);
}
