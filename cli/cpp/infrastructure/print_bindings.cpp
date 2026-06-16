#include "infrastructure/print_bindings.hpp"
#include <iostream>

void print_bindings::print(
    i_runtime& runtime,
    i_expr_printer& printer,
    expr_pool& pool,
    const std::map<std::string, uint32_t>& var_name_to_idx) {
    for (const auto& [name, idx] : var_name_to_idx) {
        std::cout << "  " << name << " = ";
        printer.print(runtime.normalize({pool.make_var(idx), 0}));
        std::cout << "\n";
    }
}
