#ifndef UNIFIER_HPP
#define UNIFIER_HPP

#include "../domain/interfaces/i_unifier.hpp"
#include <memory>

struct unifier : i_unifier {
    virtual ~unifier() = default;
    unifier(std::unique_ptr<i_bind_map>);
    bool unify(const expr*, const expr*, std::unordered_set<uint32_t>&) override;
private:
    bool occurs_check(uint32_t, const expr*);
};

#endif
