#include "../hpp/expr_printer.hpp"

expr_printer::expr_printer(std::ostream& os, const std::map<uint32_t, std::string>& var_names)
    : os(os), var_names(var_names)
{}

void expr_printer::operator()(const expr* e) const {
    if (const expr::var* v = std::get_if<expr::var>(&e->content)) {
        auto it = var_names.find(v->index);
        if (it != var_names.end())
            os << it->second;
        else
            os << "?" << v->index;
        return;
    }

    if (const expr::functor* f = std::get_if<expr::functor>(&e->content)) {
        // Nullary functor (atom-like)
        if (f->args.empty()) {
            if (f->name == "nil")
                os << "[]";
            else
                os << f->name;
            return;
        }

        // List spine: cons(head, tail)
        if (f->name == "cons" && f->args.size() == 2) {
            os << "[";
            operator()(f->args[0]);
            const expr* tail = f->args[1];
            while (true) {
                const expr::functor* tf = std::get_if<expr::functor>(&tail->content);
                if (tf && tf->name == "nil" && tf->args.empty()) {
                    os << "]";
                    break;
                }
                if (tf && tf->name == "cons" && tf->args.size() == 2) {
                    os << ", ";
                    operator()(tf->args[0]);
                    tail = tf->args[1];
                } else {
                    os << "|";
                    operator()(tail);
                    os << "]";
                    break;
                }
            }
            return;
        }

        // General functor: name(arg1, arg2, ...)
        os << f->name << "(";
        for (size_t i = 0; i < f->args.size(); ++i) {
            if (i > 0) os << ", ";
            operator()(f->args[i]);
        }
        os << ")";
        return;
    }

    throw std::runtime_error("Unsupported expression type");
}
