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
#include "infrastructure/pud_module_serializer_stack.hpp"
#include "value_objects/pud_module_id.hpp"

// Reads module files into memory one at a time, puts them in dependency order, and writes them back. The
// expression pool exists from construction, so a runtime can be built before any file is read.
struct pud_loaded_modules {
    pud_loaded_modules();
    pud_module_id read_module_id(const std::string& path) const;
    const pud_module& read(const std::string& path);
    void sort_by_dependencies();
    expr_pool& exprs();
    std::deque<pud_module>& modules();
    void save_all() const;
private:
    void visit(
        pud_module_id id,
        const std::deque<pud_module>& loaded,
        const std::unordered_map<pud_module_id, size_t>& index_by_id,
        std::unordered_set<pud_module_id>& visiting,
        std::unordered_set<pud_module_id>& emitted,
        std::vector<pud_module_id>& order) const;

    pud_module_deserializer_stack stack_;
    pud_module_serializer_stack serializer_;
    std::deque<pud_module> modules_;
    std::vector<std::string> paths_;
};

#endif
