#include "../../hpp/infrastructure/expr_traverser.hpp"

expr_traverser::expr_traverser(const expr* root) : root_(root) {}

void expr_traverser::accept(i_visitor<const expr*>& v) {
    traverse(root_, v);
}

void expr_traverser::traverse(const expr* e, i_visitor<const expr*>& v) {
    v.visit(e);
    if (const expr::functor* f = std::get_if<expr::functor>(&e->content)) {
        for (const expr* arg : f->args)
            traverse(arg, v);
    }
}
