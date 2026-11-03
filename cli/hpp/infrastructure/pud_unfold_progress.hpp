#ifndef PUD_UNFOLD_PROGRESS_HPP
#define PUD_UNFOLD_PROGRESS_HPP

#include <chrono>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include "value_objects/pud_focus_leaf_counts.hpp"

// Counts the session's unfolds and prints a progress line: total unfolds, unfolds per second and average
// unfold depth since the previous line, total nodes in the loaded modules, live and blocked leaves under
// the focus, how long ago the modules were last saved, and how long the solve has run. On a terminal
// each line overwrites the previous one in place, like print_progress for the sim solvers; otherwise
// each goes on its own line. finish_line() ends an in-place line so other output starts on a fresh one.
template<
    typename IGetTimeSinceStart,
    typename IGetUnfoldDepthSum,
    typename ICountNodes,
    typename ICountFocusLeaves,
    typename IGetTimeSinceLastSave>
struct pud_unfold_progress {
    pud_unfold_progress(
        IGetTimeSinceStart& get_time_since_start,
        IGetUnfoldDepthSum& get_unfold_depth_sum,
        ICountNodes& count_nodes,
        ICountFocusLeaves& count_focus_leaves,
        IGetTimeSinceLastSave& get_time_since_last_save,
        bool overwrite_line);
    void on_unfold();
    void print();
    void finish_line();
private:
    IGetTimeSinceStart& get_time_since_start_;
    IGetUnfoldDepthSum& get_unfold_depth_sum_;
    ICountNodes& count_nodes_;
    ICountFocusLeaves& count_focus_leaves_;
    IGetTimeSinceLastSave& get_time_since_last_save_;
    bool overwrite_line_;
    size_t total_unfolds_;
    size_t unfolds_since_last_;
    double last_seconds_;
    size_t last_depth_sum_;
    size_t previous_line_width_;
    bool line_active_;
};

#define PUD_UNFOLD_PROGRESS_TEMPLATE                                                                \
    template<typename IGetTimeSinceStart, typename IGetUnfoldDepthSum, typename ICountNodes,       \
             typename ICountFocusLeaves, typename IGetTimeSinceLastSave>
#define PUD_UNFOLD_PROGRESS                                                                         \
    pud_unfold_progress<IGetTimeSinceStart, IGetUnfoldDepthSum, ICountNodes, ICountFocusLeaves,    \
        IGetTimeSinceLastSave>

PUD_UNFOLD_PROGRESS_TEMPLATE
PUD_UNFOLD_PROGRESS::pud_unfold_progress(
    IGetTimeSinceStart& get_time_since_start,
    IGetUnfoldDepthSum& get_unfold_depth_sum,
    ICountNodes& count_nodes,
    ICountFocusLeaves& count_focus_leaves,
    IGetTimeSinceLastSave& get_time_since_last_save,
    bool overwrite_line)
    : get_time_since_start_(get_time_since_start)
    , get_unfold_depth_sum_(get_unfold_depth_sum)
    , count_nodes_(count_nodes)
    , count_focus_leaves_(count_focus_leaves)
    , get_time_since_last_save_(get_time_since_last_save)
    , overwrite_line_(overwrite_line)
    , total_unfolds_(0)
    , unfolds_since_last_(0)
    , last_seconds_(0.0)
    , last_depth_sum_(0)
    , previous_line_width_(0)
    , line_active_(false) {}

PUD_UNFOLD_PROGRESS_TEMPLATE
void PUD_UNFOLD_PROGRESS::on_unfold() {
    ++unfolds_since_last_;
}

PUD_UNFOLD_PROGRESS_TEMPLATE
void PUD_UNFOLD_PROGRESS::print() {
    if (unfolds_since_last_ == 0)
        return;
    const double seconds = std::chrono::duration<double>(get_time_since_start_.time_since_start()).count();
    const double elapsed = seconds - last_seconds_;
    const double unfolds_per_second = elapsed > 0.0 ? static_cast<double>(unfolds_since_last_) / elapsed : 0.0;
    const size_t depth_sum = get_unfold_depth_sum_.unfold_depth_sum();
    const double average_depth =
        static_cast<double>(depth_sum - last_depth_sum_) / static_cast<double>(unfolds_since_last_);
    const pud_focus_leaf_counts focus_leaves = count_focus_leaves_.count_focus_leaves();
    total_unfolds_ += unfolds_since_last_;
    unfolds_since_last_ = 0;
    last_seconds_ = seconds;
    last_depth_sum_ = depth_sum;
    std::ostringstream line;
    line << std::fixed << std::setprecision(1)
         << total_unfolds_ << " unfolds"
         << " | " << static_cast<size_t>(unfolds_per_second) << " unfolds/s"
         << " | avg depth " << average_depth
         << " | " << count_nodes_.count_nodes() << " nodes"
         << " | focus leaves: " << focus_leaves.live << " live, " << focus_leaves.blocked << " blocked";
    const auto since_save = get_time_since_last_save_.time_since_last_save();
    line << " | last saved: ";
    if (since_save.has_value())
        line << std::chrono::duration_cast<std::chrono::seconds>(since_save.value()).count() << "s ago";
    else
        line << "never";
    line << " | TSS: " << seconds << "s";
    const std::string text = line.str();
    if (!overwrite_line_) {
        std::cout << text << '\n' << std::flush;
        return;
    }
    const size_t padding_width = text.size() < previous_line_width_ ? previous_line_width_ - text.size() : 0;
    std::cout << '\r' << text << std::string(padding_width, ' ') << "\033[K" << std::flush;
    previous_line_width_ = text.size();
    line_active_ = true;
}

PUD_UNFOLD_PROGRESS_TEMPLATE
void PUD_UNFOLD_PROGRESS::finish_line() {
    if (!line_active_)
        return;
    std::cout << '\n' << std::flush;
    line_active_ = false;
    previous_line_width_ = 0;
}

#undef PUD_UNFOLD_PROGRESS
#undef PUD_UNFOLD_PROGRESS_TEMPLATE

#endif
