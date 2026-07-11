#ifndef EXPR_HASH_HPP
#define EXPR_HASH_HPP

#include <cstddef>
#include <functional>
#include <variant>
#include "expr.hpp"

struct expr_hash {
    size_t operator()(const expr& expression) const noexcept {
        if (const expr::var* variable = std::get_if<expr::var>(&expression.content)) {
            size_t seed = std::hash<uint32_t>{}(variable->index);
            return hash_combine(seed, 1);
        }

        const expr::functor& functor = std::get<expr::functor>(expression.content);
        size_t seed = std::hash<uint32_t>{}(functor.id);
        seed = hash_combine(seed, std::hash<size_t>{}(functor.args.size()));
        for (const expr* arg : functor.args)
            seed = hash_combine(seed, std::hash<const expr*>{}(arg));
        return hash_combine(seed, 2);
    }

private:
    static size_t hash_combine(size_t seed, size_t value) noexcept {
        return seed ^ (value + 0x9e3779b9 + (seed << 6) + (seed >> 2));
    }
};

#endif
