#ifndef RULE_ID_SET_HPP
#define RULE_ID_SET_HPP

#include <unordered_set>
#include "../interfaces/i_rule_id_set.hpp"

struct rule_id_set : i_rule_id_set {
    void insert(rule_id) override;
    void erase(rule_id) override;
    state_machine<rule_id> iterate() const override;
    size_t size() const override;
private:
    std::unordered_set<rule_id> rule_ids_;
};

#endif
