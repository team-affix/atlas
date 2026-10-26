#ifndef FRAMED_FUNCTOR_HASH_HPP
#define FRAMED_FUNCTOR_HASH_HPP

#include <cstddef>
#include "value_objects/framed_functor.hpp"

struct framed_functor_hash {
    size_t operator()(const framed_functor& framed) const noexcept;
};

#endif
