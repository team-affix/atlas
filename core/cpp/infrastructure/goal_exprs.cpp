#include "../../hpp/infrastructure/goal_exprs.hpp"

const expr* goal_exprs::get(const goal_lineage* gl) const {
    auto it = exprs_.find(gl);
    if (it == exprs_.end())
        return nullptr;
    return it->second;
}

void goal_exprs::set(const goal_lineage* gl, const expr* e) {
    exprs_[gl] = e;
}

void goal_exprs::unset(const goal_lineage* gl) {
    exprs_.erase(gl);
}
