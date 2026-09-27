#ifndef PUD_LINEAGE_HPP
#define PUD_LINEAGE_HPP

#include <compare>
#include <cstddef>
#include <variant>

struct pud_lineage {
    struct axiom {
        size_t entry_idx;
        auto operator<=>(const axiom&) const = default;
    };
    struct inference {
        const pud_lineage* caller;
        size_t call_site;
        const pud_lineage* callee;
        auto operator<=>(const inference&) const = default;
    };
    std::variant<axiom, inference> content;
    auto operator<=>(const pud_lineage&) const = default;
};

#endif
