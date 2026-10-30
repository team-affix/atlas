#ifndef PUD_SOLVE_LOOP_HPP
#define PUD_SOLVE_LOOP_HPP

#include <cstddef>
#include <iostream>
#include "value_objects/expr.hpp"

template<typename IRuntime,
         typename IExprPrinter,
         typename IOnTick,
         typename IPrintStats,
         typename IFinishPrintingLine,
         typename IPauseTimer,
         typename IResumeTimer>
struct pud_solve_loop {
    pud_solve_loop(
        IOnTick& on_tick,
        IPrintStats& print_stats,
        IFinishPrintingLine& finish_printing_line,
        IPauseTimer& pause_timer,
        IResumeTimer& resume_timer,
        size_t interval);
    void run(IRuntime& runtime, IExprPrinter& printer);
private:
    IOnTick& on_tick_;
    IPrintStats& print_stats_;
    IFinishPrintingLine& finish_printing_line_;
    IPauseTimer& pause_timer_;
    IResumeTimer& resume_timer_;
    size_t interval_;
};

template<typename IR, typename IEP, typename IOT, typename IPS, typename IFPL, typename IPT, typename IRT>
pud_solve_loop<IR, IEP, IOT, IPS, IFPL, IPT, IRT>::pud_solve_loop(
    IOT& on_tick,
    IPS& print_stats,
    IFPL& finish_printing_line,
    IPT& pause_timer,
    IRT& resume_timer,
    size_t interval)
    : on_tick_(on_tick)
    , print_stats_(print_stats)
    , finish_printing_line_(finish_printing_line)
    , pause_timer_(pause_timer)
    , resume_timer_(resume_timer)
    , interval_(interval) {}

template<typename IR, typename IEP, typename IOT, typename IPS, typename IFPL, typename IPT, typename IRT>
void pud_solve_loop<IR, IEP, IOT, IPS, IFPL, IPT, IRT>::run(IR& runtime, IEP& printer) {
    resume_timer_.resume();
    size_t total_ticks = 0;
    while (runtime.next()) {
        ++total_ticks;
        if (interval_ > 0) {
            on_tick_.on_tick();
            const bool boundary = total_ticks % interval_ == 0;
            if (boundary)
                print_stats_.print();
        }
        const expr* head = runtime.solution();
        if (head == nullptr)
            continue;
        if (interval_ > 0)
            finish_printing_line_.finish_line();
        std::cout << "SOLVED\n";
        printer.print(head);
        std::cout << '\n';
        std::cout << "depth " << runtime.solution_depth() << '\n';
        std::cout << "[press Enter for next solution]";
        pause_timer_.pause();
        std::cin.get();
        resume_timer_.resume();
    }
    if (interval_ > 0 && total_ticks > 0) {
        print_stats_.print();
        finish_printing_line_.finish_line();
    }
    std::cout << "REFUTED\n";
}

#endif
