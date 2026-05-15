#ifndef UNIFIER_HPP
#define UNIFIER_HPP

#include "../domain/interfaces/i_unifier.hpp"
#include "../domain/interfaces/i_bind_map.hpp"

struct unifier : i_unifier {
    virtual ~unifier() = default;
    unifier(i_bind_map&);
    bool unify(const expr*, const expr*, std::unordered_set<uint32_t>&) override;
private:
    bool occurs_check(uint32_t, const expr*);
    i_bind_map& bind_map;
};

#endif
