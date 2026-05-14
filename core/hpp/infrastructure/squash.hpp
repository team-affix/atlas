#ifndef SQUASH_HPP
#define SQUASH_HPP

#include <unordered_map>
#include "../domain/interfaces/i_squash.hpp"

struct squash : i_squash {
    virtual ~squash() = default;
    squash();
    void bind(uint32_t, const expr*) override;
    const expr* whnf(const expr*) override;
private:
    std::unordered_map<uint32_t, const expr*> bindings;
};

#endif
