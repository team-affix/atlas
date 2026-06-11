#ifndef I_RUN_SOLVE_LOOP_HPP
#define I_RUN_SOLVE_LOOP_HPP

#include <cstdint>
#include <map>
#include <string>
#include "infrastructure/expr_pool.hpp"
#include "interfaces/i_expr_printer.hpp"
#include "interfaces/i_runtime.hpp"

struct i_run_solve_loop {
    virtual ~i_run_solve_loop() = default;
    virtual void run(
        i_runtime& runtime,
        i_expr_printer& printer,
        expr_pool& pool,
        const std::map<std::string, uint32_t>& var_name_to_idx) = 0;
};

#endif
