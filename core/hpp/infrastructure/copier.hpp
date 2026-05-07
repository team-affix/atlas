#ifndef COPIER_HPP
#define COPIER_HPP

#include <unordered_map>
#include "../domain/interfaces/i_copier.hpp"
#include "../domain/value_objects/expr.hpp"
#include "../domain/interfaces/i_var_sequencer.hpp"
#include "../domain/interfaces/i_expr_pool.hpp"

struct copier : i_copier {
    copier();
    const expr* copy(const expr*, std::unordered_map<uint32_t, uint32_t>&) override;
private:
    i_var_sequencer& var_seq_ref;
    i_expr_pool& expr_pool_ref;
};

#endif
