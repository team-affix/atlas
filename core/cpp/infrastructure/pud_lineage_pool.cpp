#include "infrastructure/pud_lineage_pool.hpp"

const pud_lineage* pud_lineage_pool::make_axiom(size_t entry_idx) {
    return intern(pud_lineage{pud_lineage::axiom{entry_idx}});
}

const pud_lineage* pud_lineage_pool::make_inference(const pud_lineage* caller,
                                                   size_t call_site,
                                                   const pud_lineage* callee) {
    DEBUG_ASSERT(caller);
    return intern(pud_lineage{pud_lineage::inference{caller, call_site, callee}});
}

const pud_lineage* pud_lineage_pool::intern(pud_lineage&& lineage) {
    return &*lineages_.insert(std::move(lineage)).first;
}
