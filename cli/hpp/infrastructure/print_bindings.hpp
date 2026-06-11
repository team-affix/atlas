#ifndef PRINT_BINDINGS_HPP
#define PRINT_BINDINGS_HPP

#include "interfaces/i_print_bindings.hpp"

struct print_bindings : i_print_bindings {
    void print(
        i_runtime& runtime,
        i_expr_printer& printer,
        expr_pool& pool,
        const std::map<std::string, uint32_t>& var_name_to_idx) override;
};

#endif
