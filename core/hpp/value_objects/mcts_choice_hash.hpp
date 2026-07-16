#ifndef MCTS_CHOICE_HASH_HPP
#define MCTS_CHOICE_HASH_HPP

#include <cstddef>
#include "value_objects/mcts_choice.hpp"

struct mcts_choice_hash {
    size_t operator()(const mcts_choice& choice) const noexcept;

private:
    static size_t hash_combine(size_t seed, size_t value) noexcept;
};

#endif
