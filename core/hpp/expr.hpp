#ifndef EXPR_HPP
#define EXPR_HPP

// This file is dedicated to defining s-expressions used for this project, as well as
// functions that operate on them such as unifications.

#include <cstdint>
#include <string>
#include <variant>
#include <vector>
#include <set>
#include "trail.hpp"

struct expr {
    struct atom { std::string value; auto operator<=>(const atom&) const = default; };
    struct var  { uint32_t index;    auto operator<=>(const var&) const = default; };
    struct cons { const expr* lhs; const expr* rhs; auto operator<=>(const cons&) const = default; };
    struct pred { std::string name; std::vector<const expr*> args; auto operator<=>(const pred&) const = default; };
    std::variant<atom, cons, var, pred> content;
    auto operator<=>(const expr&) const = default;
};

struct expr_pool {
    expr_pool(trail&);
    const expr* atom(const std::string&);
    const expr* var(uint32_t);
    const expr* cons(const expr*, const expr*);
    const expr* pred(const std::string&, std::vector<const expr*>);
    const expr* as_expr(const expr::pred*);
    const expr* import(const expr*);
    size_t size() const;
#ifndef DEBUG
private:
#endif
    const expr* intern(expr&&);
    trail& trail_ref;
    std::set<expr> exprs;
};

#endif

