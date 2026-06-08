#ifndef RA_RULE_ID_SET_HPP
#define RA_RULE_ID_SET_HPP

#include <unordered_map>
#include <vector>
#include "interfaces/i_ra_rule_id_set.hpp"

struct ra_rule_id_set : i_ra_rule_id_set {
    void insert(rule_id) override;
    void erase(rule_id) override;
    bool contains(rule_id) const override;
    coroutine<rule_id, void> iterate() const override;
    rule_id front() const override;
    size_t size() const override;
    std::unique_ptr<i_rule_id_set> copy() const override;
    rule_id select(size_t index) const override;
private:
    std::unordered_map<rule_id, size_t> index_;
    std::vector<rule_id> items_;
};

#endif
