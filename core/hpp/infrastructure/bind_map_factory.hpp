#ifndef BIND_MAP_FACTORY_HPP
#define BIND_MAP_FACTORY_HPP

#include <memory>
#include "../domain/interfaces/i_factory.hpp"
#include "../domain/interfaces/i_bind_map.hpp"
#include "../domain/value_objects/lineage.hpp"

struct bind_map_factory : i_factory<i_bind_map, const resolution_lineage*> {
    std::unique_ptr<i_bind_map> make(const resolution_lineage*) override;
};

#endif
