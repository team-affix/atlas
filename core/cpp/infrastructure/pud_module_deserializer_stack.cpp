#include "infrastructure/pud_module_deserializer_stack.hpp"

pud_module_deserializer_stack::pud_module_deserializer_stack()
    : extract_expr_(expr_pool_, expr_pool_, extract_uint8_, extract_uint32_)
    , extract_rule_(extract_expr_, extract_uint64_, extract_uint32_)
    , extract_inference_(extract_expr_, extract_uint64_, extract_uint32_)
    , deserialize_vector_uint64_(extract_uint64_)
    , extract_module_(
        extract_fixed_bytes_,
        extract_uint64_,
        deserialize_vector_uint64_,
        extract_rule_,
        extract_inference_,
        extract_uint8_) {}

pud_module pud_module_deserializer_stack::deserialize_module(std::istream& in) const {
    return extract_module_.extract_module(in);
}
