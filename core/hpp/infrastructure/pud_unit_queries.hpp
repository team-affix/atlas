#ifndef PUD_UNIT_QUERIES_HPP
#define PUD_UNIT_QUERIES_HPP

#include <deque>
#include "value_objects/pud_mhq_head_id.hpp"

// The query heads that became unit, oldest first, so every entry is eventually served. Each forces an
// unfold of every leaf that still holds that exact head; an entry may be stale by the time it is read.
struct pud_unit_queries {
    pud_unit_queries();
    bool empty() const;
    void push(pud_mhq_head_id head_id);
    pud_mhq_head_id front() const;
    void pop();
private:
    std::deque<pud_mhq_head_id> head_ids_;
};

#endif
