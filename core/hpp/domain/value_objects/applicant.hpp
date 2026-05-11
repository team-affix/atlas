#ifndef APPLICANT_HPP
#define APPLICANT_HPP

#include <memory>
#include <cstddef>
#include "../interfaces/i_translation_map.hpp"
#include "../interfaces/i_unifier.hpp"
#include "expr.hpp"

struct applicant {
    size_t rule_idx;
    const expr* copied_head;
    std::unique_ptr<i_translation_map> tm;
    std::unique_ptr<i_unifier> u;
};

#endif
