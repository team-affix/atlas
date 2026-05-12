#include "../../../hpp/domain/entities/var_extractor.hpp"

var_extractor::var_extractor(std::unordered_set<const expr*>& vars) : vars_(vars) {}

void var_extractor::visit(const expr* e) {
    if (std::holds_alternative<expr::var>(e->content))
        vars_.insert(e);
}
