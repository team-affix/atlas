#include "infrastructure/pud_loaded_modules.hpp"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <utility>

#include "infrastructure/hex_uint64.hpp"

pud_loaded_modules::pud_loaded_modules()
    : stack_()
    , serializer_()
    , modules_()
    , paths_() {}

pud_module_id pud_loaded_modules::read_module_id(const std::string& path) const {
    std::ifstream in(path, std::ios::binary);
    if (!in)
        throw std::runtime_error("cannot open module file: " + path);
    return stack_.deserialize_module_id(in);
}

const pud_module& pud_loaded_modules::read(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in)
        throw std::runtime_error("cannot open module file: " + path);
    pud_module module = stack_.deserialize_module(in);
    for (const pud_module& loaded : modules_) {
        if (loaded.get_module_id() == module.get_module_id())
            throw std::runtime_error("duplicate module id");
    }
    modules_.push_back(std::move(module));
    paths_.push_back(path);
    return modules_.back();
}

void pud_loaded_modules::sort_by_dependencies() {
    if (modules_.empty())
        throw std::runtime_error("no module files");
    std::deque<pud_module> loaded = std::move(modules_);
    std::vector<std::string> loaded_paths = std::move(paths_);
    modules_.clear();
    paths_.clear();
    std::unordered_map<pud_module_id, size_t> index_by_id;
    for (size_t index = 0; index < loaded.size(); ++index)
        index_by_id.emplace(loaded[index].get_module_id(), index);
    std::unordered_set<pud_module_id> visiting;
    std::unordered_set<pud_module_id> emitted;
    std::vector<pud_module_id> order;
    for (const pud_module& module : loaded)
        visit(module.get_module_id(), loaded, index_by_id, visiting, emitted, order);
    for (const pud_module_id id : order) {
        modules_.push_back(std::move(loaded[index_by_id.at(id)]));
        paths_.push_back(loaded_paths[index_by_id.at(id)]);
    }
}

expr_pool& pud_loaded_modules::exprs() {
    return stack_.exprs();
}

std::deque<pud_module>& pud_loaded_modules::modules() {
    return modules_;
}

// Each module is written to a temporary file next to its own and then renamed over it, so a failure
// mid-write never leaves a truncated module behind.
void pud_loaded_modules::save_all() const {
    for (size_t index = 0; index < modules_.size(); ++index) {
        const std::string& path = paths_[index];
        const std::string temporary = path + ".tmp";
        std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
        if (!out)
            throw std::runtime_error("cannot write module file: " + temporary);
        serializer_.serialize_module(out, modules_[index]);
        out.close();
        if (!out)
            throw std::runtime_error("failed writing module file: " + temporary);
        std::filesystem::rename(temporary, path);
    }
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
