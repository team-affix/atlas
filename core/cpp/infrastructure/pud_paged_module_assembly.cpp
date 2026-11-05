#include "infrastructure/pud_paged_module_assembly.hpp"
#include <fstream>
#include <stdexcept>

pud_paged_module_assembly::pud_paged_module_assembly(
    const std::string& directory, expr_pool& exprs, size_t cache_bytes)
    : extract_uint8_()
    , extract_uint32_()
    , extract_uint64_()
    , extract_fixed_bytes_()
    , extract_expr_(exprs, exprs, extract_uint8_, extract_uint32_)
    , extract_rule_(extract_expr_, extract_uint64_, extract_uint32_)
    , extract_inference_(extract_expr_, extract_uint64_, extract_uint32_)
    , extract_module_id_(extract_fixed_bytes_, extract_uint64_)
    , extract_module_(
          extract_module_id_, extract_fixed_bytes_, extract_uint64_, extract_rule_, extract_inference_, extract_uint8_)
    , insert_uint8_()
    , insert_uint32_()
    , insert_uint64_()
    , insert_expr_(insert_uint8_, insert_uint32_)
    , insert_inference_(insert_expr_, insert_uint64_, insert_uint32_)
    , header_(read_header(directory))
    , records_(directory + "/records")
    , payload_(directory + "/payload")
    , names_(directory + "/names")
    , name_log_(names_, names_)
    , node_cache_(records_, payload_, extract_inference_, cache_bytes)
    , module_(
          records_, payload_, name_log_, node_cache_, insert_inference_, header_.get_module_id(),
          header_.get_dependencies(), copy_axioms(), name_log_.read_all()) {}

pud_paged_module_assembly::module_t& pud_paged_module_assembly::module() {
    return module_;
}

void pud_paged_module_assembly::sync() {
    records_.sync();
    payload_.sync();
    names_.sync();
}

size_t pud_paged_module_assembly::bytes() const {
    return records_.bytes() + payload_.size() + names_.size();
}

size_t pud_paged_module_assembly::cache_hits() const {
    return node_cache_.hits();
}

size_t pud_paged_module_assembly::cache_misses() const {
    return node_cache_.misses();
}

pud_module pud_paged_module_assembly::read_header(const std::string& directory) const {
    std::ifstream in(directory + "/header", std::ios::binary);
    if (!in)
        throw std::runtime_error("cannot open paged module header in " + directory);
    return extract_module_.extract_module(in);
}

std::vector<rule> pud_paged_module_assembly::copy_axioms() const {
    std::vector<rule> axioms;
    for (pud_node_id axiom = 0; axiom < header_.count_axioms(); ++axiom)
        axioms.push_back(header_.get_axiom(axiom));
    return axioms;
}
