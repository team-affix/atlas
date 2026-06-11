#ifndef I_PRINT_BINDINGS_HPP
#define I_PRINT_BINDINGS_HPP

#include <cstdint>
#include <map>
#include <string>
#include "infrastructure/expr_pool.hpp"
#include "interfaces/i_expr_printer.hpp"
#include "interfaces/i_runtime.hpp"

struct i_print_bindings {
    virtual ~i_print_bindings() = default;
    virtual void print(
        i_runtime& runtime,
        i_expr_printer& printer,
        expr_pool& pool,
        const std::map<std::string, uint32_t>& var_name_to_idx) = 0;
};

#endif
