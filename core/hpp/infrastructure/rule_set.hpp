#ifndef RULE_SET_HPP
#define RULE_SET_HPP

#include <unordered_set>
#include "../interfaces/i_rule_set.hpp"

struct rule_set : i_rule_set {
    void insert(const rule*) override;
    void erase(const rule*) override;
    state_machine<const rule*> iterate() const override;
    size_t size() const override;
private:
    std::unordered_set<const rule*> rules_;
};

#endif
