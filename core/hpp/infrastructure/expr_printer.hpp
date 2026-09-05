#ifndef EXPR_PRINTER_HPP
#define EXPR_PRINTER_HPP

#include <ostream>
#include <stdexcept>
#include <vector>
#include "infrastructure/functor_names.hpp"
#include "value_objects/expr.hpp"

template<typename IPrintVar, typename IPrintFunctor>
struct expr_printer {
    expr_printer(std::ostream& os, IPrintVar& print_var, IPrintFunctor& print_functor);
    void print(const expr*) const;
private:
    std::ostream& os_;
    IPrintVar& print_var_;
    IPrintFunctor& print_functor_;
};

template<typename IPrintVar, typename IPrintFunctor>
expr_printer<IPrintVar, IPrintFunctor>::expr_printer(
    std::ostream& os, IPrintVar& print_var, IPrintFunctor& print_functor)
    : os_(os), print_var_(print_var), print_functor_(print_functor) {}

template<typename IPrintVar, typename IPrintFunctor>
void expr_printer<IPrintVar, IPrintFunctor>::print(const expr* e) const {
    if (const expr::var* v = std::get_if<expr::var>(&e->content)) {
        print_var_.print(os_, v->index);
        return;
    }
    if (const expr::functor* f = std::get_if<expr::functor>(&e->content)) {
        if (f->args.empty()) {
            if (f->id == k_nil_functor_id) os_ << "[]";
            else print_functor_.print(os_, f->id);
            return;
        }
        if (f->id == k_cons_functor_id && f->args.size() == 2) {
            os_ << "[";
            print(f->args[0]);
            const expr* tail = f->args[1];
            while (true) {
                const expr::functor* tf = std::get_if<expr::functor>(&tail->content);
                if (tf && tf->id == k_nil_functor_id && tf->args.empty()) { os_ << "]"; break; }
                if (tf && tf->id == k_cons_functor_id && tf->args.size() == 2) {
                    os_ << ", "; print(tf->args[0]); tail = tf->args[1];
                } else { os_ << "|"; print(tail); os_ << "]"; break; }
            }
            return;
        }
        print_functor_.print(os_, f->id);
        os_ << "(";
        for (size_t i = 0; i < f->args.size(); ++i) {
            if (i > 0) os_ << ", ";
            print(f->args[i]);
        }
        os_ << ")";
        return;
    }
    throw std::runtime_error("Unsupported expression type");
}

#endif
