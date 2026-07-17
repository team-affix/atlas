#ifndef SOLVE_LOOP_HPP
#define SOLVE_LOOP_HPP

#include <cstddef>
#include <iostream>
#include <map>
#include <string>
#include "infrastructure/expr_pool.hpp"

template<typename IRuntime,
         typename IExprPrinter,
         typename IPrintBindings,
         typename IPrintProgress,
         typename IPause,
         typename IResume>
struct solve_loop {
    solve_loop(IPrintBindings&, IPrintProgress&, IPause&, IResume&, size_t interval);
    void run(IRuntime&, IExprPrinter&, expr_pool&, const std::map<std::string, uint32_t>&);
private:
    IPrintBindings& print_bindings_;
    IPrintProgress& print_progress_;
    IPause& pause_;
    IResume& resume_;
    size_t sim_progress_interval_;
};

template<typename IR, typename IEP, typename IPB, typename IPP, typename IPa, typename IRe>
solve_loop<IR, IEP, IPB, IPP, IPa, IRe>::solve_loop(
    IPB& pb, IPP& pp, IPa& pause, IRe& resume, size_t interval)
    : print_bindings_(pb)
    , print_progress_(pp)
    , pause_(pause)
    , resume_(resume)
    , sim_progress_interval_(interval)
{}

template<typename IR, typename IEP, typename IPB, typename IPP, typename IPa, typename IRe>
void solve_loop<IR, IEP, IPB, IPP, IPa, IRe>::run(
    IR& runtime, IEP& printer, expr_pool& pool,
    const std::map<std::string, uint32_t>& var_name_to_idx) {
    resume_.resume();
    size_t total_sims = 0;
    while (runtime.next()) {
        ++total_sims;
        if (sim_progress_interval_ > 0) {
            print_progress_.on_sim();
            if (total_sims % sim_progress_interval_ == 0)
                print_progress_.print();
        }
        if (!runtime.solved()) continue;
        if (sim_progress_interval_ > 0) print_progress_.finish_line();
        std::cout << "SOLVED\n";
        print_bindings_.print(runtime, printer, pool, var_name_to_idx);
        std::cout << "[press Enter for next solution]";
        pause_.pause();
        std::cin.get();
        resume_.resume();
    }
    if (sim_progress_interval_ > 0 && total_sims > 0) {
        print_progress_.print();
        print_progress_.finish_line();
    }
    std::cout << "REFUTED\n";
}

#endif
