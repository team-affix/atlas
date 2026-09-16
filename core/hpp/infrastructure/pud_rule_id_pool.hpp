#ifndef PUD_RULE_ID_POOL_HPP
#define PUD_RULE_ID_POOL_HPP

#include <unordered_set>
#include "value_objects/pud_rule_id.hpp"
#include "value_objects/pud_rule_id_hash.hpp"
#include "debug_assert.hpp"

struct pud_rule_id_pool {
    const pud_rule_id* make_axiom(size_t entry_idx);
    const pud_rule_id* make_inference(const pud_rule_id* caller,
                                      size_t call_site,
                                      const pud_rule_id* callee);
    size_t size() const;
private:
    const pud_rule_id* intern(pud_rule_id&&);
    std::unordered_set<pud_rule_id, pud_rule_id_hash> ids_;
};

#endif
