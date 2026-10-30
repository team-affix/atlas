#ifndef MODULED_FUNCTOR_HASH_HPP
#define MODULED_FUNCTOR_HASH_HPP

#include <cstddef>
#include "value_objects/moduled_functor.hpp"

struct moduled_functor_hash {
    size_t operator()(const moduled_functor& functor) const noexcept;
};

#endif
