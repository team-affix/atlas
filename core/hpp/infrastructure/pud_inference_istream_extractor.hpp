#ifndef PUD_INFERENCE_ISTREAM_EXTRACTOR_HPP
#define PUD_INFERENCE_ISTREAM_EXTRACTOR_HPP

#include <istream>
#include <vector>
#include "value_objects/pud_inference.hpp"
#include "value_objects/pud_substitution.hpp"

template<typename IExtractExpr, typename IExtractUint64, typename IExtractUint32>
struct pud_inference_istream_extractor {
    pud_inference_istream_extractor(
        IExtractExpr& extract_expr,
        IExtractUint64& extract_uint64,
        IExtractUint32& extract_uint32);
    pud_inference extract_inference(std::istream& in) const;
private:
    IExtractExpr& extract_expr_;
    IExtractUint64& extract_uint64_;
    IExtractUint32& extract_uint32_;
};

#define PUD_INFERENCE_ISTREAM_EXTRACTOR_TEMPLATE template<typename IExtractExpr, typename IExtractUint64, typename IExtractUint32>
#define PUD_INFERENCE_ISTREAM_EXTRACTOR pud_inference_istream_extractor<IExtractExpr, IExtractUint64, IExtractUint32>

PUD_INFERENCE_ISTREAM_EXTRACTOR_TEMPLATE
PUD_INFERENCE_ISTREAM_EXTRACTOR::pud_inference_istream_extractor(
    IExtractExpr& extract_expr,
    IExtractUint64& extract_uint64,
    IExtractUint32& extract_uint32)
    : extract_expr_(extract_expr)
    , extract_uint64_(extract_uint64)
    , extract_uint32_(extract_uint32) {}

PUD_INFERENCE_ISTREAM_EXTRACTOR_TEMPLATE
pud_inference PUD_INFERENCE_ISTREAM_EXTRACTOR::extract_inference(std::istream& in) const {
    pud_inference value{
        .parent = static_cast<pud_node_id>(extract_uint64_.extract_uint64(in)),
        .call_site = static_cast<pud_body_goal_id>(extract_uint64_.extract_uint64(in)),
        .substitutions = {},
        .body_goals = {},
        .var_count = 0,
    };
    const uint64_t substitution_count = extract_uint64_.extract_uint64(in);
    value.substitutions.reserve(substitution_count);
    for (uint64_t substitution = 0; substitution < substitution_count; ++substitution) {
        const uint32_t var_idx = extract_uint32_.extract_uint32(in);
        value.substitutions.push_back(
            pud_substitution{.var_idx = var_idx, .value = extract_expr_.extract_expr(in)});
    }
    const uint64_t body_len = extract_uint64_.extract_uint64(in);
    value.body_goals.reserve(body_len);
    for (uint64_t goal = 0; goal < body_len; ++goal)
        value.body_goals.push_back(extract_expr_.extract_expr(in));
    value.var_count = extract_uint32_.extract_uint32(in);
    return value;
}

#undef PUD_INFERENCE_ISTREAM_EXTRACTOR
#undef PUD_INFERENCE_ISTREAM_EXTRACTOR_TEMPLATE

#endif
