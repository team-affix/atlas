#include "infrastructure/pud_rule_id_pool.hpp"

const pud_rule_id* pud_rule_id_pool::make_axiom(size_t entry_idx) {
    return intern(pud_rule_id{pud_rule_id::axiom{entry_idx}});
}

const pud_rule_id* pud_rule_id_pool::make_inference(const pud_rule_id* caller,
                                                   size_t call_site,
                                                   const pud_rule_id* callee) {
    DEBUG_ASSERT(caller);
    DEBUG_ASSERT(callee);
    return intern(pud_rule_id{pud_rule_id::inference{caller, call_site, callee}});
}

const pud_rule_id* pud_rule_id_pool::intern(pud_rule_id&& id) {
    return &*ids_.insert(std::move(id)).first;
}
