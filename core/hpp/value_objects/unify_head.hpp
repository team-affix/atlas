#ifndef UNIFY_HEAD_HPP
#define UNIFY_HEAD_HPP

#include <memory>
#include "../interfaces/i_bind_map.hpp"
#include "../interfaces/i_overlay_bind_map.hpp"
#include "../interfaces/i_unifier.hpp"

struct unify_head {
    std::unique_ptr<i_bind_map> local_bind_map;
    std::unique_ptr<i_overlay_bind_map> overlay_bind_map;
    std::unique_ptr<i_unifier> unifier;
};

#endif
