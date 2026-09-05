#ifndef ID_VAR_PRINTER_HPP
#define ID_VAR_PRINTER_HPP

#include <cstdint>
#include <ostream>

struct id_var_printer {
    id_var_printer();
    void print(std::ostream&, uint32_t index) const;
};

#endif
