#ifndef EXPR_PRINTER_HPP
#define EXPR_PRINTER_HPP

#include <ostream>
#include "infrastructure/locator.hpp"
#include "interfaces/i_expr_printer.hpp"
#include "interfaces/i_atom_names.hpp"
#include "interfaces/i_var_names.hpp"
#include "infrastructure/atom_names.hpp"

struct expr_printer : i_expr_printer {
    expr_printer(std::ostream& os, locator& loc);
    void print(const expr*) const override;
private:
    std::ostream& os;
    const i_var_names& var_names;
    const i_atom_names& atom_names;
};

#endif
