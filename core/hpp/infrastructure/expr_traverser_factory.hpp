#ifndef EXPR_TRAVERSER_FACTORY_HPP
#define EXPR_TRAVERSER_FACTORY_HPP

#include "../domain/interfaces/i_expr_traverser_factory.hpp"

struct expr_traverser_factory : i_expr_traverser_factory {
    std::unique_ptr<i_expr_traverser> make(const expr*) override;
};

#endif
