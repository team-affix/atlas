// dbuct_mhu_elimination_generator: try_add_head / remove_head / frame restore contracts.
// Mirrors non-dbuct MHU unit style (real bind/unifier slice + mocks for lineage/var/candidates)
// plus DBUCT remove_head and push/pop_frame.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <stdexcept>
#include <vector>
#include "infrastructure/dbuct_mhu_elimination_generator.hpp"
#include "infrastructure/coroutine.hpp"
#include "infrastructure/dbuct_bind_map.hpp"
#include "infrastructure/dbuct_bind_map_factory.hpp"
#include "infrastructure/pool_allocator.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/unifier_factory.hpp"
#include "infrastructure/unifier.hpp"
#include "infrastructure/ra_rule_id_set.hpp"
#include "functor_fixture.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

std::vector<const resolution_lineage*> collect_elims(
    coroutine<const resolution_lineage*, void> sm) {
    std::vector<const resolution_lineage*> out;
    while (!sm.done()) {
        sm.resume();
        if (sm.has_yield())
            out.push_back(sm.consume_yield());
    }
    return out;
}

} // namespace

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

using bind_map_t = dbuct_bind_map<globalizer>;
using bind_map_factory_t = dbuct_bind_map_factory<globalizer>;
using unifier_t = unifier<globalizer, bind_map_t>;
using unifier_factory_t = unifier_factory<globalizer, bind_map_t>;

using local_bind_map_pool_t = pool_allocator<bind_map_t>;
using test_dbuct_mhu_t = dbuct_mhu_elimination_generator<
    bind_map_t, bind_map_t, bind_map_t,
    local_bind_map_pool_t, local_bind_map_pool_t, local_bind_map_pool_t,
    bind_map_factory_t, unifier_t, unifier_factory_t,
    MockMakeResolutionLineage, MockMakeVar, MockGetGoalCandidateRuleIds>;

struct DbuctMhuEliminationGeneratorUnitTest : public ::testing::Test {
    test_functors functors;
    globalizer g_;
    bind_map_t common{g_};
    bind_map_factory_t bmf{g_};
    local_bind_map_pool_t pool;
    unifier_factory_t uf{g_};
    NiceMock<MockMakeResolutionLineage> mrl;
    NiceMock<MockGetGoalCandidateRuleIds> gcri;
    NiceMock<MockMakeVar> mv;
    ra_rule_id_set candidates;
    goal_lineage gl{nullptr, 0};
    resolution_lineage rl{&gl, 0};
    test_dbuct_mhu_t mhu{common, common, mrl, mv, pool, pool, pool, bmf, uf, gcri};

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

TEST_F(DbuctMhuEliminationGeneratorUnitTest, TryAddHeadReturnsFalseWhenUnifyFails) {
    EXPECT_FALSE(mhu.try_add_head(&rl, {&head_f, 0}, {&head_g, 0}));
}

TEST_F(DbuctMhuEliminationGeneratorUnitTest, TryAddHeadSucceedsWhenUnifiable) {
    EXPECT_TRUE(mhu.try_add_head(&rl, {&goal, 0}, {&head_f, 0}));
}

TEST_F(DbuctMhuEliminationGeneratorUnitTest, RemoveHeadAllowsFreshTryAdd) {
    ASSERT_TRUE(mhu.try_add_head(&rl, {&goal, 0}, {&head_f, 0}));
    mhu.remove_head(&rl);
    EXPECT_TRUE(mhu.try_add_head(&rl, {&goal, 0}, {&head_f, 0}));
}

TEST_F(DbuctMhuEliminationGeneratorUnitTest, RemoveHeadIsIdempotent) {
    ASSERT_TRUE(mhu.try_add_head(&rl, {&goal, 0}, {&head_f, 0}));
    mhu.remove_head(&rl);
    mhu.remove_head(&rl);
    EXPECT_TRUE(mhu.try_add_head(&rl, {&goal, 0}, {&head_f, 0}));
}

TEST_F(DbuctMhuEliminationGeneratorUnitTest, PopFrameRestoresRemovedHead) {
    EXPECT_CALL(gcri, get(&gl)).WillRepeatedly(ReturnRef(candidates));
    ASSERT_TRUE(mhu.try_add_head(&rl, {&goal, 0}, {&head_f, 0}));
    mhu.push_frame();
    mhu.remove_head(&rl);
    EXPECT_THROW(collect_elims(mhu.constrain(&rl)), std::out_of_range);
    mhu.pop_frame();
    EXPECT_NO_THROW(collect_elims(mhu.constrain(&rl)));
}

TEST_F(DbuctMhuEliminationGeneratorUnitTest, PopFrameUndoesTryAddOnNewFrame) {
    mhu.push_frame();
    ASSERT_TRUE(mhu.try_add_head(&rl, {&goal, 0}, {&head_f, 0}));
    mhu.pop_frame();
    EXPECT_TRUE(mhu.try_add_head(&rl, {&goal, 0}, {&head_f, 0}));
}
