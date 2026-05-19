#ifndef COPIER_HPP
#define COPIER_HPP

#include "../interfaces/i_copier.hpp"
#include "../value_objects/expr.hpp"
#include "../interfaces/i_var_sequencer.hpp"
#include "../interfaces/i_expr_pool.hpp"

struct copier : i_copier {
    copier(
        i_var_sequencer&,
        i_expr_pool&);
    const expr* copy(const expr*, translation_map&) const override;
private:
    i_var_sequencer& var_seq_ref;
    i_expr_pool& expr_pool_ref;
};

#endif
