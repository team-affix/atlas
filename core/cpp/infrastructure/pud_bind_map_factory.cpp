#include "infrastructure/pud_bind_map_factory.hpp"

ubs_bind_map pud_bind_map_factory::make_bind_map(const pud_ubs_state& state) const {
    return ubs_bind_map{state};
}
