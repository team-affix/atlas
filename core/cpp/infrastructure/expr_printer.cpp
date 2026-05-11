#include "../../hpp/infrastructure/expr_printer.hpp"
#include "../../hpp/bootstrap/locator.hpp"

expr_printer::expr_printer(std::ostream& os)
    : os(os), var_names(locator::locate<i_var_names>())
{}

void expr_printer::print(const expr* e) const {
    if (const expr::var* v = std::get_if<expr::var>(&e->content)) {
        if (var_names.is_named(v->index))
            os << var_names.name(v->index);
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
            print(f->args[0]);
            const expr* tail = f->args[1];
            while (true) {
                const expr::functor* tf = std::get_if<expr::functor>(&tail->content);
                if (tf && tf->name == "nil" && tf->args.empty()) {
                    os << "]";
                    break;
                }
                if (tf && tf->name == "cons" && tf->args.size() == 2) {
                    os << ", ";
                    print(tf->args[0]);
                    tail = tf->args[1];
                } else {
                    os << "|";
                    print(tail);
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
            print(f->args[i]);
        }
        os << ")";
        return;
    }

    throw std::runtime_error("Unsupported expression type");
}
