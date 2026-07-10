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
    void squash_frame();

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

inline void dbuct_resolution_memory::record_resolution(const resolution_lineage* rl) {
    if (resolutions_.contains(rl))
        return;
    resolutions_.insert(rl);
    log(resolution_set_insert{rl});
}

inline size_t dbuct_resolution_memory::get_resolution_count() const { return resolutions_.size(); }

inline lemma dbuct_resolution_memory::derive_resolution_lemma() const { return lemma{resolutions_}; }

inline void dbuct_resolution_memory::push_frame() { frame_stack_.push(frame{}); }

inline void dbuct_resolution_memory::pop_frame() {
    auto current = std::move(frame_stack_.top());
    frame_stack_.pop();
    for (auto it = current.actions.rbegin(); it != current.actions.rend(); ++it)
        undo_action(*it);
}

inline void dbuct_resolution_memory::squash_frame() {
    auto top = std::move(frame_stack_.top());
    frame_stack_.pop();
    auto& parent = frame_stack_.top().actions;
    parent.splice(parent.end(), std::move(top.actions));
}

inline void dbuct_resolution_memory::log(decision_memory_action action) {
    DEBUG_ASSERT(!frame_stack_.empty());
    frame_stack_.top().actions.push_back(std::move(action));
}

inline void dbuct_resolution_memory::undo_action(const decision_memory_action& action) {
    const auto& ins = std::get<resolution_set_insert>(action);
    resolutions_.erase(ins.rl);
}

#endif
