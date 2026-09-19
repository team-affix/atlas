#ifndef PUD_WITNESS_SEARCH_RESULT_HPP
#define PUD_WITNESS_SEARCH_RESULT_HPP

#include <compare>
#include <variant>

struct pud_witness_search_result {
    struct found {
        auto operator<=>(const found&) const = default;
    };
    struct failed {
        auto operator<=>(const failed&) const = default;
    };
    std::variant<found, failed> content;
    auto operator<=>(const pud_witness_search_result&) const = default;
};

#endif
