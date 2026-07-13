#ifndef DBUCT_AVOIDANCE_UNIT_BOUNDARY_HPP
#define DBUCT_AVOIDANCE_UNIT_BOUNDARY_HPP

#include <cstddef>
#include <deque>
#include <list>
#include <stack>
#include "value_objects/avoidance_boundary_action.hpp"
#include "value_objects/lineage.hpp"
#include "debug_assert.hpp"

template<typename IGetNearestDecision, typename IGetChoiceDepth>
struct dbuct_avoidance_unit_boundary {
    dbuct_avoidance_unit_boundary(IGetNearestDecision& nd, IGetChoiceDepth& choice_depth);

    void log_decision(const resolution_lineage* rl);
    size_t get_penultimate_decision_choice_depth() const;
    const resolution_lineage* get_ultimate_decision() const;
    const resolution_lineage* get_penultimate_decision() const;
    size_t get_ultimate_decision_choice_depth() const;

    void push_frame();
    void pop_frame();

private:
    struct frame {
        std::list<avoidance_boundary_action> actions_;
    };

    void assign_ultimate(const resolution_lineage* rl);
    void assign_penultimate(const resolution_lineage* rl);
    void assign_ultimate_decision_choice_depth(size_t value);
    void assign_penultimate_decision_choice_depth(size_t value);
    void log(avoidance_boundary_action action);
    void undo_action(const avoidance_boundary_action& action);

    const resolution_lineage* ultimate_;
    const resolution_lineage* penultimate_;
    size_t ultimate_decision_choice_depth_;
    size_t penultimate_decision_choice_depth_;

    IGetNearestDecision& get_nearest_decision_;
    IGetChoiceDepth& get_choice_depth_;
    std::stack<frame> frame_stack_;
};

template<typename IGetNearestDecision, typename IGetChoiceDepth>
dbuct_avoidance_unit_boundary<IGetNearestDecision, IGetChoiceDepth>::dbuct_avoidance_unit_boundary(
    IGetNearestDecision& nd, IGetChoiceDepth& choice_depth)
    : ultimate_(nullptr), penultimate_(nullptr), ultimate_decision_choice_depth_(0),
      penultimate_decision_choice_depth_(0), get_nearest_decision_(nd), get_choice_depth_(choice_depth),
      frame_stack_(std::deque<frame>{frame{}}) {}

template<typename IGetNearestDecision, typename IGetChoiceDepth>
void dbuct_avoidance_unit_boundary<IGetNearestDecision, IGetChoiceDepth>::log_decision(
    const resolution_lineage* rl) {
    if (ultimate_ != get_nearest_decision_.get_nearest_decision(rl->parent->parent)) {
        assign_penultimate(ultimate_);
        assign_penultimate_decision_choice_depth(ultimate_decision_choice_depth_);
    }
    assign_ultimate(rl);
    assign_ultimate_decision_choice_depth(get_choice_depth_.depth());
}

template<typename IGetNearestDecision, typename IGetChoiceDepth>
size_t dbuct_avoidance_unit_boundary<IGetNearestDecision, IGetChoiceDepth>::get_penultimate_decision_choice_depth() const {
    return penultimate_decision_choice_depth_;
}

template<typename IGetNearestDecision, typename IGetChoiceDepth>
const resolution_lineage* dbuct_avoidance_unit_boundary<IGetNearestDecision, IGetChoiceDepth>::get_ultimate_decision() const {
    return ultimate_;
}

template<typename IGetNearestDecision, typename IGetChoiceDepth>
const resolution_lineage* dbuct_avoidance_unit_boundary<IGetNearestDecision, IGetChoiceDepth>::get_penultimate_decision() const {
    return penultimate_;
}

template<typename IGetNearestDecision, typename IGetChoiceDepth>
size_t dbuct_avoidance_unit_boundary<IGetNearestDecision, IGetChoiceDepth>::get_ultimate_decision_choice_depth() const {
    return ultimate_decision_choice_depth_;
}

template<typename IGetNearestDecision, typename IGetChoiceDepth>
void dbuct_avoidance_unit_boundary<IGetNearestDecision, IGetChoiceDepth>::push_frame() {
    frame_stack_.push(frame{});
}

template<typename IGetNearestDecision, typename IGetChoiceDepth>
void dbuct_avoidance_unit_boundary<IGetNearestDecision, IGetChoiceDepth>::pop_frame() {
    auto current = std::move(frame_stack_.top());
    frame_stack_.pop();
    for (auto it = current.actions_.rbegin(); it != current.actions_.rend(); ++it)
        undo_action(*it);
}

template<typename IGetNearestDecision, typename IGetChoiceDepth>
void dbuct_avoidance_unit_boundary<IGetNearestDecision, IGetChoiceDepth>::assign_ultimate(
    const resolution_lineage* rl) {
    avoidance_boundary_rl_assign action{avoidance_rl_slot::ultimate, ultimate_};
    ultimate_ = rl;
    log(std::move(action));
}

template<typename IGetNearestDecision, typename IGetChoiceDepth>
void dbuct_avoidance_unit_boundary<IGetNearestDecision, IGetChoiceDepth>::assign_penultimate(
    const resolution_lineage* rl) {
    avoidance_boundary_rl_assign action{avoidance_rl_slot::penultimate, penultimate_};
    penultimate_ = rl;
    log(std::move(action));
}

template<typename IGetNearestDecision, typename IGetChoiceDepth>
void dbuct_avoidance_unit_boundary<IGetNearestDecision, IGetChoiceDepth>::assign_ultimate_decision_choice_depth(
    size_t value) {
    avoidance_boundary_frame_assign action{avoidance_frame_slot::ultimate_frame_depth, ultimate_decision_choice_depth_};
    ultimate_decision_choice_depth_ = value;
    log(std::move(action));
}

template<typename IGetNearestDecision, typename IGetChoiceDepth>
void dbuct_avoidance_unit_boundary<IGetNearestDecision, IGetChoiceDepth>::assign_penultimate_decision_choice_depth(
    size_t value) {
    avoidance_boundary_frame_assign action{avoidance_frame_slot::unit_boundary_frame_depth, penultimate_decision_choice_depth_};
    penultimate_decision_choice_depth_ = value;
    log(std::move(action));
}

template<typename IGetNearestDecision, typename IGetChoiceDepth>
void dbuct_avoidance_unit_boundary<IGetNearestDecision, IGetChoiceDepth>::log(avoidance_boundary_action action) {
    DEBUG_ASSERT(!frame_stack_.empty());
    frame_stack_.top().actions_.push_back(std::move(action));
}

template<typename IGetNearestDecision, typename IGetChoiceDepth>
void dbuct_avoidance_unit_boundary<IGetNearestDecision, IGetChoiceDepth>::undo_action(
    const avoidance_boundary_action& action) {
    if (const auto* rl = std::get_if<avoidance_boundary_rl_assign>(&action)) {
        if (rl->slot == avoidance_rl_slot::ultimate)
            ultimate_ = rl->previous;
        else
            penultimate_ = rl->previous;
    } else {
        const auto& fr = std::get<avoidance_boundary_frame_assign>(action);
        if (fr.slot == avoidance_frame_slot::ultimate_frame_depth)
            ultimate_decision_choice_depth_ = fr.previous;
        else
            penultimate_decision_choice_depth_ = fr.previous;
    }
}

#endif
