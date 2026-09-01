#ifndef RULE_PRINTER_HPP
#define RULE_PRINTER_HPP

#include <cstddef>
#include <ostream>
#include "value_objects/rule.hpp"

template<typename IPrintExpr>
struct rule_printer {
    rule_printer(std::ostream& os, IPrintExpr& print_expr);
    void print(const rule& r) const;
private:
    std::ostream& os_;
    IPrintExpr& print_expr_;
};

template<typename IPrintExpr>
rule_printer<IPrintExpr>::rule_printer(std::ostream& os, IPrintExpr& print_expr)
    : os_(os), print_expr_(print_expr) {}

template<typename IPrintExpr>
void rule_printer<IPrintExpr>::print(const rule& r) const {
    print_expr_.print(r.head);
    if (r.body.empty()) { os_ << "."; return; }
    os_ << " :-";
    for (size_t i = 0; i < r.body.size(); ++i) {
        os_ << (i == 0 ? " " : ", ");
        print_expr_.print(r.body[i]);
    }
    os_ << ".";
}

#endif
