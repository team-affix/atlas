#include "../hpp/expr_printer.hpp"

expr_printer::expr_printer(std::ostream& os) : os(os) {
}

void expr_printer::operator()(const expr* e) const {
    if (const expr::atom* a = std::get_if<expr::atom>(&e->content)) {
        os << a->value;
        return;
    }

    if (const expr::var* v = std::get_if<expr::var>(&e->content)) {
        os << "?" << v->index;
        return;
    }

    if (const expr::cons* c = std::get_if<expr::cons>(&e->content)) {
        os << "(";
        operator()(c->lhs);
        os << " . ";
        operator()(c->rhs);
        os << ")";
        return;
    }

    throw std::runtime_error("Unsupported expression type");
}
