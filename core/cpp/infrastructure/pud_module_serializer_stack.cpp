#include "infrastructure/pud_module_serializer_stack.hpp"

pud_module_serializer_stack::pud_module_serializer_stack()
    : insert_expr_(insert_uint8_, insert_uint32_)
    , insert_rule_(insert_expr_, insert_uint64_, insert_uint32_)
    , insert_inference_(insert_expr_, insert_uint64_, insert_uint32_)
    , serialize_vector_uint64_(insert_uint64_)
    , insert_module_(
        insert_fixed_bytes_,
        insert_uint64_,
        serialize_vector_uint64_,
        insert_rule_,
        insert_inference_,
        insert_uint8_) {}

void pud_module_serializer_stack::serialize_module(std::ostream& out, const pud_module& module) const {
    insert_module_.insert_module(out, module);
}
