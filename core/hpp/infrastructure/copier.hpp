#ifndef COPIER_HPP
#define COPIER_HPP

#include "../domain/interfaces/i_copier.hpp"
#include "../domain/value_objects/expr.hpp"
#include "../domain/interfaces/i_var_sequencer.hpp"
#include "../domain/interfaces/i_expr_pool.hpp"

struct copier : i_copier {
    copier();
    const expr* copy(const expr*, i_translation_map&) override;
private:
    i_var_sequencer& var_seq_ref;
    i_expr_pool& expr_pool_ref;
};

#endif
