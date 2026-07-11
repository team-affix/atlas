#ifndef DBUCT_NEAREST_DECISION_HPP
#define DBUCT_NEAREST_DECISION_HPP

#include <list>
#include <stack>
#include <unordered_map>
#include "value_objects/lineage.hpp"
#include "value_objects/nearest_decision_action.hpp"
#include "debug_assert.hpp"

struct dbuct_nearest_decision {
    void note_unit_resolution(const resolution_lineage* rl);
    void note_decision_resolution(const resolution_lineage* rl);
    const resolution_lineage* get_nearest_decision(const resolution_lineage* rl) const;

    void push_frame();
    void pop_frame();

private:
    struct frame {
        std::list<nearest_decision_action> actions;
    };

    using map_t = std::unordered_map<const resolution_lineage*, const resolution_lineage*>;

    void log(nearest_decision_action action);
    void undo_action(const nearest_decision_action& action);

    map_t nd_{{nullptr, nullptr}};
    std::stack<frame> frame_stack_;
};

inline void dbuct_nearest_decision::note_unit_resolution(const resolution_lineage* rl) {
    nd_.insert({rl, nd_.at(rl->parent->parent)});
    log(nearest_decision_action{nearest_decision_insert{rl, nd_.at(rl)}});
}

inline void dbuct_nearest_decision::note_decision_resolution(const resolution_lineage* rl) {
    nd_.insert({rl, rl});
    log(nearest_decision_insert{rl, rl});
}

inline const resolution_lineage* dbuct_nearest_decision::get_nearest_decision(const resolution_lineage* rl) const {
    return nd_.at(rl);
}

inline void dbuct_nearest_decision::push_frame() { frame_stack_.push(frame{}); }

inline void dbuct_nearest_decision::pop_frame() {
    auto current = std::move(frame_stack_.top());
    frame_stack_.pop();
    for (auto it = current.actions.rbegin(); it != current.actions.rend(); ++it)
        undo_action(*it);
}

inline void dbuct_nearest_decision::log(nearest_decision_action action) {
    DEBUG_ASSERT(!frame_stack_.empty());
    frame_stack_.top().actions.push_back(std::move(action));
}

inline void dbuct_nearest_decision::undo_action(const nearest_decision_action& action) {
    const auto& ins = std::get<nearest_decision_insert>(action);
    nd_.erase(ins.key);
}

#endif
