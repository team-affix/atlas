#include "infrastructure/pud_loaded_modules.hpp"

#include <fstream>
#include <stdexcept>
#include <utility>

#include "infrastructure/hex_uint64.hpp"

pud_loaded_modules::pud_loaded_modules(const std::vector<std::string>& paths)
    : stack_()
    , modules_() {
    if (paths.empty())
        throw std::runtime_error("no module files");
    std::deque<pud_module> loaded;
    std::unordered_map<pud_module_id, size_t> index_by_id;
    for (const std::string& path : paths) {
        std::ifstream in(path, std::ios::binary);
        if (!in)
            throw std::runtime_error("cannot open module file: " + path);
        pud_module module = stack_.deserialize_module(in);
        const pud_module_id id = module.get_module_id();
        if (index_by_id.contains(id))
            throw std::runtime_error("duplicate module id");
        index_by_id.emplace(id, loaded.size());
        loaded.push_back(std::move(module));
    }
    std::unordered_set<pud_module_id> visiting;
    std::unordered_set<pud_module_id> emitted;
    std::vector<pud_module_id> order;
    for (const pud_module& module : loaded)
        visit(module.get_module_id(), loaded, index_by_id, visiting, emitted, order);
    for (const pud_module_id id : order)
        modules_.push_back(std::move(loaded[index_by_id.at(id)]));
}

expr_pool& pud_loaded_modules::exprs() {
    return stack_.exprs();
}

std::deque<pud_module>& pud_loaded_modules::modules() {
    return modules_;
}

void pud_loaded_modules::visit(
    pud_module_id id,
    const std::deque<pud_module>& loaded,
    const std::unordered_map<pud_module_id, size_t>& index_by_id,
    std::unordered_set<pud_module_id>& visiting,
    std::unordered_set<pud_module_id>& emitted,
    std::vector<pud_module_id>& order) const {
    if (emitted.contains(id))
        return;
    if (!visiting.insert(id).second)
        throw std::runtime_error("module dependency cycle");
    const pud_module& module = loaded[index_by_id.at(id)];
    hex_uint64 hex_codec;
    for (const pud_module_id dependency : module.get_dependencies()) {
        if (!index_by_id.contains(dependency))
            throw std::runtime_error("missing dependency module " + hex_codec.format_hex(dependency));
        visit(dependency, loaded, index_by_id, visiting, emitted, order);
    }
    visiting.erase(id);
    emitted.insert(id);
    order.push_back(id);
}
