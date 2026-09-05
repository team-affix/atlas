#include "infrastructure/id_var_printer.hpp"

id_var_printer::id_var_printer() {}

void id_var_printer::print(std::ostream& os, uint32_t index) const {
    os << "_" << index;
}
