#include "../../hpp/infrastructure/overlay_bind_map.hpp"

overlay_bind_map::overlay_bind_map(i_bind_map& local, i_bind_map& remote) :
    local_(local),
    remote_(remote) {
}

void overlay_bind_map::bind(uint32_t index, const expr* e) {
    local_.bind(index, e);
}

const expr* overlay_bind_map::whnf(const expr* e) {
    const expr* result = local_.whnf(e);
    if (result != e)
        return result;
    return remote_.whnf(e);
}
