#ifndef PUD_MODULE_ISTREAM_EXTRACTOR_HPP
#define PUD_MODULE_ISTREAM_EXTRACTOR_HPP

#include <cstring>
#include <istream>
#include <stdexcept>
#include <vector>
#include "infrastructure/pud_module.hpp"
#include "infrastructure/pud_module_ostream_inserter.hpp"
#include "value_objects/pud_module_id.hpp"
#include "value_objects/rule.hpp"

template<
    typename IExtractFixedBytes,
    typename IExtractUint64,
    typename IDeserializeVectorUint64,
    typename IExtractRule,
    typename IExtractInference,
    typename IExtractUint8>
struct pud_module_istream_extractor {
    pud_module_istream_extractor(
        IExtractFixedBytes& extract_fixed_bytes,
        IExtractUint64& extract_uint64,
        IDeserializeVectorUint64& deserialize_vector_uint64,
        IExtractRule& extract_rule,
        IExtractInference& extract_inference,
        IExtractUint8& extract_uint8);
    pud_module extract_module(std::istream& in) const;
private:
    IExtractFixedBytes& extract_fixed_bytes_;
    IExtractUint64& extract_uint64_;
    IDeserializeVectorUint64& deserialize_vector_uint64_;
    IExtractRule& extract_rule_;
    IExtractInference& extract_inference_;
    IExtractUint8& extract_uint8_;
};

#define PUD_MODULE_ISTREAM_EXTRACTOR_TEMPLATE \
    template< \
        typename IExtractFixedBytes, \
        typename IExtractUint64, \
        typename IDeserializeVectorUint64, \
        typename IExtractRule, \
        typename IExtractInference, \
        typename IExtractUint8>
#define PUD_MODULE_ISTREAM_EXTRACTOR pud_module_istream_extractor< \
    IExtractFixedBytes, \
    IExtractUint64, \
    IDeserializeVectorUint64, \
    IExtractRule, \
    IExtractInference, \
    IExtractUint8>

PUD_MODULE_ISTREAM_EXTRACTOR_TEMPLATE
PUD_MODULE_ISTREAM_EXTRACTOR::pud_module_istream_extractor(
    IExtractFixedBytes& extract_fixed_bytes,
    IExtractUint64& extract_uint64,
    IDeserializeVectorUint64& deserialize_vector_uint64,
    IExtractRule& extract_rule,
    IExtractInference& extract_inference,
    IExtractUint8& extract_uint8)
    : extract_fixed_bytes_(extract_fixed_bytes)
    , extract_uint64_(extract_uint64)
    , deserialize_vector_uint64_(deserialize_vector_uint64)
    , extract_rule_(extract_rule)
    , extract_inference_(extract_inference)
    , extract_uint8_(extract_uint8) {}

PUD_MODULE_ISTREAM_EXTRACTOR_TEMPLATE
pud_module PUD_MODULE_ISTREAM_EXTRACTOR::extract_module(std::istream& in) const {
    char magic[4] = {};
    extract_fixed_bytes_.extract_fixed_bytes(in, magic, 4);
    if (std::memcmp(magic, k_pud_module_blob_magic, 4) != 0)
        throw std::runtime_error("invalid pud module magic");
    const pud_module_id module_id = extract_uint64_.extract_uint64(in);
    const std::vector<uint64_t> dependency_ids = deserialize_vector_uint64_.deserialize_vector_uint64(in);
    std::vector<pud_module_id> dependencies(dependency_ids.begin(), dependency_ids.end());
    const uint64_t axiom_count = extract_uint64_.extract_uint64(in);
    std::vector<rule> axioms;
    axioms.reserve(axiom_count);
    for (uint64_t axiom = 0; axiom < axiom_count; ++axiom)
        axioms.push_back(extract_rule_.extract_rule(in));
    pud_module module{module_id, std::move(dependencies), std::move(axioms)};
    const uint64_t inference_count = extract_uint64_.extract_uint64(in);
    for (uint64_t stored = 0; stored < inference_count; ++stored) {
        pud_inference inference = extract_inference_.extract_inference(in);
        if (inference.parent >= module.count_nodes())
            throw std::runtime_error("pud inference parent out of range");
        module.store_inference(std::move(inference));
    }
    const uint64_t node_count = extract_uint64_.extract_uint64(in);
    if (node_count != module.count_nodes())
        throw std::runtime_error("pud refuted node count does not match stored nodes");
    for (uint64_t node = 0; node < node_count; ++node) {
        const uint8_t flag = extract_uint8_.extract_uint8(in);
        if (flag > 1)
            throw std::runtime_error("invalid pud refuted flag");
        if (flag == 1)
            module.mark_refuted(static_cast<pud_node_id>(node));
    }
    return module;
}

#undef PUD_MODULE_ISTREAM_EXTRACTOR
#undef PUD_MODULE_ISTREAM_EXTRACTOR_TEMPLATE

#endif
