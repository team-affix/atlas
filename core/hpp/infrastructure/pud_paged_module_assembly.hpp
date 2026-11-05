#ifndef PUD_PAGED_MODULE_ASSEMBLY_HPP
#define PUD_PAGED_MODULE_ASSEMBLY_HPP

#include <cstddef>
#include <string>
#include <vector>
#include "infrastructure/expr_istream_extractor.hpp"
#include "infrastructure/expr_ostream_inserter.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/istream_fixed_bytes_extractor.hpp"
#include "infrastructure/istream_uint32_extractor.hpp"
#include "infrastructure/istream_uint64_extractor.hpp"
#include "infrastructure/istream_uint8_extractor.hpp"
#include "infrastructure/ostream_uint32_inserter.hpp"
#include "infrastructure/ostream_uint64_inserter.hpp"
#include "infrastructure/ostream_uint8_inserter.hpp"
#include "infrastructure/pud_append_file.hpp"
#include "infrastructure/pud_decoded_node_cache.hpp"
#include "infrastructure/pud_inference_istream_extractor.hpp"
#include "infrastructure/pud_inference_ostream_inserter.hpp"
#include "infrastructure/pud_mapped_record_file.hpp"
#include "infrastructure/pud_module.hpp"
#include "infrastructure/pud_module_id_istream_extractor.hpp"
#include "infrastructure/pud_module_istream_extractor.hpp"
#include "infrastructure/pud_name_log.hpp"
#include "infrastructure/pud_paged_module.hpp"
#include "infrastructure/rule_istream_extractor.hpp"
#include "value_objects/rule.hpp"

// One paged module opened from its directory: the header (a v1 encoding of the module with its axioms only), the
// node records, the inference payload and the functor name log, plus the decoded-node cache over them. Terms decoded
// from the files are interned in the given pool, the one the runtime shares.
struct pud_paged_module_assembly {
    using expr_extractor_t =
        expr_istream_extractor<expr_pool, expr_pool, istream_uint8_extractor, istream_uint32_extractor>;
    using rule_extractor_t = rule_istream_extractor<expr_extractor_t, istream_uint64_extractor, istream_uint32_extractor>;
    using inference_extractor_t =
        pud_inference_istream_extractor<expr_extractor_t, istream_uint64_extractor, istream_uint32_extractor>;
    using module_id_extractor_t = pud_module_id_istream_extractor<istream_fixed_bytes_extractor, istream_uint64_extractor>;
    using module_extractor_t = pud_module_istream_extractor<
        module_id_extractor_t, istream_fixed_bytes_extractor, istream_uint64_extractor, rule_extractor_t,
        inference_extractor_t, istream_uint8_extractor>;
    using expr_inserter_t = expr_ostream_inserter<ostream_uint8_inserter, ostream_uint32_inserter>;
    using inference_inserter_t =
        pud_inference_ostream_inserter<expr_inserter_t, ostream_uint64_inserter, ostream_uint32_inserter>;
    using name_log_t = pud_name_log<pud_append_file, pud_append_file>;
    using node_cache_t = pud_decoded_node_cache<pud_mapped_record_file, pud_append_file, inference_extractor_t>;
    using module_t =
        pud_paged_module<pud_mapped_record_file, pud_append_file, name_log_t, node_cache_t, inference_inserter_t>;

    pud_paged_module_assembly(const std::string& directory, expr_pool& exprs, size_t cache_bytes);
    module_t& module();
    void sync();
    size_t bytes() const;
    size_t cache_hits() const;
    size_t cache_misses() const;
private:
    pud_module read_header(const std::string& directory) const;
    std::vector<rule> copy_axioms() const;

    istream_uint8_extractor extract_uint8_;
    istream_uint32_extractor extract_uint32_;
    istream_uint64_extractor extract_uint64_;
    istream_fixed_bytes_extractor extract_fixed_bytes_;
    expr_extractor_t extract_expr_;
    rule_extractor_t extract_rule_;
    inference_extractor_t extract_inference_;
    module_id_extractor_t extract_module_id_;
    module_extractor_t extract_module_;
    ostream_uint8_inserter insert_uint8_;
    ostream_uint32_inserter insert_uint32_;
    ostream_uint64_inserter insert_uint64_;
    expr_inserter_t insert_expr_;
    inference_inserter_t insert_inference_;
    pud_module header_;
    pud_mapped_record_file records_;
    pud_append_file payload_;
    pud_append_file names_;
    name_log_t name_log_;
    node_cache_t node_cache_;
    module_t module_;
};

#endif
