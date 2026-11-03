#ifndef PUD_MODULE_ID_ISTREAM_EXTRACTOR_HPP
#define PUD_MODULE_ID_ISTREAM_EXTRACTOR_HPP

#include <cstring>
#include <istream>
#include <stdexcept>
#include "infrastructure/pud_module.hpp"
#include "value_objects/pud_module_id.hpp"

// Reads a module blob's header: the magic bytes, then the module id. On its own it tells which module a
// file holds without reading the rest of it.
template<typename IExtractFixedBytes, typename IExtractUint64>
struct pud_module_id_istream_extractor {
    pud_module_id_istream_extractor(IExtractFixedBytes& extract_fixed_bytes, IExtractUint64& extract_uint64);
    pud_module_id extract_module_id(std::istream& in) const;
private:
    IExtractFixedBytes& extract_fixed_bytes_;
    IExtractUint64& extract_uint64_;
};

template<typename IExtractFixedBytes, typename IExtractUint64>
pud_module_id_istream_extractor<IExtractFixedBytes, IExtractUint64>::pud_module_id_istream_extractor(
    IExtractFixedBytes& extract_fixed_bytes, IExtractUint64& extract_uint64)
    : extract_fixed_bytes_(extract_fixed_bytes)
    , extract_uint64_(extract_uint64) {}

template<typename IExtractFixedBytes, typename IExtractUint64>
pud_module_id pud_module_id_istream_extractor<IExtractFixedBytes, IExtractUint64>::extract_module_id(
    std::istream& in) const {
    char magic[4] = {};
    extract_fixed_bytes_.extract_fixed_bytes(in, magic, 4);
    if (std::memcmp(magic, k_pud_module_blob_magic, 4) != 0)
        throw std::runtime_error("invalid pud module magic");
    return extract_uint64_.extract_uint64(in);
}

#endif
