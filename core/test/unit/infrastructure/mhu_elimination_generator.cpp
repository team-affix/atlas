// mhu_elimination_generator unit contracts: mock-only mirrors of integration
// try_add_head / clear_mhu_heads / duplicate-head behavior. Goal is fast regression
// with mock discipline, not new scenarios beyond integration coverage.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "locator_fixture.hpp"
#include "infrastructure/mhu_elimination_generator.hpp"
#include "infrastructure/bind_map.hpp"
#include "infrastructure/unifier_factory.hpp"
#include "interfaces/i_bind_map.hpp"
#include "interfaces/i_bind_map_factory.hpp"
#include "interfaces/i_unifier_factory.hpp"
#include "interfaces/i_make_resolution_lineage.hpp"
#include "interfaces/i_make_var.hpp"
#include "interfaces/i_get_goal_candidate_rule_ids.hpp"
#include "infrastructure/rule_id_set.hpp"

using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct failing_unifier : i_unifier {
    coroutine<uint32_t, bool> unify(const expr*, const expr*) override {
        co_return false;
    }
};

struct failing_unifier_factory : i_unifier_factory {
    std::unique_ptr<i_unifier> make(i_bind_map&) const override {
        return std::make_unique<failing_unifier>();
    }
};

struct real_unifier_factory : i_unifier_factory {
    mutable unifier_factory inner;
    std::unique_ptr<i_unifier> make(i_bind_map& bm) const override {
        return inner.make(bm);
    }
};

struct test_bind_map_factory : i_bind_map_factory {
    std::unique_ptr<i_bind_map> make() const override {
        return std::make_unique<bind_map>();
    }
};

struct toggling_unifier_factory : i_unifier_factory {
    mutable int calls = 0;
    real_unifier_factory success_factory;
    std::unique_ptr<i_unifier> make(i_bind_map& bm) const override {
        if (calls++ == 0)
            return std::make_unique<failing_unifier>();
        return success_factory.make(bm);
    }
};

struct test_make_var : i_make_var {
    std::vector<expr> vars;
    const expr* make(uint32_t idx) override {
        while (vars.size() <= idx)
            vars.emplace_back(expr::var{static_cast<uint32_t>(vars.size())});
        return &vars[idx];
    }
};

struct identity_bind_map : i_bind_map {
    void bind(uint32_t, const expr*) override {}
    const expr* whnf(const expr* e) override { return e; }
};

} // namespace

struct MockMakeResolutionLineage : public i_make_resolution_lineage {
    MOCK_METHOD((const resolution_lineage*), make_resolution_lineage,
        (const goal_lineage*, rule_id), (override));
};

struct MockGetGoalCandidateRuleIds : public i_get_goal_candidate_rule_ids {
    MOCK_METHOD(i_rule_id_set&, get, (const goal_lineage*), (override));
    MOCK_METHOD(const i_rule_id_set&, get, (const goal_lineage*), (const, override));
};

struct MhuEliminationGeneratorUnitTest : public ::testing::Test {
    locator loc;
    identity_bind_map common;
    test_make_var make_var;
    test_bind_map_factory bind_map_factory;
    MockMakeResolutionLineage make_resolution_lineage;
    MockGetGoalCandidateRuleIds get_goal_candidate_rule_ids;
    rule_id_set candidates;
    goal_lineage gl{nullptr, 0};
    resolution_lineage rl{&gl, 0};

    void bind_base_deps() {
        loc.bind_as<i_bind_map>(common);
        loc.bind_as<i_make_var>(make_var);
        loc.bind_as<i_bind_map_factory>(bind_map_factory);
        loc.bind_as<i_make_resolution_lineage>(make_resolution_lineage);
        loc.bind_as<i_get_goal_candidate_rule_ids>(get_goal_candidate_rule_ids);
    }

    expr goal{expr::var{0}};
    expr head_f{expr::functor{"f", {}}};
    expr head_g{expr::functor{"g", {}}};
};

TEST_F(MhuEliminationGeneratorUnitTest, TryAddHeadReturnsFalseWhenUnifyFails) {
    failing_unifier_factory unifier_factory;
    bind_base_deps();
    loc.bind_as<i_unifier_factory>(unifier_factory);
    mhu_elimination_generator mhu{loc};

    EXPECT_FALSE(mhu.try_add_head(&rl, &goal, &head_f));
}

TEST_F(MhuEliminationGeneratorUnitTest, TryAddHeadFalseThenSuccessOnRetry) {
    toggling_unifier_factory unifier_factory;
    bind_base_deps();
    loc.bind_as<i_unifier_factory>(unifier_factory);
    mhu_elimination_generator mhu{loc};

    EXPECT_FALSE(mhu.try_add_head(&rl, &goal, &head_f));
    EXPECT_TRUE(mhu.try_add_head(&rl, &goal, &head_f));
}

TEST_F(MhuEliminationGeneratorUnitTest, ClearMhuHeadsAllowsFreshTryAdd) {
    real_unifier_factory unifier_factory;
    bind_base_deps();
    loc.bind_as<i_unifier_factory>(unifier_factory);
    mhu_elimination_generator mhu{loc};

    ASSERT_TRUE(mhu.try_add_head(&rl, &goal, &head_f));
    mhu.clear_mhu_heads();
    EXPECT_TRUE(mhu.try_add_head(&rl, &goal, &head_f));
}

TEST_F(MhuEliminationGeneratorUnitTest, DuplicateTryAddHeadThrowsInDebug) {
    real_unifier_factory unifier_factory;
    bind_base_deps();
    loc.bind_as<i_unifier_factory>(unifier_factory);
    mhu_elimination_generator mhu{loc};

    ASSERT_TRUE(mhu.try_add_head(&rl, &goal, &head_f));
    EXPECT_THROW(mhu.try_add_head(&rl, &goal, &head_g), std::logic_error);
}
