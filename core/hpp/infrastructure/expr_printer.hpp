#ifndef EXPR_PRINTER_HPP
#define EXPR_PRINTER_HPP

#include <ostream>
#include <stdexcept>
#include <vector>
#include "infrastructure/functor_names.hpp"
#include "infrastructure/var_names.hpp"
#include "value_objects/expr.hpp"

struct expr_printer {
    expr_printer(std::ostream& os, const var_names& vn, const functor_names& fn);
    void print(const expr*) const;
private:
    std::ostream& os;
    const var_names& var_names_;
    const functor_names& functor_names_;
};

#endif
