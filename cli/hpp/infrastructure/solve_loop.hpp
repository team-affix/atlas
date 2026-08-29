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
         typename IOnSim,
         typename IPrintStats,
         typename IFinishPrintingLine,
         typename IPauseTimer,
         typename IResumeTimer>
struct solve_loop {
    solve_loop(IPrintBindings&, IOnSim&, IPrintStats&, IFinishPrintingLine&,
               IPauseTimer&, IResumeTimer&, size_t interval);
    void run(IRuntime&, IExprPrinter&, expr_pool&, const std::map<std::string, uint32_t>&);
private:
    IPrintBindings& print_bindings_;
    IOnSim& on_sim_;
    IPrintStats& print_stats_;
    IFinishPrintingLine& finish_printing_line_;
    IPauseTimer& pause_timer_;
    IResumeTimer& resume_timer_;
    size_t sim_progress_interval_;
};

template<typename IR, typename IEP, typename IPB, typename IOS, typename IPS,
         typename IFPL, typename IPT, typename IRT>
solve_loop<IR, IEP, IPB, IOS, IPS, IFPL, IPT, IRT>::solve_loop(
    IPB& pb, IOS& on_sim, IPS& print_stats, IFPL& finish_printing_line,
    IPT& pause_timer, IRT& resume_timer, size_t interval)
    : print_bindings_(pb)
    , on_sim_(on_sim)
    , print_stats_(print_stats)
    , finish_printing_line_(finish_printing_line)
    , pause_timer_(pause_timer)
    , resume_timer_(resume_timer)
    , sim_progress_interval_(interval)
{}

template<typename IR, typename IEP, typename IPB, typename IOS, typename IPS,
         typename IFPL, typename IPT, typename IRT>
void solve_loop<IR, IEP, IPB, IOS, IPS, IFPL, IPT, IRT>::run(
    IR& runtime, IEP& printer, expr_pool& pool,
    const std::map<std::string, uint32_t>& var_name_to_idx) {
    resume_timer_.resume();
    size_t total_sims = 0;
    while (runtime.next()) {
        ++total_sims;
        if (sim_progress_interval_ > 0) {
            on_sim_.on_sim();
            if (total_sims % sim_progress_interval_ == 0)
                print_stats_.print();
        }
        if (!runtime.solved()) continue;
        if (sim_progress_interval_ > 0) finish_printing_line_.finish_line();
        std::cout << "SOLVED\n";
        print_bindings_.print(runtime, printer, pool, var_name_to_idx);
        std::cout << "[press Enter for next solution]";
        pause_timer_.pause();
        std::cin.get();
        resume_timer_.resume();
    }
    if (sim_progress_interval_ > 0 && total_sims > 0) {
        print_stats_.print();
        finish_printing_line_.finish_line();
    }
    std::cout << "REFUTED\n";
}

#endif
