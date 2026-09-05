#ifndef NAMED_VAR_PRINTER_HPP
#define NAMED_VAR_PRINTER_HPP

#include <cstdint>
#include <ostream>

template<typename IVarNames>
struct named_var_printer {
    named_var_printer(IVarNames& var_names);
    void print(std::ostream&, uint32_t index) const;
private:
    IVarNames& var_names_;
};

template<typename IVarNames>
named_var_printer<IVarNames>::named_var_printer(IVarNames& var_names)
    : var_names_(var_names) {}

template<typename IVarNames>
void named_var_printer<IVarNames>::print(std::ostream& os, uint32_t index) const {
    if (var_names_.is_named(index)) {
        os << var_names_.name(index);
        return;
    }
    os << "?" << index;
}

#endif
