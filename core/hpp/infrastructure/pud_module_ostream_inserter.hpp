#ifndef PUD_MODULE_OSTREAM_INSERTER_HPP
#define PUD_MODULE_OSTREAM_INSERTER_HPP

#include <ostream>
#include "infrastructure/pud_module.hpp"

inline constexpr char k_pud_module_blob_magic[4] = {'P', 'U', 'D', 'M'};

template<
    typename IInsertFixedBytes,
    typename IInsertUint64,
    typename ISerializeVectorUint64,
    typename IInsertRule,
    typename IInsertInference,
    typename IInsertUint8>
struct pud_module_ostream_inserter {
    pud_module_ostream_inserter(
        IInsertFixedBytes& insert_fixed_bytes,
        IInsertUint64& insert_uint64,
        ISerializeVectorUint64& serialize_vector_uint64,
        IInsertRule& insert_rule,
        IInsertInference& insert_inference,
        IInsertUint8& insert_uint8);
    void insert_module(std::ostream& out, const pud_module& module) const;
private:
    IInsertFixedBytes& insert_fixed_bytes_;
    IInsertUint64& insert_uint64_;
    ISerializeVectorUint64& serialize_vector_uint64_;
    IInsertRule& insert_rule_;
    IInsertInference& insert_inference_;
    IInsertUint8& insert_uint8_;
};

#define PUD_MODULE_OSTREAM_INSERTER_TEMPLATE \
    template< \
        typename IInsertFixedBytes, \
        typename IInsertUint64, \
        typename ISerializeVectorUint64, \
        typename IInsertRule, \
        typename IInsertInference, \
        typename IInsertUint8>
#define PUD_MODULE_OSTREAM_INSERTER pud_module_ostream_inserter< \
    IInsertFixedBytes, \
    IInsertUint64, \
    ISerializeVectorUint64, \
    IInsertRule, \
    IInsertInference, \
    IInsertUint8>

PUD_MODULE_OSTREAM_INSERTER_TEMPLATE
PUD_MODULE_OSTREAM_INSERTER::pud_module_ostream_inserter(
    IInsertFixedBytes& insert_fixed_bytes,
    IInsertUint64& insert_uint64,
    ISerializeVectorUint64& serialize_vector_uint64,
    IInsertRule& insert_rule,
    IInsertInference& insert_inference,
    IInsertUint8& insert_uint8)
    : insert_fixed_bytes_(insert_fixed_bytes)
    , insert_uint64_(insert_uint64)
    , serialize_vector_uint64_(serialize_vector_uint64)
    , insert_rule_(insert_rule)
    , insert_inference_(insert_inference)
    , insert_uint8_(insert_uint8) {}

PUD_MODULE_OSTREAM_INSERTER_TEMPLATE
void PUD_MODULE_OSTREAM_INSERTER::insert_module(std::ostream& out, const pud_module& module) const {
    insert_fixed_bytes_.insert_fixed_bytes(out, k_pud_module_blob_magic, 4);
    insert_uint64_.insert_uint64(out, module.get_module_id());
    const std::vector<pud_module_id>& dependencies = module.get_dependencies();
    std::vector<uint64_t> dependency_ids(dependencies.begin(), dependencies.end());
    serialize_vector_uint64_.serialize_vector_uint64(out, dependency_ids);
    const size_t axiom_count = module.count_axioms();
    insert_uint64_.insert_uint64(out, axiom_count);
    for (pud_node_id axiom = 0; axiom < axiom_count; ++axiom)
        insert_rule_.insert_rule(out, module.get_axiom(axiom));
    const size_t node_count = module.count_nodes();
    insert_uint64_.insert_uint64(out, node_count - axiom_count);
    for (pud_node_id inference = axiom_count; inference < node_count; ++inference)
        insert_inference_.insert_inference(out, module.get_inference(inference));
    insert_uint64_.insert_uint64(out, node_count);
    for (pud_node_id node = 0; node < node_count; ++node)
        insert_uint8_.insert_uint8(out, module.check_refuted(node) ? 1 : 0);
    const std::vector<std::string>& names = module.functor_names();
    insert_uint64_.insert_uint64(out, names.size());
    for (const std::string& name : names) {
        insert_uint64_.insert_uint64(out, name.size());
        insert_fixed_bytes_.insert_fixed_bytes(out, name.data(), name.size());
    }
}

#undef PUD_MODULE_OSTREAM_INSERTER
#undef PUD_MODULE_OSTREAM_INSERTER_TEMPLATE

#endif
