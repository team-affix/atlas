#include "infrastructure/id_functor_printer.hpp"

id_functor_printer::id_functor_printer() {}

void id_functor_printer::print(std::ostream& os, uint32_t id) const {
    os << "f" << id;
}
