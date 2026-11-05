#include "infrastructure/pud_module_pager.hpp"
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <vector>
#include "infrastructure/pud_mapped_record_file.hpp"
#include "infrastructure/pud_module_serializer_stack.hpp"
#include "value_objects/pud_node_record.hpp"
#include "value_objects/rule.hpp"

namespace {

// The pager reads every node once while writing, so its cache only needs to hold one step's worth.
constexpr size_t k_paging_cache_bytes = size_t{1} << 24;

}

pud_module_pager::pud_module_pager() {}

void pud_module_pager::page(const pud_module& module, const std::string& directory, expr_pool& exprs) const {
    if (std::filesystem::exists(directory))
        throw std::runtime_error("paged module directory already exists: " + directory);
    std::filesystem::create_directories(directory);
    std::vector<rule> axioms;
    for (pud_node_id axiom = 0; axiom < module.count_axioms(); ++axiom)
        axioms.push_back(module.get_axiom(axiom));
    const pud_module header(module.get_module_id(), module.get_dependencies(), std::move(axioms));
    {
        std::ofstream out(directory + "/header", std::ios::binary);
        pud_module_serializer_stack{}.serialize_module(out, header);
    }
    {
        pud_mapped_record_file records(directory + "/records");
        for (pud_node_id axiom = 0; axiom < module.count_axioms(); ++axiom)
            records.append(pud_node_record{
                .payload_offset = 0,
                .payload_size = 0,
                .parent = pud_node_record::k_no_parent,
                .first_child = 0,
                .child_count = 0,
                .refuted = 0});
    }
    pud_paged_module_assembly assembly(directory, exprs, k_paging_cache_bytes);
    pud_paged_module_assembly::module_t& paged = assembly.module();
    for (const std::string& name : module.functor_names())
        paged.append_functor_name(name);
    for (pud_node_id node = module.count_axioms(); node < module.count_nodes(); ++node) {
        paged.store_inference(module.get_inference(node));
        paged.trim();
    }
    for (pud_node_id node = 0; node < module.count_nodes(); ++node) {
        if (!module.is_refuted(node))
            continue;
        paged.mark_refuted(node);
    }
    assembly.sync();
}

pud_module pud_module_pager::unpage(pud_paged_module_assembly::module_t& paged) const {
    std::vector<rule> axioms;
    for (pud_node_id axiom = 0; axiom < paged.count_axioms(); ++axiom)
        axioms.push_back(paged.get_axiom(axiom));
    pud_module module(paged.get_module_id(), paged.get_dependencies(), std::move(axioms));
    for (const std::string& name : paged.functor_names())
        module.append_functor_name(name);
    for (pud_node_id node = paged.count_axioms(); node < paged.count_nodes(); ++node) {
        module.store_inference(paged.get_inference(node));
        paged.trim();
    }
    for (pud_node_id node = 0; node < paged.count_nodes(); ++node) {
        if (!paged.is_refuted(node))
            continue;
        module.mark_refuted(node);
    }
    return module;
}
