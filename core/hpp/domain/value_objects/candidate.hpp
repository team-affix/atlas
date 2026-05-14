#ifndef CANDIDATE_HPP
#define CANDIDATE_HPP

#include <memory>
#include "expr.hpp"
#include "../interfaces/i_translation_map.hpp"

struct candidate {
    virtual ~candidate() = default;
    const expr* copied_head;
    std::unique_ptr<i_translation_map> tm;
};

#endif
