#ifndef EXPR_PRINTER_HPP
#define EXPR_PRINTER_HPP

#include <ostream>
#include "../domain/interfaces/i_expr_printer.hpp"
#include "../domain/interfaces/i_var_names.hpp"

struct expr_printer : i_expr_printer {
    expr_printer(std::ostream&);
    void print(const expr*) const override;
private:
    std::ostream& os;
    const i_var_names& var_names;
};

#endif
