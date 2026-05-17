#ifndef EXPAND_CTX_HPP
#define EXPAND_CTX_HPP

#include <vector>
#include <unordered_map>
#include <cstdint>
#include "expr.hpp"
#include "goal.hpp"
#include "candidate.hpp"
#include "rule.hpp"

struct expand_ctx {
    virtual ~expand_ctx() = default;
    expand_ctx(
        const goal&, candidate&, const rule&);
    std::unordered_map<uint32_t, uint32_t>& tm;
    const std::vector<const expr*>& original_rule_body;
};

#endif
