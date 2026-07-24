#ifndef DBUCT_GOAL_DEPTHS_HPP
#define DBUCT_GOAL_DEPTHS_HPP

#include <cstddef>
#include <deque>
#include <list>
#include <stack>
#include <unordered_map>
#include "value_objects/goal_depth_action.hpp"
#include "value_objects/lineage.hpp"
#include "debug_assert.hpp"

struct dbuct_goal_depths {
    dbuct_goal_depths();
    size_t get(const goal_lineage*) const;
    void set(const goal_lineage*, size_t);
    void erase(const goal_lineage*);

    void push_frame();
    void pop_frame();

private:
    struct frame {
        std::list<goal_depth_action> actions_;
    };

    void log(goal_depth_action action);
    void undo_action(const goal_depth_action& action);

    std::unordered_map<const goal_lineage*, size_t> depths_;
    std::stack<frame> frame_stack_;
};

#endif
