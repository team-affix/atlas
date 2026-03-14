#ifndef EXPR_HPP
#define EXPR_HPP

// This file is dedicated to defining s-expressions used for this project, as well as
// functions that operate on them such as unifications.

#include <cstdint>
#include <string>
#include <variant>
#include <set>
#include "trail.hpp"

struct expr {
    struct atom { std::string value; auto operator<=>(const atom&) const = default; };
    struct var  { uint32_t index;    auto operator<=>(const var&) const = default; };
    struct cons { const expr* lhs; const expr* rhs; auto operator<=>(const cons&) const = default; };
    std::variant<atom, cons, var> content;
    auto operator<=>(const expr&) const = default;
};

struct expr_pool {
    expr_pool(trail&);
    const expr* atom(const std::string&);
    const expr* var(uint32_t);
    const expr* cons(const expr*, const expr*);
    size_t size() const;
#ifndef DEBUG
private:
#endif
    const expr* intern(expr&&);
    trail& trail_ref;
    std::set<expr> exprs;
};

#endif

