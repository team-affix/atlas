#ifndef PUD_SOLVE_LOOP_HPP
#define PUD_SOLVE_LOOP_HPP

#include <cstddef>
#include <iostream>
#include "value_objects/expr.hpp"
#include "value_objects/pud_search_stop_reason.hpp"

// Ticks a PUD solver session to its end, printing every solution and waiting for Enter after each.
// The last line says how the session ended: REFUTED, EXHAUSTED, or nothing when the unfold budget
// ran out (so a caller can run the session again with more budget).
template<
    typename INextTick,
    typename IGetSolution,
    typename IGetSolutionDepth,
    typename IGetStopReason,
    typename IPrintExpr,
    typename IOnTick,
    typename IPrintStats,
    typename IPauseTimer,
    typename IResumeTimer>
struct pud_solve_loop {
    pud_solve_loop(
        INextTick& next_tick,
        IGetSolution& get_solution,
        IGetSolutionDepth& get_solution_depth,
        IGetStopReason& get_stop_reason,
        IPrintExpr& print_expr,
        IOnTick& on_tick,
        IPrintStats& print_stats,
        IPauseTimer& pause_timer,
        IResumeTimer& resume_timer,
        size_t interval);
    void run();
private:
    INextTick& next_tick_;
    IGetSolution& get_solution_;
    IGetSolutionDepth& get_solution_depth_;
    IGetStopReason& get_stop_reason_;
    IPrintExpr& print_expr_;
    IOnTick& on_tick_;
    IPrintStats& print_stats_;
    IPauseTimer& pause_timer_;
    IResumeTimer& resume_timer_;
    size_t interval_;
};

#define PUD_SOLVE_LOOP_TEMPLATE                                                                     \
    template<typename INextTick, typename IGetSolution, typename IGetSolutionDepth, typename IGetStopReason, \
             typename IPrintExpr, typename IOnTick, typename IPrintStats, typename IPauseTimer,     \
             typename IResumeTimer>
#define PUD_SOLVE_LOOP                                                                              \
    pud_solve_loop<INextTick, IGetSolution, IGetSolutionDepth, IGetStopReason, IPrintExpr, IOnTick, \
        IPrintStats, IPauseTimer, IResumeTimer>

PUD_SOLVE_LOOP_TEMPLATE
PUD_SOLVE_LOOP::pud_solve_loop(
    INextTick& next_tick,
    IGetSolution& get_solution,
    IGetSolutionDepth& get_solution_depth,
    IGetStopReason& get_stop_reason,
    IPrintExpr& print_expr,
    IOnTick& on_tick,
    IPrintStats& print_stats,
    IPauseTimer& pause_timer,
    IResumeTimer& resume_timer,
    size_t interval)
    : next_tick_(next_tick)
    , get_solution_(get_solution)
    , get_solution_depth_(get_solution_depth)
    , get_stop_reason_(get_stop_reason)
    , print_expr_(print_expr)
    , on_tick_(on_tick)
    , print_stats_(print_stats)
    , pause_timer_(pause_timer)
    , resume_timer_(resume_timer)
    , interval_(interval) {}

PUD_SOLVE_LOOP_TEMPLATE
void PUD_SOLVE_LOOP::run() {
    resume_timer_.resume();
    size_t total_ticks = 0;
    while (next_tick_.next()) {
        ++total_ticks;
        if (interval_ > 0) {
            on_tick_.on_tick();
            const bool boundary = total_ticks % interval_ == 0;
            if (boundary)
                print_stats_.print();
        }
        const expr* head = get_solution_.solution();
        if (head == nullptr)
            continue;
        std::cout << "SOLVED\n";
        print_expr_.print(head);
        std::cout << '\n';
        std::cout << "depth " << get_solution_depth_.solution_depth() << '\n';
        std::cout << "[press Enter for next solution]";
        pause_timer_.pause();
        std::cin.get();
        resume_timer_.resume();
    }
    if (interval_ > 0 && total_ticks > 0)
        print_stats_.print();
    const pud_search_stop_reason reason = get_stop_reason_.stop_reason();
    if (reason == pud_search_stop_reason::refuted)
        std::cout << "REFUTED\n";
    if (reason == pud_search_stop_reason::exhausted)
        std::cout << "EXHAUSTED\n";
}

#undef PUD_SOLVE_LOOP
#undef PUD_SOLVE_LOOP_TEMPLATE

#endif
