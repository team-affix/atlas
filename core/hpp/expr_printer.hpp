#ifndef EXPR_PRINTER_HPP
#define EXPR_PRINTER_HPP

#include <ostream>
#include <map>
#include <string>
#include <cstdint>
#include "expr.hpp"

struct expr_printer {
    expr_printer(std::ostream&, const std::map<uint32_t, std::string>& var_names = empty_var_names);
    void operator()(const expr*) const;
#ifndef DEBUG
private:
#endif
    static const std::map<uint32_t, std::string> empty_var_names;
    std::ostream& os;
    const std::map<uint32_t, std::string>& var_names;
};

#endif
