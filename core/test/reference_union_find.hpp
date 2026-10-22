#ifndef REFERENCE_UNION_FIND_HPP
#define REFERENCE_UNION_FIND_HPP

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Tracks variable classes for bind-map invariant checks (not a production type).
struct reference_variable_classes {
    std::unordered_map<uint32_t, std::unordered_set<uint32_t>> by_min_index;

    std::unordered_set<uint32_t>& class_for(uint32_t var) {
        for (auto& entry : by_min_index) {
            if (entry.second.count(var))
                return entry.second;
        }
        by_min_index[var] = {var};
        return by_min_index.at(var);
    }

    void equate(uint32_t var_a, uint32_t var_b) {
        std::unordered_set<uint32_t>& class_a = class_for(var_a);
        std::unordered_set<uint32_t>& class_b = class_for(var_b);
        if (&class_a == &class_b)
            return;
        const uint32_t min_a = *std::min_element(class_a.begin(), class_a.end());
        const uint32_t min_b = *std::min_element(class_b.begin(), class_b.end());
        const uint32_t survivor_min = std::min(min_a, min_b);
        std::unordered_set<uint32_t> merged = class_a;
        merged.insert(class_b.begin(), class_b.end());
        by_min_index.erase(min_a);
        by_min_index.erase(min_b);
        by_min_index[survivor_min] = std::move(merged);
    }

    uint32_t min_index(uint32_t var) const {
        for (const auto& entry : by_min_index) {
            if (entry.second.count(var))
                return entry.first;
        }
        return var;
    }

    size_t class_size(uint32_t var) const {
        for (const auto& entry : by_min_index) {
            if (entry.second.count(var))
                return entry.second.size();
        }
        return 1u;
    }

    bool same_class(uint32_t left, uint32_t right) const {
        return min_index(left) == min_index(right);
    }

    void touch(uint32_t var) {
        class_for(var);
    }
};

inline uint32_t reference_parent_chain_depth(
    const std::unordered_map<uint32_t, uint32_t>& parents,
    uint32_t var) {
    uint32_t depth = 0;
    while (parents.find(var) != parents.end()) {
        var = parents.at(var);
        ++depth;
    }
    return depth;
}

inline bool reference_parent_points_inside_class(
    const std::unordered_map<uint32_t, uint32_t>& parents,
    const reference_variable_classes& classes) {
    for (const auto& edge : parents) {
        const uint32_t child = edge.first;
        const uint32_t parent = edge.second;
        if (!classes.same_class(child, parent))
            return false;
    }
    return true;
}

inline bool reference_height_within_log_bound(
    const std::unordered_map<uint32_t, uint32_t>& parents,
    const reference_variable_classes& classes,
    uint32_t var) {
    const size_t class_size = classes.class_size(var);
    if (class_size <= 1u)
        return true;
    const uint32_t depth = reference_parent_chain_depth(parents, var);
    const double log_bound = std::floor(std::log2(static_cast<double>(class_size)));
    return depth <= static_cast<uint32_t>(log_bound);
}

#endif
