#ifndef NORMALIZER_HPP
#define NORMALIZER_HPP

#include "../domain/interfaces/i_normalizer.hpp"
#include "../domain/interfaces/i_expr_pool.hpp"
#include "../domain/interfaces/i_unifier.hpp"

struct normalizer : i_normalizer {
    normalizer();
    const expr* normalize(const expr*) override;
private:
    i_expr_pool& expr_pool_ref;
    i_unifier& unifier_ref;
};

#endif
