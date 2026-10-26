#ifndef RULE_ISTREAM_EXTRACTOR_HPP
#define RULE_ISTREAM_EXTRACTOR_HPP

#include <istream>
#include <vector>
#include "value_objects/expr.hpp"
#include "value_objects/rule.hpp"

template<typename IExtractExpr, typename IExtractUint64, typename IExtractUint32>
struct rule_istream_extractor {
    rule_istream_extractor(
        IExtractExpr& extract_expr,
        IExtractUint64& extract_uint64,
        IExtractUint32& extract_uint32);
    rule extract_rule(std::istream& in) const;
private:
    IExtractExpr& extract_expr_;
    IExtractUint64& extract_uint64_;
    IExtractUint32& extract_uint32_;
};

#define RULE_ISTREAM_EXTRACTOR_TEMPLATE template<typename IExtractExpr, typename IExtractUint64, typename IExtractUint32>
#define RULE_ISTREAM_EXTRACTOR rule_istream_extractor<IExtractExpr, IExtractUint64, IExtractUint32>

RULE_ISTREAM_EXTRACTOR_TEMPLATE
RULE_ISTREAM_EXTRACTOR::rule_istream_extractor(
    IExtractExpr& extract_expr,
    IExtractUint64& extract_uint64,
    IExtractUint32& extract_uint32)
    : extract_expr_(extract_expr)
    , extract_uint64_(extract_uint64)
    , extract_uint32_(extract_uint32) {}

RULE_ISTREAM_EXTRACTOR_TEMPLATE
rule RULE_ISTREAM_EXTRACTOR::extract_rule(std::istream& in) const {
    const expr* head = extract_expr_.extract_expr(in);
    const uint64_t body_len = extract_uint64_.extract_uint64(in);
    std::vector<const expr*> body;
    body.reserve(body_len);
    for (uint64_t goal = 0; goal < body_len; ++goal)
        body.push_back(extract_expr_.extract_expr(in));
    return rule(head, std::move(body), extract_uint32_.extract_uint32(in));
}

#undef RULE_ISTREAM_EXTRACTOR
#undef RULE_ISTREAM_EXTRACTOR_TEMPLATE

#endif
