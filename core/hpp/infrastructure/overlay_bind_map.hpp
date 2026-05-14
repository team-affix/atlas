#ifndef OVERLAY_BIND_MAP_HPP
#define OVERLAY_BIND_MAP_HPP

#include "../domain/interfaces/i_overlay_bind_map.hpp"
#include <memory>

struct overlay_bind_map : i_overlay_bind_map {
    virtual ~overlay_bind_map() = default;
    overlay_bind_map(std::unique_ptr<i_bind_map> local, i_bind_map& remote);
    void bind(uint32_t, const expr*) override;
    const expr* whnf(const expr*) override;
private:
    std::unique_ptr<i_bind_map> local_;
    i_bind_map& remote_;
};

#endif
