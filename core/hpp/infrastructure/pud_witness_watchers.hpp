#ifndef PUD_WITNESS_WATCHERS_HPP
#define PUD_WITNESS_WATCHERS_HPP

#include <unordered_map>
#include <unordered_set>
#include "value_objects/pud_query.hpp"
#include "value_objects/pud_rule_id.hpp"

struct pud_witness_watchers {
    pud_witness_watchers();
    void watch(const pud_rule_id* witness, pud_query* query);
    void unwatch(const pud_rule_id* witness, pud_query* query);
    void unwatch_query(pud_query* query);
    const std::unordered_set<pud_query*>& get(const pud_rule_id* witness) const;
private:
    using query_set_t = std::unordered_set<pud_query*>;
    using witness_set_t = std::unordered_set<const pud_rule_id*>;
    using by_witness_t = std::unordered_map<const pud_rule_id*, query_set_t>;
    using by_query_t = std::unordered_map<pud_query*, witness_set_t>;

    by_witness_t by_witness_;
    by_query_t by_query_;
    query_set_t empty_;
};

#endif
