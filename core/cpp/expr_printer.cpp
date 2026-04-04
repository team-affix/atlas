#include "../hpp/expr_printer.hpp"

const std::map<uint32_t, std::string> expr_printer::empty_var_names;

expr_printer::expr_printer(std::ostream& os, const std::map<uint32_t, std::string>& var_names)
    : os(os), var_names(var_names)
{}

void expr_printer::operator()(const expr* e) const {
    if (const expr::atom* a = std::get_if<expr::atom>(&e->content)) {
        os << a->value;
        return;
    }

    if (const expr::var* v = std::get_if<expr::var>(&e->content)) {
        auto it = var_names.find(v->index);
        if (it != var_names.end())
            os << it->second;
        else
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
