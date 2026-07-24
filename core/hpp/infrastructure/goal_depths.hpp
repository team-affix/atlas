#ifndef GOAL_DEPTHS_HPP
#define GOAL_DEPTHS_HPP

#include <cstddef>
#include <unordered_map>
#include "value_objects/lineage.hpp"
#include "debug_assert.hpp"

struct goal_depths {
    size_t get(const goal_lineage*) const;
    void set(const goal_lineage*, size_t);
    void erase(const goal_lineage*);
    void clear_goal_depths();
private:
    std::unordered_map<const goal_lineage*, size_t> depths_;
};

#endif
