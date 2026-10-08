#include <cstdint>
#include <limits>
#include <optional>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <immer/map.hpp>
#include <immer/map_transient.hpp>
#include "infrastructure/coroutine.hpp"
#include "infrastructure/pud_descender.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct SpecScript {
    std::vector<uint32_t> reps;
    bool ok = true;
};

struct MockGlobalize {
    MOCK_METHOD(uint32_t, globalize, (uint32_t, uint32_t));
};

struct MockRefuted {
    MOCK_METHOD(bool, check_refuted, (pud_node_id));
};

struct MockMakeNode {
    MOCK_METHOD(pud_node_id, make, (std::vector<pud_specialization>, std::vector<const expr*>, uint32_t));
};

struct MockMakeVar {
    MOCK_METHOD(const expr*, make_var, (uint32_t));
};

struct BindLog {
    MOCK_METHOD(void, bind, (uint32_t, const expr*, uint32_t));
};

struct MockBindMap {
    template<typename G>
    MockBindMap(G&, immer::map<uint32_t, framed_expr>::transient_type&) {}
    void bind(uint32_t key, framed_expr value) {
        log->bind(key, value.skeleton, value.frame_offset);
    }
    framed_expr whnf(framed_expr fe) { return fe; }
    static BindLog* log;
};

BindLog* MockBindMap::log = nullptr;

struct MockUnifier {
    MockUnifier(MockGlobalize&, MockBindMap*) {}
};

struct SpecLog {
    MOCK_METHOD(SpecScript, specialize, (uint32_t, uint32_t, const expr*));
};

struct MockSpecializer {
    template<typename MV, typename U>
    MockSpecializer(MV&, U&) {}
    coroutine<uint32_t, bool> specialize(uint32_t frame_offset, pud_specialization specialization) {
        SpecScript script = log->specialize(frame_offset, specialization.var_idx, specialization.value);
        for (uint32_t rep : script.reps)
            co_yield rep;
        co_return script.ok;
    }
    static SpecLog* log;
};

SpecLog* MockSpecializer::log = nullptr;

struct NormLog {
    MOCK_METHOD(const expr*, normalize, (const expr*, uint32_t, uint32_t, (std::unordered_map<uint32_t, uint32_t>*)));
};

struct MockNormalizer {
    template<typename G, typename MF, typename MV, typename BM>
    MockNormalizer(G&, MF&, MV&, BM&) {}
    const expr* normalize(framed_expr value, uint32_t cutoff, std::unordered_map<uint32_t, uint32_t>& translation) {
        return log->normalize(value.skeleton, value.frame_offset, cutoff, &translation);
    }
    static NormLog* log;
};

NormLog* MockNormalizer::log = nullptr;

struct MockCallSite {
    size_t get(pud_node_id) const { return std::numeric_limits<size_t>::max(); }
};

// Dummy root: var_count=1, head=something, no body goals
const pud_node_id g_dummy_root_id = 0;
expr g_dummy_head{expr::var{99}};

struct MockIterateRoots {
    coroutine<pud_node_id, void> iterate_roots() {
        co_yield g_dummy_root_id;
    }
};

struct MockGetAddedSpecializations {
    std::vector<pud_specialization> specs;
    const std::vector<pud_specialization>& get(pud_node_id) const { return specs; }
};

struct MockGetAddedBodyGoals {
    std::vector<const expr*> goals;
    const std::vector<const expr*>& get(pud_node_id) const { return goals; }
};

struct MockGetAddedVarCountLog {
    MOCK_METHOD(uint32_t, get, (pud_node_id));
};

struct MockGetAddedVarCount {
    uint32_t get(pud_node_id id) const { return log->get(id); }
    static MockGetAddedVarCountLog* log;
};
MockGetAddedVarCountLog* MockGetAddedVarCount::log = nullptr;

struct MockGetAxiomHead {
    const expr* get(pud_node_id) const { return &g_dummy_head; }
};

using test_propagator_t = pud_descender<
    MockBindMap,
    MockUnifier,
    MockSpecializer,
    MockNormalizer,
    MockMakeNode,
    MockMakeVar,
    MockGlobalize,
    MockRefuted,
    MockCallSite,
    MockIterateRoots,
    MockGetAddedSpecializations,
    MockGetAddedBodyGoals,
    MockGetAddedVarCount,
    MockGetAxiomHead>;

} // namespace

struct PudQueryPropagatorTest : public ::testing::Test {
    NiceMock<MockMakeNode>  make_node;
    NiceMock<MockMakeVar>   make_var;
    NiceMock<MockGlobalize> globalize;
    NiceMock<MockRefuted>   refuted;
    NiceMock<BindLog>       bind_log;
    NiceMock<SpecLog>       spec_log;
    NiceMock<NormLog>       norm_log;
    NiceMock<MockGetAddedVarCountLog> var_count_log;
    MockCallSite            call_site;
    MockIterateRoots        iter_roots;
    MockGetAddedSpecializations get_specs;
    MockGetAddedBodyGoals   get_body_goals;
    MockGetAddedVarCount    get_var_count;
    MockGetAxiomHead        get_axiom_head;
    test_propagator_t propagator{
        make_node, make_var, globalize, refuted, call_site, iter_roots,
        get_specs, get_body_goals, get_var_count, get_axiom_head};
    expr query_expr{expr::var{1}};
    expr goal_a{expr::var{2}};
    expr goal_b{expr::var{3}};
    expr goal_c{expr::var{4}};
    expr spec_value{expr::var{5}};
    expr norm_a{expr::var{6}};
    expr norm_b{expr::var{7}};
    expr norm_c{expr::var{8}};
    pud_node_id made_id = 99;

    void SetUp() override {
        MockBindMap::log = &bind_log;
        MockSpecializer::log = &spec_log;
        MockNormalizer::log = &norm_log;
        MockGetAddedVarCount::log = &var_count_log;
        // root has var_count=1 (one var for the root)
        ON_CALL(var_count_log, get(_)).WillByDefault(Return(0u));
        ON_CALL(var_count_log, get(g_dummy_root_id)).WillByDefault(Return(1u));
        ON_CALL(globalize, globalize(_, _)).WillByDefault(Return(0u));
        ON_CALL(refuted, check_refuted(_)).WillByDefault(Return(false));
        ON_CALL(spec_log, specialize(_, _, _)).WillByDefault(Return(SpecScript{}));
        ON_CALL(norm_log, normalize(_, _, _, _)).WillByDefault(Return(&norm_a));
    }
};

TEST_F(PudQueryPropagatorTest, OpenQueryFromRootBindsAnchorVarToQueryExpr) {
    // root var_count=1, so query_frame_offset = 0+1 = 1
    // anchor_var_idx = 1 (= var_count), so globalize(1, 1) is called for bind
    EXPECT_CALL(globalize, globalize(1u, 1u)).WillRepeatedly(Return(11u));
    EXPECT_CALL(bind_log, bind(11u, &query_expr, 0u));
    EXPECT_CALL(spec_log, specialize(1u, 1u, &g_dummy_head)).WillOnce(Return(SpecScript{}));
    propagator.open_query(propagator.descent_roots()[0], &query_expr);
}

TEST_F(PudQueryPropagatorTest, RefutedChildIsNotEntered) {
    pud_node_id child = 5;
    EXPECT_CALL(refuted, check_refuted(child)).WillOnce(Return(true));
    auto root = propagator.descent_roots()[0];
    EXPECT_FALSE(propagator.descend(root, child).has_value());
}

TEST_F(PudQueryPropagatorTest, ChildWithNoSpecializationsIsEntered) {
    pud_node_id child = 5;
    auto root = propagator.descent_roots()[0];
    EXPECT_TRUE(propagator.descend(root, child).has_value());
}

TEST_F(PudQueryPropagatorTest, OneSpecializationSucceeds) {
    pud_node_id child = 5;
    get_specs.specs = {{.var_idx = 1, .value = &spec_value}};
    EXPECT_CALL(spec_log, specialize(0u, 1u, &spec_value)).WillOnce(Return(SpecScript{{}, true}));
    auto root = propagator.descent_roots()[0];
    EXPECT_TRUE(propagator.descend(root, child).has_value());
}

TEST_F(PudQueryPropagatorTest, SeveralSpecializationsSucceed) {
    pud_node_id child = 5;
    get_specs.specs = {
        {.var_idx = 1, .value = &spec_value},
        {.var_idx = 2, .value = &goal_a}
    };
    EXPECT_CALL(spec_log, specialize(0u, 1u, &spec_value)).WillOnce(Return(SpecScript{{}, true}));
    EXPECT_CALL(spec_log, specialize(0u, 2u, &goal_a)).WillOnce(Return(SpecScript{{}, true}));
    auto root = propagator.descent_roots()[0];
    EXPECT_TRUE(propagator.descend(root, child).has_value());
}

TEST_F(PudQueryPropagatorTest, FailingSpecializationIsNotEntered) {
    pud_node_id child = 5;
    get_specs.specs = {
        {.var_idx = 1, .value = &spec_value},
        {.var_idx = 2, .value = &goal_a}
    };
    EXPECT_CALL(spec_log, specialize(0u, 1u, &spec_value)).WillOnce(Return(SpecScript{{7}, true}));
    EXPECT_CALL(spec_log, specialize(0u, 2u, &goal_a)).WillOnce(Return(SpecScript{{}, false}));
    auto root = propagator.descent_roots()[0];
    EXPECT_FALSE(propagator.descend(root, child).has_value());
}

TEST_F(PudQueryPropagatorTest, BodyGoalCountDoesNotRejectChild) {
    pud_node_id none = 5, one = 6, several = 7;
    // goals are looked up per node — use empty by default
    auto root = propagator.descent_roots()[0];
    EXPECT_TRUE(propagator.descend(root, none).has_value());
    get_body_goals.goals = {&goal_a};
    EXPECT_TRUE(propagator.descend(root, one).has_value());
    get_body_goals.goals = {&goal_a, &goal_b, &goal_c};
    EXPECT_TRUE(propagator.descend(root, several).has_value());
}

TEST_F(PudQueryPropagatorTest, OpenQueryAfterPropagateUsesCallerFramePlusLiveVars) {
    pud_node_id child = 5;
    ON_CALL(var_count_log, get(child)).WillByDefault(Return(4u));
    auto root = propagator.descent_roots()[0];
    auto at_child = propagator.descend(root, child);
    ASSERT_TRUE(at_child.has_value());
    // root.lvc=1, child.var_count=4 → at_child.lvc=5 → query_frame_offset=5
    // anchor_var_idx=1 (root var_count), globalize(5, 1)
    EXPECT_CALL(globalize, globalize(5u, 1u)).WillRepeatedly(Return(20u));
    EXPECT_CALL(bind_log, bind(20u, &query_expr, 0u));
    EXPECT_CALL(spec_log, specialize(5u, 1u, &g_dummy_head)).WillOnce(Return(SpecScript{}));
    propagator.open_query(*at_child, &query_expr);
}

TEST_F(PudQueryPropagatorTest, OpenQueryHandleCanBePropagated) {
    pud_node_id child = 5;
    EXPECT_CALL(spec_log, specialize(_, 1u, &g_dummy_head)).WillOnce(Return(SpecScript{}));
    auto root = propagator.descent_roots()[0];
    auto query = propagator.open_query(root, &query_expr)[0];
    EXPECT_TRUE(propagator.descend(query, child).has_value());
}

TEST_F(PudQueryPropagatorTest, CloseNeverPropagatedRootHasNoGoalsOrSpecs) {
    EXPECT_CALL(make_node, make(_, _, 0u)).WillOnce([&](std::vector<pud_specialization> specs, std::vector<const expr*> goals, uint32_t) -> pud_node_id {
        EXPECT_TRUE(specs.empty());
        EXPECT_TRUE(goals.empty());
        return made_id;
    });
    auto root = propagator.descent_roots()[0];
    propagator.close_query(root);
}

TEST_F(PudQueryPropagatorTest, CloseOpenQueryWithoutPropagateHasNoGoalsOrSpecs) {
    EXPECT_CALL(spec_log, specialize(_, _, _)).WillRepeatedly(Return(SpecScript{}));
    EXPECT_CALL(make_node, make(_, _, 0u)).WillOnce([&](std::vector<pud_specialization> specs, std::vector<const expr*> goals, uint32_t) -> pud_node_id {
        EXPECT_TRUE(specs.empty());
        EXPECT_TRUE(goals.empty());
        return made_id;
    });
    auto root = propagator.descent_roots()[0];
    auto query = propagator.open_query(root, &query_expr)[0];
    propagator.close_query(query);
}

TEST_F(PudQueryPropagatorTest, CloseOneStepReceivesThatStepsGoalsAndReps) {
    pud_node_id step = 5;
    get_body_goals.goals = {&goal_a};
    get_specs.specs = {{.var_idx = 1, .value = &spec_value}};
    EXPECT_CALL(spec_log, specialize(0u, 1u, &spec_value)).WillOnce(Return(SpecScript{{3}, true}));
    EXPECT_CALL(norm_log, normalize(_, _, _, _)).WillRepeatedly(Return(&norm_a));
    EXPECT_CALL(make_node, make(_, _, _)).WillOnce([&](std::vector<pud_specialization> specs, std::vector<const expr*> goals, uint32_t) -> pud_node_id {
        EXPECT_FALSE(specs.empty());
        EXPECT_FALSE(goals.empty());
        return made_id;
    });
    auto root = propagator.descent_roots()[0];
    auto at_step = propagator.descend(root, step);
    ASSERT_TRUE(at_step.has_value());
    propagator.close_query(*at_step);
}

TEST_F(PudQueryPropagatorTest, CloseVarCountZeroWhenNothingIsNormalizedIntoANewVar) {
    pud_node_id step = 5;
    get_body_goals.goals = {&goal_a};
    get_specs.specs.clear();
    EXPECT_CALL(norm_log, normalize(_, _, _, _)).WillRepeatedly(Return(&norm_a));
    EXPECT_CALL(make_node, make(_, _, 0u)).WillOnce(Return(made_id));
    auto root = propagator.descent_roots()[0];
    auto at_step = propagator.descend(root, step);
    ASSERT_TRUE(at_step.has_value());
    EXPECT_EQ(propagator.close_query(*at_step), made_id);
}
