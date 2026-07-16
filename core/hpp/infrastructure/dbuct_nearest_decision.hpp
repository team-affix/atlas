#ifndef DBUCT_NEAREST_DECISION_HPP
#define DBUCT_NEAREST_DECISION_HPP

#include <deque>
#include <list>
#include <stack>
#include <unordered_map>
#include "value_objects/lineage.hpp"
#include "value_objects/nearest_decision_action.hpp"
#include "debug_assert.hpp"

struct dbuct_nearest_decision {
    dbuct_nearest_decision();
    void note_unit_resolution(const resolution_lineage* rl);
    void note_decision_resolution(const resolution_lineage* rl);
    const resolution_lineage* get_nearest_decision(const resolution_lineage* rl) const;

    void push_frame();
    void pop_frame();

private:
    struct frame {
        std::list<nearest_decision_action> actions_;
    };

    using map_t = std::unordered_map<const resolution_lineage*, const resolution_lineage*>;

    void log(nearest_decision_action action);
    void undo_action(const nearest_decision_action& action);

    map_t nd_;
    std::stack<frame> frame_stack_;
};

#endif
