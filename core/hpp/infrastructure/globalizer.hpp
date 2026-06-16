#ifndef GLOBALIZER_HPP
#define GLOBALIZER_HPP

#include "infrastructure/locator.hpp"
#include "interfaces/i_globalizer.hpp"
#include "interfaces/i_make_functor.hpp"
#include "interfaces/i_make_var.hpp"
#include "value_objects/framed_expr.hpp"

struct globalizer : i_globalizer {
    globalizer(locator& loc);
    const expr* globalize(framed_expr) override;
private:
    i_make_functor& make_functor_ref;
    i_make_var& make_var_ref;
};

#endif
