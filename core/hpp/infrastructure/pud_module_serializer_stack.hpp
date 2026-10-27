#ifndef PUD_MODULE_SERIALIZER_STACK_HPP
#define PUD_MODULE_SERIALIZER_STACK_HPP

#include <ostream>
#include "infrastructure/expr_ostream_inserter.hpp"
#include "infrastructure/ostream_fixed_bytes_inserter.hpp"
#include "infrastructure/ostream_uint8_inserter.hpp"
#include "infrastructure/ostream_uint32_inserter.hpp"
#include "infrastructure/ostream_uint64_inserter.hpp"
#include "infrastructure/pud_inference_ostream_inserter.hpp"
#include "infrastructure/pud_module.hpp"
#include "infrastructure/pud_module_ostream_inserter.hpp"
#include "infrastructure/rule_ostream_inserter.hpp"
#include "infrastructure/vector_uint64_ostream_serializer.hpp"

struct pud_module_serializer_stack {
    pud_module_serializer_stack();
    void serialize_module(std::ostream& out, const pud_module& module) const;
private:
    ostream_uint8_inserter insert_uint8_;
    ostream_uint32_inserter insert_uint32_;
    ostream_uint64_inserter insert_uint64_;
    ostream_fixed_bytes_inserter insert_fixed_bytes_;
    expr_ostream_inserter<ostream_uint8_inserter, ostream_uint32_inserter> insert_expr_;
    rule_ostream_inserter<
        expr_ostream_inserter<ostream_uint8_inserter, ostream_uint32_inserter>,
        ostream_uint64_inserter,
        ostream_uint32_inserter> insert_rule_;
    pud_inference_ostream_inserter<
        expr_ostream_inserter<ostream_uint8_inserter, ostream_uint32_inserter>,
        ostream_uint64_inserter,
        ostream_uint32_inserter> insert_inference_;
    vector_uint64_ostream_serializer<ostream_uint64_inserter> serialize_vector_uint64_;
    pud_module_ostream_inserter<
        ostream_fixed_bytes_inserter,
        ostream_uint64_inserter,
        vector_uint64_ostream_serializer<ostream_uint64_inserter>,
        rule_ostream_inserter<
            expr_ostream_inserter<ostream_uint8_inserter, ostream_uint32_inserter>,
            ostream_uint64_inserter,
            ostream_uint32_inserter>,
        pud_inference_ostream_inserter<
            expr_ostream_inserter<ostream_uint8_inserter, ostream_uint32_inserter>,
            ostream_uint64_inserter,
            ostream_uint32_inserter>,
        ostream_uint8_inserter> insert_module_;
};

#endif
