#ifndef BIND_MAP_HPP
#define BIND_MAP_HPP

#include <unordered_map>
#include "../domain/interfaces/i_bind_map.hpp"

struct bind_map : i_bind_map {
    virtual ~bind_map() = default;
    bind_map();
    void bind(uint32_t, const expr*) override;
    const expr* whnf(const expr*) override;
private:
    std::unordered_map<uint32_t, const expr*> bindings;
};

#endif
