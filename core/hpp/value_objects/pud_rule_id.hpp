#ifndef PUD_RULE_ID_HPP
#define PUD_RULE_ID_HPP

#include <compare>
#include <cstddef>
#include <variant>

struct pud_rule_id {
    struct axiom {
        size_t entry_idx;
        auto operator<=>(const axiom&) const = default;
    };
    struct inference {
        const pud_rule_id* caller;
        size_t call_site;
        const pud_rule_id* callee;
        auto operator<=>(const inference&) const = default;
    };
    std::variant<axiom, inference> content;
    auto operator<=>(const pud_rule_id&) const = default;
};

#endif
