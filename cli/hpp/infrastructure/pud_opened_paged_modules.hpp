#ifndef PUD_OPENED_PAGED_MODULES_HPP
#define PUD_OPENED_PAGED_MODULES_HPP

#include <cstddef>
#include <deque>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/pud_paged_module_assembly.hpp"
#include "value_objects/pud_module_id.hpp"

// Opens paged module directories one at a time and lists them in dependency order. Nothing is read beyond each
// header; the files are the modules, so saving only flushes them. The expression pool exists from construction, so
// a runtime can be built before any directory is opened.
struct pud_opened_paged_modules {
    pud_opened_paged_modules(size_t cache_bytes);
    pud_paged_module_assembly& open(const std::string& directory);
    std::vector<pud_paged_module_assembly*> in_dependency_order();
    expr_pool& exprs();
    void save_all();
private:
    void visit(
        pud_module_id id,
        const std::unordered_map<pud_module_id, pud_paged_module_assembly*>& by_id,
        std::unordered_set<pud_module_id>& visiting,
        std::unordered_set<pud_module_id>& emitted,
        std::vector<pud_paged_module_assembly*>& order) const;

    size_t cache_bytes_;
    expr_pool exprs_;
    std::deque<pud_paged_module_assembly> modules_;
};

#endif
