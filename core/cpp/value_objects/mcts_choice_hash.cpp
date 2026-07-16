#include "value_objects/mcts_choice_hash.hpp"

#include <functional>
#include <variant>

size_t mcts_choice_hash::operator()(const mcts_choice& choice) const noexcept {
    if (const goal_lineage* const* goal =
            std::get_if<const goal_lineage*>(&choice)) {
        size_t seed = std::hash<const goal_lineage*>{}(*goal);
        return hash_combine(seed, 1);
    }
    size_t seed = std::hash<rule_id>{}(std::get<rule_id>(choice));
    return hash_combine(seed, 2);
}

size_t mcts_choice_hash::hash_combine(size_t seed, size_t value) noexcept {
    return seed ^ (value + 0x9e3779b9 + (seed << 6) + (seed >> 2));
}
