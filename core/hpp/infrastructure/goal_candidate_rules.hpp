#ifndef GOAL_CANDIDATE_RULES_HPP
#define GOAL_CANDIDATE_RULES_HPP

#include <unordered_map>
#include "infrastructure/ra_rule_id_set_factory.hpp"
#include "infrastructure/ra_rule_id_set.hpp"
#include "value_objects/lineage.hpp"
#include "value_objects/rule.hpp"
#include "debug_assert.hpp"

struct goal_candidate_rules {
    goal_candidate_rules(ra_rule_id_set_factory&);
    ra_rule_id_set& get(const goal_lineage*);
    const ra_rule_id_set& get(const goal_lineage*) const;
    void insert(const goal_lineage*);
    void link_goal_candidate(const goal_lineage*, rule_id);
    void unlink_goal_candidate(const goal_lineage*, rule_id);
    void erase(const goal_lineage*);
    void clear_goal_candidate_rule_ids();
private:
    using map_t = std::unordered_map<const goal_lineage*, ra_rule_id_set>;

    ra_rule_id_set_factory& factory_;
    map_t by_goal_;
};

#endif
