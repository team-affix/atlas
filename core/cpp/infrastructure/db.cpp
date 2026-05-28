#include "../../hpp/infrastructure/db.hpp"

db::db() = default;

rule_id db::push(rule r) {
    rule_id id = rules_.size();
    rules_.push_back(std::move(r));
    total_rule_set_.insert(id);
    return id;
}

const rule* db::get(rule_id id) const {
    return &rules_.at(id);
}

i_rule_id_set& db::get(const goal_lineage* /*gl*/) {
    return total_rule_set_;
}
