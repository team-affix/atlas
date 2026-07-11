#ifndef DBUCT_DECISION_MEMORY_HPP
#define DBUCT_DECISION_MEMORY_HPP

#include <list>
#include <stack>
#include <unordered_set>
#include "value_objects/decision_memory_action.hpp"
#include "value_objects/lemma.hpp"
#include "value_objects/lineage.hpp"
#include "debug_assert.hpp"

struct dbuct_decision_memory {
    void record_decision(const resolution_lineage* rl);
    size_t count() const;
    lemma derive_decision_lemma() const;

    void push_frame();
    void pop_frame();

private:
    struct frame {
        std::list<decision_memory_action> actions;
    };

    using set_t = std::unordered_set<const resolution_lineage*>;

    void log(decision_memory_action action);
    void undo_action(const decision_memory_action& action);

    set_t decisions_;
    std::stack<frame> frame_stack_;
};

inline void dbuct_decision_memory::record_decision(const resolution_lineage* rl) {
    if (decisions_.contains(rl))
        return;
    decisions_.insert(rl);
    log(resolution_set_insert{rl});
}

inline size_t dbuct_decision_memory::count() const { return decisions_.size(); }

inline lemma dbuct_decision_memory::derive_decision_lemma() const { return lemma{decisions_}; }

inline void dbuct_decision_memory::push_frame() { frame_stack_.push(frame{}); }

inline void dbuct_decision_memory::pop_frame() {
    auto current = std::move(frame_stack_.top());
    frame_stack_.pop();
    for (auto it = current.actions.rbegin(); it != current.actions.rend(); ++it)
        undo_action(*it);
}

inline void dbuct_decision_memory::log(decision_memory_action action) {
    DEBUG_ASSERT(!frame_stack_.empty());
    frame_stack_.top().actions.push_back(std::move(action));
}

inline void dbuct_decision_memory::undo_action(const decision_memory_action& action) {
    const auto& ins = std::get<resolution_set_insert>(action);
    decisions_.erase(ins.rl);
}

#endif
