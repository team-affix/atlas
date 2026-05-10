#ifndef UNIFIER_FACTORY_HPP
#define UNIFIER_FACTORY_HPP

#include <memory>
#include "../domain/interfaces/i_factory.hpp"
#include "../domain/interfaces/i_unifier.hpp"
#include "../domain/value_objects/lineage.hpp"

struct unifier_factory : i_factory<i_unifier, const resolution_lineage*> {
    std::unique_ptr<i_unifier> make(const resolution_lineage*) override;
};

#endif
