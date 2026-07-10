#ifndef DBUCT_CANDIDATE_FRAME_OFFSETS_HPP
#define DBUCT_CANDIDATE_FRAME_OFFSETS_HPP

#include <cstdint>
#include <list>
#include <stack>
#include <unordered_map>
#include "value_objects/candidate_offset_action.hpp"
#include "value_objects/lineage.hpp"
#include "debug_assert.hpp"

struct dbuct_candidate_frame_offsets {
    void set(const resolution_lineage* rl, uint32_t frame_offset);
    void unset(const resolution_lineage* rl);
    uint32_t get(const resolution_lineage* rl) const;

    void push_frame();
    void pop_frame();
    void squash_frame();

private:
    struct frame {
        std::list<candidate_offset_action> actions;
    };

    using map_t = std::unordered_map<const resolution_lineage*, uint32_t>;

    void log(candidate_offset_action action);
    void undo_action(const candidate_offset_action& action);

    map_t offsets_;
    std::stack<frame> frame_stack_;
};

inline void dbuct_candidate_frame_offsets::set(const resolution_lineage* rl, uint32_t frame_offset) {
    offsets_.insert({rl, frame_offset});
    log(candidate_offset_insert{rl, frame_offset});
}

inline void dbuct_candidate_frame_offsets::unset(const resolution_lineage* rl) {
    uint32_t captured = offsets_.at(rl);
    offsets_.erase(rl);
    log(candidate_offset_erase{rl, captured});
}

inline uint32_t dbuct_candidate_frame_offsets::get(const resolution_lineage* rl) const {
    return offsets_.at(rl);
}

inline void dbuct_candidate_frame_offsets::push_frame() { frame_stack_.push(frame{}); }

inline void dbuct_candidate_frame_offsets::pop_frame() {
    auto current = std::move(frame_stack_.top());
    frame_stack_.pop();
    for (auto it = current.actions.rbegin(); it != current.actions.rend(); ++it)
        undo_action(*it);
}

inline void dbuct_candidate_frame_offsets::squash_frame() {
    auto top = std::move(frame_stack_.top());
    frame_stack_.pop();
    auto& parent = frame_stack_.top().actions;
    parent.splice(parent.end(), std::move(top.actions));
}

inline void dbuct_candidate_frame_offsets::log(candidate_offset_action action) {
    DEBUG_ASSERT(!frame_stack_.empty());
    frame_stack_.top().actions.push_back(std::move(action));
}

inline void dbuct_candidate_frame_offsets::undo_action(const candidate_offset_action& action) {
    if (const auto* ins = std::get_if<candidate_offset_insert>(&action))
        offsets_.erase(ins->rl);
    else {
        const auto& er = std::get<candidate_offset_erase>(action);
        offsets_.insert({er.rl, er.frame_offset});
    }
}

#endif
