#include <gtest/gtest.h>
#include <algorithm>
#include "../../../core/hpp/infrastructure/conflict_detector.hpp"
#include "../../../core/hpp/interfaces/i_rule_set.hpp"

namespace {

class fake_rule_set : public i_rule_set {
public:
    void insert(const rule* r) override { rules.push_back(r); }
    void erase(const rule* r) override {
        rules.erase(std::remove(rules.begin(), rules.end(), r), rules.end());
    }
    void accept(i_visitor<const rule*>& v) const override {
        for (const rule* r : rules)
            v.visit(r);
    }
    size_t size() const override { return rules.size(); }

    std::vector<const rule*> rules;
};

class fake_get_goal_candidate_rules : public i_get_goal_candidate_rules {
public:
    explicit fake_get_goal_candidate_rules(fake_rule_set& rs) : rs(rs) {}
    i_rule_set& get(const goal_lineage*) override { return rs; }

    fake_rule_set& rs;
};

} // namespace

class ConflictDetectorTest : public ::testing::Test {
protected:
    goal_lineage gl{nullptr, nullptr};
    expr head{expr::var{0}};
    rule r{&head, {}};
    fake_rule_set rules;
    fake_get_goal_candidate_rules ggcr{rules};
    conflict_detector detector{ggcr};
};

TEST_F(ConflictDetectorTest, NoCandidatesIsConflict) {
    EXPECT_TRUE(detector.detect(&gl));
}

TEST_F(ConflictDetectorTest, OneCandidateIsNotConflict) {
    rules.insert(&r);
    EXPECT_FALSE(detector.detect(&gl));
}

TEST_F(ConflictDetectorTest, ManyCandidatesIsNotConflict) {
    expr head2{expr::var{1}};
    rule r2{&head2, {}};
    rules.insert(&r);
    rules.insert(&r2);
    EXPECT_FALSE(detector.detect(&gl));
}
