#ifndef PUD_CANDIDATE_SEARCH_RESULT_HPP
#define PUD_CANDIDATE_SEARCH_RESULT_HPP

#include <compare>
#include <variant>

struct pud_candidate_search_result {
    struct choice_point {
        auto operator<=>(const choice_point&) const = default;
    };
    struct self_witness {
        auto operator<=>(const self_witness&) const = default;
    };
    struct axiom_refuted {
        auto operator<=>(const axiom_refuted&) const = default;
    };
    std::variant<choice_point, self_witness, axiom_refuted> content;
    auto operator<=>(const pud_candidate_search_result&) const = default;
};

#endif
