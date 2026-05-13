#ifndef CANDIDATE_HPP
#define CANDIDATE_HPP

#include <memory>
#include "../interfaces/i_translation_map.hpp"
#include "../interfaces/i_unifier.hpp"

struct candidate {
    virtual ~candidate() = default;
    std::unique_ptr<i_translation_map> tm;
    std::unique_ptr<i_unifier> u;
};

#endif
