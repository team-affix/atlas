#include "infrastructure/overlay_bind_map_factory.hpp"
#include "infrastructure/overlay_bind_map.hpp"

std::unique_ptr<i_bind_map> overlay_bind_map_factory::make(
    i_bind_map& local,
    i_bind_map& remote) const {
    return std::make_unique<overlay_bind_map>(local, remote);
}
