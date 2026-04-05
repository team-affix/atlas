#ifndef EXPR_HPP
#define EXPR_HPP

// This file is dedicated to defining s-expressions used for this project, as well as
// functions that operate on them such as unifications.

#include <cstdint>
#include <string>
#include <variant>
#include <set>
#include "trail.hpp"
#include "sequencer.hpp"
#include "registry.hpp"

using expr_id = size_t;

struct expr {
    struct atom { std::string value; auto operator<=>(const atom&) const = default; };
    struct var  { uint32_t index;    auto operator<=>(const var&) const = default; };
    struct cons { expr_id lhs; expr_id rhs; auto operator<=>(const cons&) const = default; };
    std::variant<atom, cons, var> content;
    auto operator<=>(const expr&) const = default;
};

struct expr_pool {
    expr_pool(trail&, sequencer&, registry<expr>&);
    expr_id atom(const std::string&);
    expr_id var(uint32_t);
    expr_id cons(expr_id, expr_id);
    expr_id import(expr_id);
    size_t size() const;
#ifndef DEBUG
private:
#endif
    expr_id intern(expr&&);
    trail& trail_ref;
    sequencer& seq;
    registry<expr>& er;
    std::set<expr> exprs;
};

#endif

