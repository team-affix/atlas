#include "../../hpp/infrastructure/unifier_factory.hpp"
#include "../../hpp/infrastructure/unifier.hpp"

std::unique_ptr<i_unifier> unifier_factory::make(i_bind_map& bind_map) const {
    return std::make_unique<unifier>(bind_map);
}
