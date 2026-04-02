#ifndef EXPR_PRINTER_HPP
#define EXPR_PRINTER_HPP

#include <ostream>
#include "expr.hpp"

struct expr_printer {
    expr_printer(std::ostream&);
    void operator()(const expr*) const;
#ifndef DEBUG
private:
#endif
    std::ostream& os;
};

#endif
