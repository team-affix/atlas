// mhu_elimination_generator unit contracts: mock-only mirrors of integration
// try_add_head / clear_mhu_heads / duplicate-head behavior. Goal is fast regression
// with mock discipline, not new scenarios beyond integration coverage.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/mhu_elimination_generator.hpp"
#include "infrastructure/bind_map.hpp"
#include "infrastructure/bind_map_factory.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/unifier_factory.hpp"
#include "infrastructure/unifier.hpp"
#include "infrastructure/rule_id_set.hpp"
#include "infrastructure/ra_rule_id_set.hpp"
#include "functor_fixture.hpp"

using ::testing::_;
using ::testing::Return;
using ::testing::ReturnRef;

struct MockMakeResolutionLineage {
    MOCK_METHOD((const resolution_lineage*), make_resolution_lineage,
        (const goal_lineage*, rule_id));
};

struct MockMakeVar {
    MOCK_METHOD(const expr*, make_var, (uint32_t));
};

struct MockGetGoalCandidateRuleIds {
    MOCK_METHOD(ra_rule_id_set&, get, (const goal_lineage*), (const));
};

using test_mhu_t = mhu_elimination_generator<
    bind_map, bind_map_factory, unifier<bind_map>, unifier_factory<bind_map>,
    MockMakeResolutionLineage, MockMakeVar, MockGetGoalCandidateRuleIds>;

struct MhuEliminationGeneratorUnitTest : public ::testing::Test {
    test_functors functors;
    globalizer g_;
    bind_map common{g_};
    bind_map_factory bmf{g_};
    unifier_factory<bind_map> uf{g_};
    testing::NiceMock<MockMakeResolutionLineage> mrl;
    testing::NiceMock<MockGetGoalCandidateRuleIds> gcri;
    testing::NiceMock<MockMakeVar> mv;
    ra_rule_id_set candidates;
    goal_lineage gl{nullptr, 0};
    resolution_lineage rl{&gl, 0};
    test_mhu_t mhu{common, mrl, mv, bmf, uf, gcri};

    expr goal{expr::var{0}};
    expr head_f{expr::functor{functors.id("f"), {}}};
    expr head_g{expr::functor{functors.id("g"), {}}};

    std::vector<expr> mv_vars;

    void SetUp() override {
        ON_CALL(mv, make_var(_))
            .WillByDefault([this](uint32_t idx) -> const expr* {
                while (mv_vars.size() <= idx)
                    mv_vars.emplace_back(expr::var{static_cast<uint32_t>(mv_vars.size())});
                return &mv_vars[idx];
            });
    }
};

TEST_F(MhuEliminationGeneratorUnitTest, TryAddHeadReturnsFalseWhenUnifyFails) {
    EXPECT_FALSE(mhu.try_add_head(&rl, {&head_f, 0}, {&head_g, 0}));
}

TEST_F(MhuEliminationGeneratorUnitTest, TryAddHeadFalseThenSuccessOnRetry) {
    EXPECT_FALSE(mhu.try_add_head(&rl, {&head_f, 0}, {&head_g, 0}));
    EXPECT_TRUE(mhu.try_add_head(&rl, {&goal, 0}, {&head_f, 0}));
}

TEST_F(MhuEliminationGeneratorUnitTest, ClearMhuHeadsAllowsFreshTryAdd) {
    ASSERT_TRUE(mhu.try_add_head(&rl, {&goal, 0}, {&head_f, 0}));
    mhu.clear_mhu_heads();
    EXPECT_TRUE(mhu.try_add_head(&rl, {&goal, 0}, {&head_f, 0}));
}

TEST_F(MhuEliminationGeneratorUnitTest, DuplicateTryAddHeadThrowsInDebug) {
    ASSERT_TRUE(mhu.try_add_head(&rl, {&goal, 0}, {&head_f, 0}));
    EXPECT_THROW(mhu.try_add_head(&rl, {&goal, 0}, {&head_f, 0}), std::logic_error);
}
