#ifndef ELIMINATION_BACKLOG_HPP
#define ELIMINATION_BACKLOG_HPP

#include <list>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include "value_objects/elimination_backlog_action.hpp"
#include "value_objects/lineage.hpp"
#include "value_objects/rule.hpp"
#include "debug_assert.hpp"

struct elimination_backlog {
    void insert_backlogged_elimination(const resolution_lineage*);
    bool is_backlogged_elimination(const resolution_lineage* rl) const;

    void push_frame();
    void pop_frame();
    size_t depth() const;

private:
    struct frame {
        std::list<elimination_backlog_action> actions_;
    };

    using eliminated_candidates_type =
        std::unordered_map<const goal_lineage*, std::unordered_set<rule_id>>;

    void log(elimination_backlog_action action);
    void undo_action(const elimination_backlog_action& action);

    eliminated_candidates_type eliminated_candidates_;
    std::stack<frame> frame_stack_;
};

#endif
