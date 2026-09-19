#ifndef PUD_LEAF_QUERIES_HPP
#define PUD_LEAF_QUERIES_HPP

#include <memory>
#include <unordered_map>
#include <vector>
#include "value_objects/pud_query.hpp"
#include "value_objects/pud_rule_id.hpp"
#include "debug_assert.hpp"

struct pud_leaf_queries {
    pud_leaf_queries();
    void replace_leaf_queries(const pud_rule_id* leaf, std::vector<pud_query> queries);
    const std::vector<pud_query*>& get(const pud_rule_id* leaf) const;
    void clear_leaf_queries(const pud_rule_id* leaf);
private:
    using owned_t = std::vector<std::unique_ptr<pud_query>>;
    using ptrs_t = std::vector<pud_query*>;
    using map_t = std::unordered_map<const pud_rule_id*, owned_t>;
    using ptr_map_t = std::unordered_map<const pud_rule_id*, ptrs_t>;

    map_t owned_;
    ptr_map_t ptrs_;
};

#endif
