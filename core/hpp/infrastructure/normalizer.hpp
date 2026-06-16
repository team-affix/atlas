#ifndef NORMALIZER_HPP
#define NORMALIZER_HPP

#include "infrastructure/locator.hpp"
#include "interfaces/i_normalizer.hpp"
#include "interfaces/i_globalizer.hpp"
#include "interfaces/i_make_functor.hpp"
#include "interfaces/i_bind_map.hpp"

struct normalizer : i_normalizer {
    normalizer(locator& loc);
    const expr* normalize(const expr*) override;
private:
    i_globalizer& globalizer_ref;
    i_make_functor& make_functor_ref;
    i_bind_map& bind_map_ref;
};

#endif
