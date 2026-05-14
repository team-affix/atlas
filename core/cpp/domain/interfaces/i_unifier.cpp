#include "../../../hpp/domain/interfaces/i_unifier.hpp"

i_unifier::i_unifier(std::unique_ptr<i_bind_map> bm)
    : bind_map(std::move(bm)) {}
