#ifndef UNIFIER_HPP
#define UNIFIER_HPP

#include "../domain/interfaces/i_unifier.hpp"
#include "../domain/interfaces/i_squash.hpp"
#include <memory>

struct unifier : i_unifier {
    virtual ~unifier() = default;
    unifier();
    bool unify(const expr*, const expr*, i_queue<uint32_t>&) override;
    const expr* whnf(const expr*) override;
private:
    bool occurs_check(uint32_t, const expr*);

    std::unique_ptr<i_squash> local_;
    i_squash& common_;
};

#endif
