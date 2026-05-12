#ifndef EXPR_TRAVERSER_HPP
#define EXPR_TRAVERSER_HPP

#include "../domain/interfaces/i_expr_traverser.hpp"

struct expr_traverser : i_expr_traverser {
    explicit expr_traverser(const expr* root);
    void accept(i_visitor<const expr*>&) override;
private:
    void traverse(const expr*, i_visitor<const expr*>&);
    const expr* root_;
};

#endif
