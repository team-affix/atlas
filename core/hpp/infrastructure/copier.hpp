#ifndef COPIER_HPP
#define COPIER_HPP

#include "infrastructure/locator.hpp"
#include "interfaces/i_copier.hpp"
#include "value_objects/expr.hpp"
#include "interfaces/i_var_sequencer.hpp"
#include "interfaces/i_make_functor.hpp"
#include "interfaces/i_make_var.hpp"

struct copier : i_copier {
    copier(locator& loc);
    const expr* copy(const expr*, translation_map&) const override;
private:
    i_var_sequencer& var_seq_ref;
    i_make_functor& make_functor_ref;
    i_make_var& make_var_ref;
};

#endif
