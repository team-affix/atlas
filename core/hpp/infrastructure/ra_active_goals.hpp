#ifndef RA_ACTIVE_GOALS_HPP
#define RA_ACTIVE_GOALS_HPP

#include <unordered_map>
#include <vector>
#include "value_objects/lineage.hpp"
#include "debug_assert.hpp"

struct ra_active_goals {
    void insert_active_goal(const goal_lineage*);
    void erase_active_goal(const goal_lineage*);
    bool is_active_goal(const goal_lineage*) const;
    size_t active_goals_size() const;
    bool empty() const;
    void clear_active_goals();
    const goal_lineage* select(size_t index) const;
private:
    std::unordered_map<const goal_lineage*, size_t> index_;
    std::vector<const goal_lineage*> items_;
};

inline void ra_active_goals::insert_active_goal(const goal_lineage* gl) {
    auto [_, inserted] = index_.emplace(gl, items_.size());
    DEBUG_ASSERT(inserted);
    items_.push_back(gl);
}

inline void ra_active_goals::erase_active_goal(const goal_lineage* gl) {
    auto it = index_.find(gl);
    DEBUG_ASSERT(it != index_.end());
    size_t idx = it->second;
    const goal_lineage* back = items_.back();
    items_.at(idx) = back;
    index_.at(back) = idx;
    items_.pop_back();
    index_.erase(it);
}

inline bool ra_active_goals::is_active_goal(const goal_lineage* gl) const {
    return index_.contains(gl);
}

inline size_t ra_active_goals::active_goals_size() const {
    return items_.size();
}

inline bool ra_active_goals::empty() const {
    return items_.empty();
}

inline void ra_active_goals::clear_active_goals() {
    index_.clear();
    items_.clear();
}

inline const goal_lineage* ra_active_goals::select(size_t index) const {
    return items_.at(index);
}

#endif
