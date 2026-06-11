#ifndef SOLVE_LOOP_HPP
#define SOLVE_LOOP_HPP

#include "infrastructure/print_bindings.hpp"
#include "interfaces/i_run_solve_loop.hpp"

struct solve_loop : i_run_solve_loop {
    void run(
        i_runtime& runtime,
        i_expr_printer& printer,
        expr_pool& pool,
        const std::map<std::string, uint32_t>& var_name_to_idx) override;

private:
    print_bindings print_bindings_;
};

#endif
