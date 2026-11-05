#ifndef PUD_MODULE_PAGER_HPP
#define PUD_MODULE_PAGER_HPP

#include <string>
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/pud_module.hpp"
#include "infrastructure/pud_paged_module_assembly.hpp"

// Converts between a module held in memory and a paged module's directory. Paging writes the header and an axiom
// record each, then stores the module's names, inferences and refuted flags through a paged module, in id order, so
// node ids carry over. Unpaging builds the in-memory module back the same way.
struct pud_module_pager {
    pud_module_pager();
    void page(const pud_module& module, const std::string& directory, expr_pool& exprs) const;
    pud_module unpage(pud_paged_module_assembly::module_t& paged) const;
};

#endif
