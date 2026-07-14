#ifndef DBUCT_AVOIDANCE_UNIT_BOUNDARY_HPP
#define DBUCT_AVOIDANCE_UNIT_BOUNDARY_HPP

#include <cstddef>
#include <deque>
#include <list>
#include <stack>
#include "value_objects/avoidance_boundary_action.hpp"
#include "value_objects/lineage.hpp"
#include "debug_assert.hpp"

template<typename IGetNearestDecision, typename IGetMctsFrameDepth>
struct dbuct_avoidance_unit_boundary {
    dbuct_avoidance_unit_boundary(IGetNearestDecision& nd, IGetMctsFrameDepth& mcts_frame_depth);

    void log_decision(const resolution_lineage* rl);
    size_t get_penultimate_mcts_frame_depth() const;
    const resolution_lineage* get_ultimate_decision() const;
    const resolution_lineage* get_penultimate_decision() const;
    size_t get_ultimate_mcts_frame_depth() const;

    void push_frame();
    void pop_frame();

private:
    struct frame {
        std::list<avoidance_boundary_action> actions_;
    };

    void assign_ultimate(const resolution_lineage* rl);
    void assign_penultimate(const resolution_lineage* rl);
    void assign_ultimate_mcts_frame_depth(size_t value);
    void assign_penultimate_mcts_frame_depth(size_t value);
    void log(avoidance_boundary_action action);
    void undo_action(const avoidance_boundary_action& action);

    const resolution_lineage* ultimate_;
    const resolution_lineage* penultimate_;
    size_t ultimate_mcts_frame_depth_;
    size_t penultimate_mcts_frame_depth_;

    IGetNearestDecision& get_nearest_decision_;
    IGetMctsFrameDepth& get_mcts_frame_depth_;
    std::stack<frame> frame_stack_;
};

template<typename IGetNearestDecision, typename IGetMctsFrameDepth>
dbuct_avoidance_unit_boundary<IGetNearestDecision, IGetMctsFrameDepth>::dbuct_avoidance_unit_boundary(
    IGetNearestDecision& nd, IGetMctsFrameDepth& mcts_frame_depth)
    : ultimate_(nullptr), penultimate_(nullptr), ultimate_mcts_frame_depth_(0),
      penultimate_mcts_frame_depth_(0), get_nearest_decision_(nd), get_mcts_frame_depth_(mcts_frame_depth),
      frame_stack_(std::deque<frame>{frame{}}) {}

template<typename IGetNearestDecision, typename IGetMctsFrameDepth>
void dbuct_avoidance_unit_boundary<IGetNearestDecision, IGetMctsFrameDepth>::log_decision(
    const resolution_lineage* rl) {
    if (ultimate_ != get_nearest_decision_.get_nearest_decision(rl->parent->parent)) {
        assign_penultimate(ultimate_);
        assign_penultimate_mcts_frame_depth(ultimate_mcts_frame_depth_);
    }
    assign_ultimate(rl);
    assign_ultimate_mcts_frame_depth(get_mcts_frame_depth_.mcts_frame_depth());
}

template<typename IGetNearestDecision, typename IGetMctsFrameDepth>
size_t dbuct_avoidance_unit_boundary<IGetNearestDecision, IGetMctsFrameDepth>::get_penultimate_mcts_frame_depth() const {
    return penultimate_mcts_frame_depth_;
}

template<typename IGetNearestDecision, typename IGetMctsFrameDepth>
const resolution_lineage* dbuct_avoidance_unit_boundary<IGetNearestDecision, IGetMctsFrameDepth>::get_ultimate_decision() const {
    return ultimate_;
}

template<typename IGetNearestDecision, typename IGetMctsFrameDepth>
const resolution_lineage* dbuct_avoidance_unit_boundary<IGetNearestDecision, IGetMctsFrameDepth>::get_penultimate_decision() const {
    return penultimate_;
}

template<typename IGetNearestDecision, typename IGetMctsFrameDepth>
size_t dbuct_avoidance_unit_boundary<IGetNearestDecision, IGetMctsFrameDepth>::get_ultimate_mcts_frame_depth() const {
    return ultimate_mcts_frame_depth_;
}

template<typename IGetNearestDecision, typename IGetMctsFrameDepth>
void dbuct_avoidance_unit_boundary<IGetNearestDecision, IGetMctsFrameDepth>::push_frame() {
    frame_stack_.push(frame{});
}

template<typename IGetNearestDecision, typename IGetMctsFrameDepth>
void dbuct_avoidance_unit_boundary<IGetNearestDecision, IGetMctsFrameDepth>::pop_frame() {
    auto current = std::move(frame_stack_.top());
    frame_stack_.pop();
    for (auto it = current.actions_.rbegin(); it != current.actions_.rend(); ++it)
        undo_action(*it);
}

template<typename IGetNearestDecision, typename IGetMctsFrameDepth>
void dbuct_avoidance_unit_boundary<IGetNearestDecision, IGetMctsFrameDepth>::assign_ultimate(
    const resolution_lineage* rl) {
    avoidance_boundary_rl_assign action{avoidance_rl_slot::ultimate, ultimate_};
    ultimate_ = rl;
    log(std::move(action));
}

template<typename IGetNearestDecision, typename IGetMctsFrameDepth>
void dbuct_avoidance_unit_boundary<IGetNearestDecision, IGetMctsFrameDepth>::assign_penultimate(
    const resolution_lineage* rl) {
    avoidance_boundary_rl_assign action{avoidance_rl_slot::penultimate, penultimate_};
    penultimate_ = rl;
    log(std::move(action));
}

template<typename IGetNearestDecision, typename IGetMctsFrameDepth>
void dbuct_avoidance_unit_boundary<IGetNearestDecision, IGetMctsFrameDepth>::assign_ultimate_mcts_frame_depth(
    size_t value) {
    avoidance_boundary_frame_assign action{avoidance_frame_slot::ultimate_mcts_frame_depth, ultimate_mcts_frame_depth_};
    ultimate_mcts_frame_depth_ = value;
    log(std::move(action));
}

template<typename IGetNearestDecision, typename IGetMctsFrameDepth>
void dbuct_avoidance_unit_boundary<IGetNearestDecision, IGetMctsFrameDepth>::assign_penultimate_mcts_frame_depth(
    size_t value) {
    avoidance_boundary_frame_assign action{avoidance_frame_slot::penultimate_mcts_frame_depth, penultimate_mcts_frame_depth_};
    penultimate_mcts_frame_depth_ = value;
    log(std::move(action));
}

template<typename IGetNearestDecision, typename IGetMctsFrameDepth>
void dbuct_avoidance_unit_boundary<IGetNearestDecision, IGetMctsFrameDepth>::log(avoidance_boundary_action action) {
    DEBUG_ASSERT(!frame_stack_.empty());
    frame_stack_.top().actions_.push_back(std::move(action));
}

template<typename IGetNearestDecision, typename IGetMctsFrameDepth>
void dbuct_avoidance_unit_boundary<IGetNearestDecision, IGetMctsFrameDepth>::undo_action(
    const avoidance_boundary_action& action) {
    if (const auto* rl = std::get_if<avoidance_boundary_rl_assign>(&action)) {
        if (rl->slot == avoidance_rl_slot::ultimate)
            ultimate_ = rl->previous;
        else
            penultimate_ = rl->previous;
    } else {
        const auto& fr = std::get<avoidance_boundary_frame_assign>(action);
        if (fr.slot == avoidance_frame_slot::ultimate_mcts_frame_depth)
            ultimate_mcts_frame_depth_ = fr.previous;
        else
            penultimate_mcts_frame_depth_ = fr.previous;
    }
}

#endif
