#ifndef UNIFIER_FACTORY_HPP
#define UNIFIER_FACTORY_HPP

#include "infrastructure/locator.hpp"
#include "interfaces/i_unifier_factory.hpp"

struct unifier_factory : i_unifier_factory {
    explicit unifier_factory(locator&);
    std::unique_ptr<i_unifier> make(i_bind_map& bind_map) const override;
private:
    locator& loc_;
};

#endif
