#ifndef DBUCT_CANDIDATE_FRAME_OFFSETS_HPP
#define DBUCT_CANDIDATE_FRAME_OFFSETS_HPP

#include <cstdint>
#include <deque>
#include <list>
#include <stack>
#include <unordered_map>
#include "value_objects/candidate_offset_action.hpp"
#include "value_objects/lineage.hpp"
#include "debug_assert.hpp"

struct dbuct_candidate_frame_offsets {
    dbuct_candidate_frame_offsets();
    void set(const resolution_lineage* rl, uint32_t frame_offset);
    void unset(const resolution_lineage* rl);
    uint32_t get(const resolution_lineage* rl) const;

    void push_frame();
    void pop_frame();

private:
    struct frame {
        std::list<candidate_offset_action> actions_;
    };

    using map_t = std::unordered_map<const resolution_lineage*, uint32_t>;

    void log(candidate_offset_action action);
    void undo_action(const candidate_offset_action& action);

    map_t offsets_;
    std::stack<frame> frame_stack_;
};

#endif
