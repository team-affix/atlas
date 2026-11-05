#include "infrastructure/pud_opened_paged_modules.hpp"
#include <stdexcept>
#include "infrastructure/hex_uint64.hpp"

pud_opened_paged_modules::pud_opened_paged_modules(size_t cache_bytes)
    : cache_bytes_(cache_bytes)
    , exprs_()
    , modules_() {}

pud_paged_module_assembly& pud_opened_paged_modules::open(const std::string& directory) {
    return modules_.emplace_back(directory, exprs_, cache_bytes_);
}

std::vector<pud_paged_module_assembly*> pud_opened_paged_modules::in_dependency_order() {
    std::unordered_map<pud_module_id, pud_paged_module_assembly*> by_id;
    for (pud_paged_module_assembly& module : modules_)
        by_id.emplace(module.module().get_module_id(), &module);
    std::unordered_set<pud_module_id> visiting;
    std::unordered_set<pud_module_id> emitted;
    std::vector<pud_paged_module_assembly*> order;
    for (pud_paged_module_assembly& module : modules_)
        visit(module.module().get_module_id(), by_id, visiting, emitted, order);
    return order;
}

expr_pool& pud_opened_paged_modules::exprs() {
    return exprs_;
}

void pud_opened_paged_modules::save_all() {
    for (pud_paged_module_assembly& module : modules_)
        module.sync();
}

void pud_opened_paged_modules::visit(
    pud_module_id id,
    const std::unordered_map<pud_module_id, pud_paged_module_assembly*>& by_id,
    std::unordered_set<pud_module_id>& visiting,
    std::unordered_set<pud_module_id>& emitted,
    std::vector<pud_paged_module_assembly*>& order) const {
    if (emitted.contains(id))
        return;
    if (!visiting.insert(id).second)
        throw std::runtime_error("module dependency cycle");
    pud_paged_module_assembly* module = by_id.at(id);
    hex_uint64 hex_codec;
    for (const pud_module_id dependency : module->module().get_dependencies()) {
        if (!by_id.contains(dependency))
            throw std::runtime_error("missing dependency module " + hex_codec.format_hex(dependency));
        visit(dependency, by_id, visiting, emitted, order);
    }
    visiting.erase(id);
    emitted.insert(id);
    order.push_back(module);
}
