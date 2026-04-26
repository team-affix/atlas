#ifndef EXPR_HPP
#define EXPR_HPP

// This file is dedicated to defining s-expressions used for this project, as well as
// functions that operate on them such as unifications.

#include <cstdint>
#include <string>
#include <vector>
#include <variant>
#include <set>
#include "trail.hpp"

struct expr {
    struct functor {
        std::string name;
        std::vector<const expr*> args;
        auto operator<=>(const functor&) const = default;
    };
    struct var  { uint32_t index; auto operator<=>(const var&) const = default; };
    std::variant<functor, var> content;
    auto operator<=>(const expr&) const = default;
};

struct expr_pool {
    expr_pool();
    const expr* functor(const std::string& name, std::vector<const expr*> args = {});
    const expr* var(uint32_t);
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

