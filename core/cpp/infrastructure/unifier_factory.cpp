#include "infrastructure/unifier.hpp"
#include "infrastructure/unifier_factory.hpp"

unifier_factory::unifier_factory(locator& loc) : loc_(loc) {}

std::unique_ptr<i_unifier> unifier_factory::make(i_bind_map& bm) const {
    return std::make_unique<unifier>(loc_, bm);
}
