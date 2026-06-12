#include "infrastructure/solve_loop.hpp"
#include <iostream>

solve_loop::solve_loop(i_print_bindings& print_bindings)
    : print_bindings_(print_bindings) {}

void solve_loop::run(
    i_runtime& runtime,
    i_expr_printer& printer,
    expr_pool& pool,
    const std::map<std::string, uint32_t>& var_name_to_idx) {
    while (runtime.next()) {
        if (!runtime.solved())
            continue;
        std::cout << "SOLVED\n";
        print_bindings_.print(runtime, printer, pool, var_name_to_idx);
        std::cout << "[press Enter for next solution]";
        std::cin.get();
    }
    std::cout << "REFUTED\n";
}
