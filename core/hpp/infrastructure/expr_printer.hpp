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

    struct piece {
        const expr* term;
        const char* text;
    };
};

template<typename IPrintVar, typename IPrintFunctor>
expr_printer<IPrintVar, IPrintFunctor>::expr_printer(
    std::ostream& os, IPrintVar& print_var, IPrintFunctor& print_functor)
    : os_(os), print_var_(print_var), print_functor_(print_functor) {}

// Prints with an explicit stack of pending pieces instead of recursing, so a term nested arbitrarily deep cannot
// exhaust the call stack. A piece is either a term still to print or text to write as it is.
template<typename IPrintVar, typename IPrintFunctor>
void expr_printer<IPrintVar, IPrintFunctor>::print(const expr* e) const {
    std::vector<piece> pending{piece{.term = e, .text = nullptr}};
    while (!pending.empty()) {
        const piece next = pending.back();
        pending.pop_back();
        if (next.term == nullptr) {
            os_ << next.text;
            continue;
        }
        if (const expr::var* v = std::get_if<expr::var>(&next.term->content)) {
            print_var_.print(os_, v->index);
            continue;
        }
        const expr::functor* f = std::get_if<expr::functor>(&next.term->content);
        if (f == nullptr)
            throw std::runtime_error("Unsupported expression type");
        if (f->args.empty()) {
            if (f->id == k_nil_functor_id) os_ << "[]";
            else print_functor_.print(os_, f->id);
            continue;
        }
        std::vector<piece> expanded;
        if (f->id == k_cons_functor_id && f->args.size() == 2) {
            expanded.push_back(piece{.term = nullptr, .text = "["});
            expanded.push_back(piece{.term = f->args[0], .text = nullptr});
            const expr* tail = f->args[1];
            while (true) {
                const expr::functor* tf = std::get_if<expr::functor>(&tail->content);
                if (tf && tf->id == k_nil_functor_id && tf->args.empty()) {
                    expanded.push_back(piece{.term = nullptr, .text = "]"});
                    break;
                }
                if (tf && tf->id == k_cons_functor_id && tf->args.size() == 2) {
                    expanded.push_back(piece{.term = nullptr, .text = ", "});
                    expanded.push_back(piece{.term = tf->args[0], .text = nullptr});
                    tail = tf->args[1];
                    continue;
                }
                expanded.push_back(piece{.term = nullptr, .text = "|"});
                expanded.push_back(piece{.term = tail, .text = nullptr});
                expanded.push_back(piece{.term = nullptr, .text = "]"});
                break;
            }
        } else {
            print_functor_.print(os_, f->id);
            expanded.push_back(piece{.term = nullptr, .text = "("});
            for (size_t i = 0; i < f->args.size(); ++i) {
                if (i > 0)
                    expanded.push_back(piece{.term = nullptr, .text = ", "});
                expanded.push_back(piece{.term = f->args[i], .text = nullptr});
            }
            expanded.push_back(piece{.term = nullptr, .text = ")"});
        }
        pending.insert(pending.end(), expanded.rbegin(), expanded.rend());
    }
}

#endif
