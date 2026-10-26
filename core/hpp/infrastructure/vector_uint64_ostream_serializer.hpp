#ifndef VECTOR_UINT64_OSTREAM_SERIALIZER_HPP
#define VECTOR_UINT64_OSTREAM_SERIALIZER_HPP

#include <cstdint>
#include <ostream>
#include <vector>

template<typename IInsertUint64>
struct vector_uint64_ostream_serializer {
    vector_uint64_ostream_serializer(IInsertUint64& insert_uint64);
    void serialize_vector_uint64(std::ostream& out, const std::vector<uint64_t>& items) const;
private:
    IInsertUint64& insert_uint64_;
};

#define VECTOR_UINT64_OSTREAM_SERIALIZER_TEMPLATE template<typename IInsertUint64>
#define VECTOR_UINT64_OSTREAM_SERIALIZER vector_uint64_ostream_serializer<IInsertUint64>

VECTOR_UINT64_OSTREAM_SERIALIZER_TEMPLATE
VECTOR_UINT64_OSTREAM_SERIALIZER::vector_uint64_ostream_serializer(IInsertUint64& insert_uint64)
    : insert_uint64_(insert_uint64) {}

VECTOR_UINT64_OSTREAM_SERIALIZER_TEMPLATE
void VECTOR_UINT64_OSTREAM_SERIALIZER::serialize_vector_uint64(std::ostream& out, const std::vector<uint64_t>& items) const {
    insert_uint64_.insert_uint64(out, items.size());
    for (uint64_t item : items)
        insert_uint64_.insert_uint64(out, item);
}

#undef VECTOR_UINT64_OSTREAM_SERIALIZER
#undef VECTOR_UINT64_OSTREAM_SERIALIZER_TEMPLATE

#endif
