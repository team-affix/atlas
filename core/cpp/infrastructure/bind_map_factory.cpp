#include "infrastructure/bind_map_factory.hpp"

bind_map_factory::bind_map_factory(globalizer& g) : globalizer_(g) {}

bind_map bind_map_factory::make() const {
    return bind_map{globalizer_};
}
