#ifndef PUD_SOLVE_LOOP_HPP
#define PUD_SOLVE_LOOP_HPP

#include <cstddef>
#include <iostream>
#include "value_objects/expr.hpp"
#include "value_objects/pud_search_step.hpp"
#include "value_objects/pud_search_stop_reason.hpp"

// Steps a PUD solver session to its end, like solve_loop does for the sim solvers: one unfold is the
// atomic unit (the sim), so progress is printed every interval unfolds. Every solution is printed and
// waits for Enter. After every step, and once at the end, the module saver gets a chance to write the
// modules back. The last line says how the session ended: REFUTED, EXHAUSTED, or nothing when the unfold
// budget ran out (so a caller can run the session again with more budget).
template<
    typename INextStep,
    typename IGetLastStep,
    typename IGetSolution,
    typename IGetSolutionDepth,
    typename IGetStopReason,
    typename IPrintExpr,
    typename IOnUnfold,
    typename IPrintStats,
    typename IPauseTimer,
    typename IResumeTimer,
    typename ITrySaveModules,
    typename ITrySaveModulesFinal>
struct pud_solve_loop {
    pud_solve_loop(
        INextStep& next_step,
        IGetLastStep& get_last_step,
        IGetSolution& get_solution,
        IGetSolutionDepth& get_solution_depth,
        IGetStopReason& get_stop_reason,
        IPrintExpr& print_expr,
        IOnUnfold& on_unfold,
        IPrintStats& print_stats,
        IPauseTimer& pause_timer,
        IResumeTimer& resume_timer,
        ITrySaveModules& try_save_modules,
        ITrySaveModulesFinal& try_save_modules_final,
        size_t interval);
    void run();
private:
    INextStep& next_step_;
    IGetLastStep& get_last_step_;
    IGetSolution& get_solution_;
    IGetSolutionDepth& get_solution_depth_;
    IGetStopReason& get_stop_reason_;
    IPrintExpr& print_expr_;
    IOnUnfold& on_unfold_;
    IPrintStats& print_stats_;
    IPauseTimer& pause_timer_;
    IResumeTimer& resume_timer_;
    ITrySaveModules& try_save_modules_;
    ITrySaveModulesFinal& try_save_modules_final_;
    size_t interval_;
};

#define PUD_SOLVE_LOOP_TEMPLATE                                                                     \
    template<typename INextStep, typename IGetLastStep, typename IGetSolution, typename IGetSolutionDepth, \
             typename IGetStopReason, typename IPrintExpr, typename IOnUnfold, typename IPrintStats,  \
             typename IPauseTimer, typename IResumeTimer, typename ITrySaveModules,                 \
             typename ITrySaveModulesFinal>
#define PUD_SOLVE_LOOP                                                                              \
    pud_solve_loop<INextStep, IGetLastStep, IGetSolution, IGetSolutionDepth, IGetStopReason, IPrintExpr, \
        IOnUnfold, IPrintStats, IPauseTimer, IResumeTimer, ITrySaveModules, ITrySaveModulesFinal>

PUD_SOLVE_LOOP_TEMPLATE
PUD_SOLVE_LOOP::pud_solve_loop(
    INextStep& next_step,
    IGetLastStep& get_last_step,
    IGetSolution& get_solution,
    IGetSolutionDepth& get_solution_depth,
    IGetStopReason& get_stop_reason,
    IPrintExpr& print_expr,
    IOnUnfold& on_unfold,
    IPrintStats& print_stats,
    IPauseTimer& pause_timer,
    IResumeTimer& resume_timer,
    ITrySaveModules& try_save_modules,
    ITrySaveModulesFinal& try_save_modules_final,
    size_t interval)
    : next_step_(next_step)
    , get_last_step_(get_last_step)
    , get_solution_(get_solution)
    , get_solution_depth_(get_solution_depth)
    , get_stop_reason_(get_stop_reason)
    , print_expr_(print_expr)
    , on_unfold_(on_unfold)
    , print_stats_(print_stats)
    , pause_timer_(pause_timer)
    , resume_timer_(resume_timer)
    , try_save_modules_(try_save_modules)
    , try_save_modules_final_(try_save_modules_final)
    , interval_(interval) {}

PUD_SOLVE_LOOP_TEMPLATE
void PUD_SOLVE_LOOP::run() {
    resume_timer_.resume();
    size_t total_unfolds = 0;
    while (next_step_.next()) {
        try_save_modules_.try_save();
        const pud_search_step step = get_last_step_.last_step();
        if (step == pud_search_step::unfolded) {
            ++total_unfolds;
            if (interval_ == 0)
                continue;
            on_unfold_.on_unfold();
            const bool boundary = total_unfolds % interval_ == 0;
            if (boundary)
                print_stats_.print();
            continue;
        }
        if (step != pud_search_step::solved)
            continue;
        std::cout << "SOLVED\n";
        print_expr_.print(get_solution_.solution());
        std::cout << '\n';
        std::cout << "depth " << get_solution_depth_.solution_depth() << '\n';
        std::cout << "[press Enter for next solution]";
        pause_timer_.pause();
        std::cin.get();
        resume_timer_.resume();
    }
    if (interval_ > 0 && total_unfolds > 0)
        print_stats_.print();
    try_save_modules_final_.try_save_final();
    const pud_search_stop_reason reason = get_stop_reason_.stop_reason();
    if (reason == pud_search_stop_reason::refuted)
        std::cout << "REFUTED\n";
    if (reason == pud_search_stop_reason::exhausted)
        std::cout << "EXHAUSTED\n";
}

#undef PUD_SOLVE_LOOP
#undef PUD_SOLVE_LOOP_TEMPLATE

#endif
