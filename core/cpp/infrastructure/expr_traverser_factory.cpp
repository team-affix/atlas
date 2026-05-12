#include "../../hpp/infrastructure/expr_traverser_factory.hpp"
#include "../../hpp/infrastructure/expr_traverser.hpp"

std::unique_ptr<i_expr_traverser> expr_traverser_factory::make(const expr* e) {
    return std::make_unique<expr_traverser>(e);
}
