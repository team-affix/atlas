#ifndef SOLVE_LOOP_HPP
#define SOLVE_LOOP_HPP

#include "interfaces/i_print_bindings.hpp"
#include "interfaces/i_print_progress.hpp"
#include "interfaces/i_run_solve_loop.hpp"

struct solve_loop : i_run_solve_loop {
    solve_loop(
        i_print_bindings& print_bindings,
        i_print_progress& print_progress,
        size_t sim_progress_interval = 1000);

    void run(
        i_runtime& runtime,
        i_expr_printer& printer,
        expr_pool& pool,
        const std::map<std::string, uint32_t>& var_name_to_idx) override;

private:
    i_print_bindings& print_bindings_;
    i_print_progress& print_progress_;
    size_t sim_progress_interval_;
};

#endif
