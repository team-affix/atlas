#ifndef PUD_LINEAGE_POOL_HPP
#define PUD_LINEAGE_POOL_HPP

#include <unordered_set>
#include "value_objects/pud_lineage.hpp"
#include "value_objects/pud_lineage_hash.hpp"
#include "debug_assert.hpp"

struct pud_lineage_pool {
    const pud_lineage* make_axiom(size_t entry_idx);
    const pud_lineage* make_inference(const pud_lineage* caller,
                                      size_t call_site,
                                      const pud_lineage* callee);
private:
    const pud_lineage* intern(pud_lineage&& lineage);
    std::unordered_set<pud_lineage, pud_lineage_hash> lineages_;
};

#endif
