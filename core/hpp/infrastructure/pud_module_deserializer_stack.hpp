#ifndef PUD_MODULE_DESERIALIZER_STACK_HPP
#define PUD_MODULE_DESERIALIZER_STACK_HPP

#include <istream>
#include "infrastructure/expr_istream_extractor.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/istream_fixed_bytes_extractor.hpp"
#include "infrastructure/istream_uint8_extractor.hpp"
#include "infrastructure/istream_uint32_extractor.hpp"
#include "infrastructure/istream_uint64_extractor.hpp"
#include "infrastructure/pud_inference_istream_extractor.hpp"
#include "infrastructure/pud_module.hpp"
#include "infrastructure/pud_module_istream_extractor.hpp"
#include "infrastructure/rule_istream_extractor.hpp"
#include "infrastructure/vector_uint64_istream_deserializer.hpp"

struct pud_module_deserializer_stack {
    pud_module_deserializer_stack();
    pud_module deserialize_module(std::istream& in) const;
private:
    using expr_extractor_t =
        expr_istream_extractor<expr_pool, expr_pool, istream_uint8_extractor, istream_uint32_extractor>;
    using rule_extractor_t =
        rule_istream_extractor<expr_extractor_t, istream_uint64_extractor, istream_uint32_extractor>;
    using inference_extractor_t =
        pud_inference_istream_extractor<expr_extractor_t, istream_uint64_extractor, istream_uint32_extractor>;

    expr_pool expr_pool_;
    istream_uint8_extractor extract_uint8_;
    istream_uint32_extractor extract_uint32_;
    istream_uint64_extractor extract_uint64_;
    istream_fixed_bytes_extractor extract_fixed_bytes_;
    expr_extractor_t extract_expr_;
    rule_extractor_t extract_rule_;
    inference_extractor_t extract_inference_;
    vector_uint64_istream_deserializer<istream_uint64_extractor> deserialize_vector_uint64_;
    pud_module_istream_extractor<
        istream_fixed_bytes_extractor,
        istream_uint64_extractor,
        vector_uint64_istream_deserializer<istream_uint64_extractor>,
        rule_extractor_t,
        inference_extractor_t,
        istream_uint8_extractor> extract_module_;
};

#endif
