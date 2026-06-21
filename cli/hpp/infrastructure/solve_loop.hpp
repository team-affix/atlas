#ifndef SOLVE_LOOP_HPP
#define SOLVE_LOOP_HPP

#include <chrono>
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
    size_t total_sims    = 0;
    size_t last_printed  = 0;
    size_t res_depth_sum = 0;
    size_t dec_depth_sum = 0;
    double ema_res_depth = 0.0;
    double ema_dec_depth = 0.0;
    double ema_freq      = 0.0;
    bool   ema_init      = false;
    auto   last_time     = std::chrono::steady_clock::now();

    const auto do_print = [&](size_t sims) {
        const size_t interval = sims - last_printed;
        const auto now = std::chrono::steady_clock::now();
        const double elapsed = std::chrono::duration<double>(now - last_time).count();
        const double cur_res  = interval > 0 ? static_cast<double>(res_depth_sum) / interval : 0.0;
        const double cur_dec  = interval > 0 ? static_cast<double>(dec_depth_sum) / interval : 0.0;
        const double cur_freq = elapsed > 0.0 ? static_cast<double>(interval) / elapsed : 0.0;
        if (!ema_init) {
            ema_res_depth = cur_res;
            ema_dec_depth = cur_dec;
            ema_freq      = cur_freq;
            ema_init      = true;
        } else {
            ema_res_depth = 0.9 * ema_res_depth + 0.1 * cur_res;
            ema_dec_depth = 0.9 * ema_dec_depth + 0.1 * cur_dec;
            ema_freq      = 0.9 * ema_freq      + 0.1 * cur_freq;
        }
        print_progress_.print(sims, ema_res_depth, ema_dec_depth, ema_freq);
        res_depth_sum = 0;
        dec_depth_sum = 0;
        last_time     = now;
        last_printed  = sims;
    };

    while (runtime.next()) {
        ++total_sims;
        if (sim_progress_interval_ > 0) {
            res_depth_sum += runtime.resolution_depth();
            dec_depth_sum += runtime.decision_depth();
            if (total_sims % sim_progress_interval_ == 0)
                do_print(total_sims);
        }
        if (!runtime.solved()) continue;
        if (sim_progress_interval_ > 0) print_progress_.finish_line();
        std::cout << "SOLVED\n";
        print_bindings_.print(runtime, printer, pool, var_name_to_idx);
        std::cout << "[press Enter for next solution]";
        std::cin.get();
    }
    if (sim_progress_interval_ > 0 && total_sims > 0) {
        if (total_sims != last_printed) do_print(total_sims);
        print_progress_.finish_line();
    }
    std::cout << "REFUTED\n";
}

#endif
