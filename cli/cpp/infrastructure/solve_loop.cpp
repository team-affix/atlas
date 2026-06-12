#include "infrastructure/solve_loop.hpp"
#include <iostream>

solve_loop::solve_loop(
    i_print_bindings& print_bindings,
    i_print_progress& print_progress,
    size_t sim_progress_interval)
    : print_bindings_(print_bindings),
      print_progress_(print_progress),
      sim_progress_interval_(sim_progress_interval) {}

void solve_loop::run(
    i_runtime& runtime,
    i_expr_printer& printer,
    expr_pool& pool,
    const std::map<std::string, uint32_t>& var_name_to_idx) {
    size_t total_sims   = 0;
    size_t last_printed = 0;

    while (runtime.next()) {
        ++total_sims;
        if (sim_progress_interval_ > 0
            && total_sims % sim_progress_interval_ == 0) {
            print_progress_.print(total_sims);
            last_printed = total_sims;
        }
        if (!runtime.solved())
            continue;
        if (sim_progress_interval_ > 0)
            print_progress_.finish_line();
        std::cout << "SOLVED\n";
        print_bindings_.print(runtime, printer, pool, var_name_to_idx);
        std::cout << "[press Enter for next solution]";
        std::cin.get();
    }
    if (sim_progress_interval_ > 0 && total_sims > 0) {
        if (total_sims != last_printed)
            print_progress_.print(total_sims);
        print_progress_.finish_line();
    }
    std::cout << "REFUTED\n";
}
