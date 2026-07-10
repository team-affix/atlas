#ifndef DBUCT_BIND_MAP_HPP
#define DBUCT_BIND_MAP_HPP

#include <functional>
#include <list>
#include <stack>
#include <unordered_map>
#include "value_objects/bind_map_action.hpp"
#include "value_objects/framed_expr.hpp"
#include "debug_assert.hpp"

template<typename IGlobalize>
struct dbuct_bind_map {
    dbuct_bind_map(IGlobalize& g);

    void bind(uint32_t global_key, framed_expr value);
    framed_expr whnf(framed_expr fe);

    void set_journal(std::function<void(bind_map_action)> journal);

    void undo_logged_action(const bind_map_action& action);

    void push_frame();
    void pop_frame();
    void squash_frame();

private:
    using map_t = std::unordered_map<uint32_t, framed_expr>;

    struct frame {
        std::list<bind_map_action> actions;
    };

    void log(bind_map_action action);
    void undo_action(const bind_map_action& action);

    IGlobalize& globalizer_;
    map_t bindings_;
    std::function<void(bind_map_action)> journal_;
    std::stack<frame> frame_stack_;
};

template<typename IGlobalize>
dbuct_bind_map<IGlobalize>::dbuct_bind_map(IGlobalize& g) : globalizer_(g) {}

template<typename IGlobalize>
void dbuct_bind_map<IGlobalize>::set_journal(std::function<void(bind_map_action)> journal) {
    journal_ = std::move(journal);
}

template<typename IGlobalize>
void dbuct_bind_map<IGlobalize>::bind(uint32_t global_key, framed_expr value) {
    DEBUG_ASSERT(
        !std::holds_alternative<expr::var>(value.skeleton->content)
        || global_key > globalizer_.globalize(
               value.frame_offset,
               std::get<expr::var>(value.skeleton->content).index));
    bindings_.insert({global_key, std::move(value)});
    log(bind_map_insert{global_key, bindings_.at(global_key)});
}

template<typename IGlobalize>
framed_expr dbuct_bind_map<IGlobalize>::whnf(framed_expr fe) {
    if (!std::holds_alternative<expr::var>(fe.skeleton->content))
        return fe;
    const uint32_t global_key = globalizer_.globalize(
        fe.frame_offset, std::get<expr::var>(fe.skeleton->content).index);
    auto it = bindings_.find(global_key);
    if (it == bindings_.end())
        return fe;
    const framed_expr resolved = whnf(it->second);
    if (!(it->second == resolved)) {
        framed_expr previous = std::move(it->second);
        it->second = resolved;
        log(bind_map_assign{global_key, std::move(previous)});
    }
    return resolved;
}

template<typename IGlobalize>
void dbuct_bind_map<IGlobalize>::undo_logged_action(const bind_map_action& action) {
    undo_action(action);
}

template<typename IGlobalize>
void dbuct_bind_map<IGlobalize>::push_frame() {
    frame_stack_.push(frame{});
}

template<typename IGlobalize>
void dbuct_bind_map<IGlobalize>::pop_frame() {
    auto current = std::move(frame_stack_.top());
    frame_stack_.pop();
    for (auto it = current.actions.rbegin(); it != current.actions.rend(); ++it)
        undo_action(*it);
}

template<typename IGlobalize>
void dbuct_bind_map<IGlobalize>::squash_frame() {
    if (journal_)
        return;
    auto top = std::move(frame_stack_.top());
    frame_stack_.pop();
    auto& parent = frame_stack_.top().actions;
    parent.splice(parent.end(), std::move(top.actions));
}

template<typename IGlobalize>
void dbuct_bind_map<IGlobalize>::log(bind_map_action action) {
    if (journal_) {
        journal_(std::move(action));
        return;
    }
    DEBUG_ASSERT(!frame_stack_.empty());
    frame_stack_.top().actions.push_back(std::move(action));
}

template<typename IGlobalize>
void dbuct_bind_map<IGlobalize>::undo_action(const bind_map_action& action) {
    if (const auto* ins = std::get_if<bind_map_insert>(&action))
        bindings_.erase(ins->key);
    else {
        const auto& asg = std::get<bind_map_assign>(action);
        auto [it, inserted] = bindings_.insert({asg.key, asg.value});
        if (!inserted)
            it->second = asg.value;
    }
}

#endif
