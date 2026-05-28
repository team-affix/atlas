#include "infrastructure/bind_map_factory.hpp"
#include "infrastructure/bind_map.hpp"

std::unique_ptr<i_bind_map> bind_map_factory::make() const {
    return std::make_unique<bind_map>();
}
