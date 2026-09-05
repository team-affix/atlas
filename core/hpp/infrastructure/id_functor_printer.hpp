#ifndef ID_FUNCTOR_PRINTER_HPP
#define ID_FUNCTOR_PRINTER_HPP

#include <cstdint>
#include <ostream>

struct id_functor_printer {
    id_functor_printer();
    void print(std::ostream&, uint32_t id) const;
};

#endif
