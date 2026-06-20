#ifndef SOLVE_LOOP_HPP
#define SOLVE_LOOP_HPP

#include <cstddef>
#include <iostream>
#include <map>
#include <string>
#include "infrastructure/expr_pool.hpp"

template<typename IRuntime, typename IExprPrinter, typename IPrintBindings, typename IPrintProgress>
struct solve_loop {
    solve_loop(IPrintBindings&, IPrintProgress&, size_t interval = 1000);
    void run(IRuntime&, IExprPrinter&, expr_pool&, const std::map<std::string, uint32_t>&);
private:
    IPrintBindings& print_bindings_;
    IPrintProgress& print_progress_;
    size_t sim_progress_interval_;
};

template<typename IR, typename IEP, typename IPB, typename IPP>
solve_loop<IR,IEP,IPB,IPP>::solve_loop(IPB& pb, IPP& pp, size_t interval)
    : print_bindings_(pb), print_progress_(pp), sim_progress_interval_(interval) {}

template<typename IR, typename IEP, typename IPB, typename IPP>
void solve_loop<IR,IEP,IPB,IPP>::run(
    IR& runtime, IEP& printer, expr_pool& pool,
    const std::map<std::string, uint32_t>& var_name_to_idx) {
    size_t total_sims = 0;
    size_t last_printed = 0;
    while (runtime.next()) {
        ++total_sims;
        if (sim_progress_interval_ > 0 && total_sims % sim_progress_interval_ == 0) {
            print_progress_.print(total_sims);
            last_printed = total_sims;
        }
        if (!runtime.solved()) continue;
        if (sim_progress_interval_ > 0) print_progress_.finish_line();
        std::cout << "SOLVED\n";
        print_bindings_.print(runtime, printer, pool, var_name_to_idx);
        std::cout << "[press Enter for next solution]";
        std::cin.get();
    }
    if (sim_progress_interval_ > 0 && total_sims > 0) {
        if (total_sims != last_printed) print_progress_.print(total_sims);
        print_progress_.finish_line();
    }
    std::cout << "REFUTED\n";
}

#endif
