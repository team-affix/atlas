#include "infrastructure/var_counter.hpp"

uint32_t var_counter::count(const expr* e) {
    if (std::holds_alternative<expr::var>(e->content)) {
        return 1;
    }

    const expr::functor& functor = std::get<expr::functor>(e->content);

    uint32_t result = 0;

    for (const expr* arg : functor.args) {
        result += count(arg);
    }

    return result;
}
