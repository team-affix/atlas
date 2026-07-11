#ifndef DBUCT_RESOLUTION_MEMORY_HPP
#define DBUCT_RESOLUTION_MEMORY_HPP

#include <list>
#include <stack>
#include <unordered_set>
#include "value_objects/decision_memory_action.hpp"
#include "value_objects/lemma.hpp"
#include "value_objects/lineage.hpp"
#include "debug_assert.hpp"

struct dbuct_resolution_memory {
    void record_resolution(const resolution_lineage* rl);
    size_t get_resolution_count() const;
    lemma derive_resolution_lemma() const;

    void push_frame();
    void pop_frame();

private:
    struct frame {
        std::list<decision_memory_action> actions;
    };

    using set_t = std::unordered_set<const resolution_lineage*>;

    void log(decision_memory_action action);
    void undo_action(const decision_memory_action& action);

    set_t resolutions_;
    std::stack<frame> frame_stack_;
};

#endif
