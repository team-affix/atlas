#include "infrastructure/expr_printer.hpp"

expr_printer::expr_printer(std::ostream& os, const var_names& vn, const functor_names& fn)
    : os(os), var_names_(vn), functor_names_(fn) {}

void expr_printer::print(const expr* e) const {
    if (const expr::var* v = std::get_if<expr::var>(&e->content)) {
        if (var_names_.is_named(v->index)) os << var_names_.name(v->index);
        else os << "?" << v->index;
        return;
    }
    if (const expr::functor* f = std::get_if<expr::functor>(&e->content)) {
        if (f->args.empty()) {
            if (f->id == k_nil_functor_id) os << "[]";
            else if (functor_names_.is_named(f->id)) os << functor_names_.name(f->id);
            else os << "!" << f->id;
            return;
        }
        if (f->id == k_cons_functor_id && f->args.size() == 2) {
            os << "[";
            print(f->args[0]);
            const expr* tail = f->args[1];
            while (true) {
                const expr::functor* tf = std::get_if<expr::functor>(&tail->content);
                if (tf && tf->id == k_nil_functor_id && tf->args.empty()) { os << "]"; break; }
                if (tf && tf->id == k_cons_functor_id && tf->args.size() == 2) {
                    os << ", "; print(tf->args[0]); tail = tf->args[1];
                } else { os << "|"; print(tail); os << "]"; break; }
            }
            return;
        }
        if (functor_names_.is_named(f->id)) os << functor_names_.name(f->id);
        else os << "!" << f->id;
        os << "(";
        for (size_t i = 0; i < f->args.size(); ++i) {
            if (i > 0) os << ", ";
            print(f->args[i]);
        }
        os << ")";
        return;
    }
    throw std::runtime_error("Unsupported expression type");
}
