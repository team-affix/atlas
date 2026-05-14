#ifndef UNIFY_HEAD_HPP
#define UNIFY_HEAD_HPP

#include <memory>
#include "../interfaces/i_translation_map.hpp"
#include "../interfaces/i_unifier.hpp"

struct unify_head {
    std::unique_ptr<i_translation_map> tm;
    std::unique_ptr<i_unifier> u;
};

#endif
