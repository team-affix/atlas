#include "../../hpp/infrastructure/bind_map_factory.hpp"
#include "../../hpp/infrastructure/bind_map.hpp"

std::unique_ptr<i_bind_map> bind_map_factory::make(const resolution_lineage* rl) {
    return std::make_unique<bind_map>(rl);
}
