#ifndef PUD_BIND_MAP_FACTORY_HPP
#define PUD_BIND_MAP_FACTORY_HPP

#include "infrastructure/ubs_bind_map.hpp"
#include "value_objects/pud_ubs_state.hpp"

struct pud_bind_map_factory {
    ubs_bind_map make_bind_map(const pud_ubs_state& state) const;
};

#endif
