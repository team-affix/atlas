#ifndef UNIFIER_FACTORY_HPP
#define UNIFIER_FACTORY_HPP

#include "../interfaces/i_unifier_factory.hpp"

struct unifier_factory : i_unifier_factory {
    std::unique_ptr<i_unifier> make(i_bind_map& bind_map) const override;
};

#endif
