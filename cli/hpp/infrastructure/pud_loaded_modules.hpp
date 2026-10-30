#ifndef PUD_LOADED_MODULES_HPP
#define PUD_LOADED_MODULES_HPP

#include <deque>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/pud_module.hpp"
#include "infrastructure/pud_module_deserializer_stack.hpp"
#include "value_objects/pud_module_id.hpp"

struct pud_loaded_modules {
    pud_loaded_modules(const std::vector<std::string>& paths);
    expr_pool& exprs();
    std::deque<pud_module>& modules();
private:
    void visit(
        pud_module_id id,
        const std::deque<pud_module>& loaded,
        const std::unordered_map<pud_module_id, size_t>& index_by_id,
        std::unordered_set<pud_module_id>& visiting,
        std::unordered_set<pud_module_id>& emitted,
        std::vector<pud_module_id>& order) const;

    pud_module_deserializer_stack stack_;
    std::deque<pud_module> modules_;
};

#endif
