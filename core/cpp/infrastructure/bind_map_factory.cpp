#include "infrastructure/bind_map_factory.hpp"
#include "infrastructure/bind_map.hpp"

bind_map_factory::bind_map_factory(i_globalizer& g) : globalizer_(g) {}

std::unique_ptr<i_bind_map> bind_map_factory::make() const {
    return std::make_unique<bind_map>(globalizer_);
}
