#ifndef VECTOR_UINT64_ISTREAM_DESERIALIZER_HPP
#define VECTOR_UINT64_ISTREAM_DESERIALIZER_HPP

#include <cstdint>
#include <istream>
#include <vector>

template<typename IExtractUint64>
struct vector_uint64_istream_deserializer {
    vector_uint64_istream_deserializer(IExtractUint64& extract_uint64);
    std::vector<uint64_t> deserialize_vector_uint64(std::istream& in) const;
private:
    IExtractUint64& extract_uint64_;
};

#define VECTOR_UINT64_ISTREAM_DESERIALIZER_TEMPLATE template<typename IExtractUint64>
#define VECTOR_UINT64_ISTREAM_DESERIALIZER vector_uint64_istream_deserializer<IExtractUint64>

VECTOR_UINT64_ISTREAM_DESERIALIZER_TEMPLATE
VECTOR_UINT64_ISTREAM_DESERIALIZER::vector_uint64_istream_deserializer(IExtractUint64& extract_uint64)
    : extract_uint64_(extract_uint64) {}

VECTOR_UINT64_ISTREAM_DESERIALIZER_TEMPLATE
std::vector<uint64_t> VECTOR_UINT64_ISTREAM_DESERIALIZER::deserialize_vector_uint64(std::istream& in) const {
    const uint64_t count = extract_uint64_.extract_uint64(in);
    std::vector<uint64_t> items;
    items.reserve(count);
    for (uint64_t item = 0; item < count; ++item)
        items.push_back(extract_uint64_.extract_uint64(in));
    return items;
}

#undef VECTOR_UINT64_ISTREAM_DESERIALIZER
#undef VECTOR_UINT64_ISTREAM_DESERIALIZER_TEMPLATE

#endif
