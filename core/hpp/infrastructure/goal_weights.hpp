#ifndef GOAL_WEIGHTS_HPP
#define GOAL_WEIGHTS_HPP

#include <unordered_map>
#include "value_objects/lineage.hpp"
#include "debug_assert.hpp"

struct goal_weights {
    double get(const goal_lineage*) const;
    void set(const goal_lineage*, double);
    void erase(const goal_lineage*);
    void clear_goal_weights();
private:
    std::unordered_map<const goal_lineage*, double> weights_;
};

inline double goal_weights::get(const goal_lineage* gl) const {
    return weights_.at(gl);
}

inline void goal_weights::set(const goal_lineage* gl, double w) {
    auto [_, inserted] = weights_.insert({gl, w});
    DEBUG_ASSERT(inserted);
}

inline void goal_weights::erase(const goal_lineage* gl) {
    auto erased = weights_.erase(gl);
    DEBUG_ASSERT(erased == 1);
}

inline void goal_weights::clear_goal_weights() {
    weights_.clear();
}

#endif
