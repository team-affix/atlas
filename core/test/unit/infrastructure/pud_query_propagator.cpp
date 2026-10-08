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

struct MockParent {
    MOCK_METHOD(const pud_node*, get, (const pud_node*));
};

struct MockRefuted {
    MOCK_METHOD(bool, check_refuted, (const pud_node*));
};

struct MockMakeNode {
    MOCK_METHOD(const pud_node*, make, (std::vector<pud_specialization>, std::vector<const expr*>, uint32_t));
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
    size_t get(const pud_node*) const { return std::numeric_limits<size_t>::max(); }
};

pud_node g_dummy_root{};

struct MockIterateRoots {
    coroutine<const pud_node*, void> iterate_roots() {
        co_yield &g_dummy_root;
    }
};

using test_propagator_t = pud_descender<
    MockBindMap,
    MockUnifier,
    MockSpecializer,
    MockNormalizer,
    MockParent,
    MockMakeNode,
    MockMakeVar,
    MockGlobalize,
    MockRefuted,
    MockCallSite,
    MockIterateRoots>;

} // namespace

struct PudQueryPropagatorTest : public ::testing::Test {
    NiceMock<MockParent>   parent;
    NiceMock<MockMakeNode> make_node;
    NiceMock<MockMakeVar>  make_var;
    NiceMock<MockGlobalize> globalize;
    NiceMock<MockRefuted>  refuted;
    NiceMock<BindLog>      bind_log;
    NiceMock<SpecLog>      spec_log;
    NiceMock<NormLog>      norm_log;
    MockCallSite           call_site;
    MockIterateRoots       iter_roots;
    test_propagator_t propagator{parent, make_node, make_var, globalize, refuted, call_site, iter_roots};
    expr query_expr{expr::var{1}};
    expr goal_a{expr::var{2}};
    expr goal_b{expr::var{3}};
    expr goal_c{expr::var{4}};
    expr spec_value{expr::var{5}};
    expr norm_a{expr::var{6}};
    expr norm_b{expr::var{7}};
    expr norm_c{expr::var{8}};
    pud_node made{};

    void SetUp() override {
        g_dummy_root = pud_node{};
        MockBindMap::log = &bind_log;
        MockSpecializer::log = &spec_log;
        MockNormalizer::log = &norm_log;
        ON_CALL(globalize, globalize(_, _)).WillByDefault(Return(0u));
        ON_CALL(refuted, check_refuted(_)).WillByDefault(Return(false));
        ON_CALL(parent, get(_)).WillByDefault(Return(&g_dummy_root));
        ON_CALL(spec_log, specialize(_, _, _)).WillByDefault(Return(SpecScript{}));
        ON_CALL(norm_log, normalize(_, _, _, _)).WillByDefault(Return(&norm_a));
    }
};

TEST_F(PudQueryPropagatorTest, OpenQueryFromRootGlobalizesVar0AtFrame1) {
    // descent_roots()[0] has lvc=1 (implicit head var), so query_frame_offset = 0+1 = 1
    EXPECT_CALL(globalize, globalize(1u, 0u)).WillOnce(Return(11u));
    EXPECT_CALL(bind_log, bind(11u, &query_expr, 0u));
    auto root = propagator.descent_roots()[0];
    propagator.open_query(root, &query_expr)[0];
}

TEST_F(PudQueryPropagatorTest, RefutedChildIsNotEntered) {
    pud_node child{};
    EXPECT_CALL(refuted, check_refuted(&child)).WillOnce(Return(true));
    auto root = propagator.descent_roots()[0];
    EXPECT_FALSE(propagator.descend(root, &child).has_value());
}

TEST_F(PudQueryPropagatorTest, ChildWithNoSpecializationsIsEntered) {
    pud_node child{};
    EXPECT_CALL(parent, get(&child)).WillOnce(Return(&g_dummy_root));
    auto root = propagator.descent_roots()[0];
    EXPECT_TRUE(propagator.descend(root, &child).has_value());
}

TEST_F(PudQueryPropagatorTest, OneSpecializationSucceeds) {
    pud_node child{};
    child.added_specializations.push_back(pud_specialization{.var_idx = 1, .value = &spec_value});
    EXPECT_CALL(parent, get(&child)).WillOnce(Return(&g_dummy_root));
    EXPECT_CALL(spec_log, specialize(0u, 1u, &spec_value)).WillOnce(Return(SpecScript{{}, true}));
    auto root = propagator.descent_roots()[0];
    EXPECT_TRUE(propagator.descend(root, &child).has_value());
}

TEST_F(PudQueryPropagatorTest, SeveralSpecializationsSucceed) {
    pud_node child{};
    child.added_specializations.push_back(pud_specialization{.var_idx = 1, .value = &spec_value});
    child.added_specializations.push_back(pud_specialization{.var_idx = 2, .value = &goal_a});
    EXPECT_CALL(parent, get(&child)).WillOnce(Return(&g_dummy_root));
    EXPECT_CALL(spec_log, specialize(0u, 1u, &spec_value)).WillOnce(Return(SpecScript{{}, true}));
    EXPECT_CALL(spec_log, specialize(0u, 2u, &goal_a)).WillOnce(Return(SpecScript{{}, true}));
    auto root = propagator.descent_roots()[0];
    EXPECT_TRUE(propagator.descend(root, &child).has_value());
}

TEST_F(PudQueryPropagatorTest, FailingSpecializationIsNotEntered) {
    pud_node child{};
    child.added_specializations.push_back(pud_specialization{.var_idx = 1, .value = &spec_value});
    child.added_specializations.push_back(pud_specialization{.var_idx = 2, .value = &goal_a});
    EXPECT_CALL(parent, get(&child)).WillOnce(Return(&g_dummy_root));
    EXPECT_CALL(spec_log, specialize(0u, 1u, &spec_value)).WillOnce(Return(SpecScript{{7}, true}));
    EXPECT_CALL(spec_log, specialize(0u, 2u, &goal_a)).WillOnce(Return(SpecScript{{}, false}));
    auto root = propagator.descent_roots()[0];
    EXPECT_FALSE(propagator.descend(root, &child).has_value());
}

TEST_F(PudQueryPropagatorTest, BodyGoalCountDoesNotRejectChild) {
    pud_node none{};
    pud_node one{};
    one.added_body_goals.push_back(&goal_a);
    pud_node several{};
    several.added_body_goals = {&goal_a, &goal_b, &goal_c};
    EXPECT_CALL(parent, get(_)).WillRepeatedly(Return(&g_dummy_root));
    auto root = propagator.descent_roots()[0];
    EXPECT_TRUE(propagator.descend(root, &none).has_value());
    EXPECT_TRUE(propagator.descend(root, &one).has_value());
    EXPECT_TRUE(propagator.descend(root, &several).has_value());
}

TEST_F(PudQueryPropagatorTest, OpenQueryAfterPropagateUsesCallerFramePlusLiveVars) {
    pud_node child{};
    child.added_var_count = 4;
    EXPECT_CALL(parent, get(&child)).WillOnce(Return(&g_dummy_root));
    auto root = propagator.descent_roots()[0];
    auto at_child = propagator.descend(root, &child);
    ASSERT_TRUE(at_child.has_value());
    // root.lvc = 1, child.added_var_count = 4 → at_child.lvc = 5 → query_frame_offset = 5
    EXPECT_CALL(globalize, globalize(5u, 0u)).WillOnce(Return(20u));
    EXPECT_CALL(bind_log, bind(20u, &query_expr, 0u));
    propagator.open_query(*at_child, &query_expr)[0];
}

TEST_F(PudQueryPropagatorTest, OpenQueryHandleCanBePropagated) {
    pud_node child{};
    EXPECT_CALL(parent, get(&child)).WillOnce(Return(&g_dummy_root));
    auto root = propagator.descent_roots()[0];
    auto query = propagator.open_query(root, &query_expr)[0];
    EXPECT_TRUE(propagator.descend(query, &child).has_value());
}

TEST_F(PudQueryPropagatorTest, CloseNeverPropagatedRootHasNoGoalsOrSpecs) {
    EXPECT_CALL(make_node, make(_, _, 0u)).WillOnce([&](std::vector<pud_specialization> specs, std::vector<const expr*> goals, uint32_t) -> const pud_node* {
        EXPECT_TRUE(specs.empty());
        EXPECT_TRUE(goals.empty());
        return &made;
    });
    auto root = propagator.descent_roots()[0];
    propagator.close_query(root);
}

TEST_F(PudQueryPropagatorTest, CloseOpenQueryWithoutPropagateHasNoGoalsOrSpecs) {
    EXPECT_CALL(make_node, make(_, _, 0u)).WillOnce([&](std::vector<pud_specialization> specs, std::vector<const expr*> goals, uint32_t) -> const pud_node* {
        EXPECT_TRUE(specs.empty());
        EXPECT_TRUE(goals.empty());
        return &made;
    });
    auto root = propagator.descent_roots()[0];
    auto query = propagator.open_query(root, &query_expr)[0];
    propagator.close_query(query);
}

TEST_F(PudQueryPropagatorTest, CloseOneStepReceivesThatStepsGoalsAndReps) {
    pud_node step{};
    step.added_body_goals = {&goal_a};
    step.added_specializations.push_back(pud_specialization{.var_idx = 1, .value = &spec_value});
    EXPECT_CALL(parent, get(&step)).WillOnce(Return(&g_dummy_root));
    EXPECT_CALL(spec_log, specialize(0u, 1u, &spec_value)).WillOnce(Return(SpecScript{{3}, true}));
    EXPECT_CALL(norm_log, normalize(_, _, _, _)).WillRepeatedly(Return(&norm_a));
    EXPECT_CALL(make_node, make(_, _, _)).WillOnce([&](std::vector<pud_specialization> specs, std::vector<const expr*> goals, uint32_t) -> const pud_node* {
        EXPECT_FALSE(specs.empty());
        EXPECT_FALSE(goals.empty());
        return &made;
    });
    auto root = propagator.descent_roots()[0];
    auto at_step = propagator.descend(root, &step);
    ASSERT_TRUE(at_step.has_value());
    propagator.close_query(*at_step);
}

TEST_F(PudQueryPropagatorTest, CloseThreeStepsAccountsForEveryStep) {
    pud_node step_a{};
    pud_node step_b{};
    pud_node step_c{};
    step_b.added_body_goals = {&goal_a};
    step_c.added_body_goals = {&goal_a, &goal_b, &goal_c};
    step_a.added_specializations.push_back(pud_specialization{.var_idx = 1, .value = &spec_value});
    step_a.added_specializations.push_back(pud_specialization{.var_idx = 2, .value = &goal_a});
    step_c.added_specializations.push_back(pud_specialization{.var_idx = 3, .value = &goal_b});
    EXPECT_CALL(parent, get(&step_a)).WillOnce(Return(&g_dummy_root));
    EXPECT_CALL(parent, get(&step_b)).WillOnce(Return(&step_a));
    EXPECT_CALL(parent, get(&step_c)).WillOnce(Return(&step_b));
    EXPECT_CALL(spec_log, specialize(0u, 1u, &spec_value)).WillOnce(Return(SpecScript{{1}, true}));
    EXPECT_CALL(spec_log, specialize(0u, 2u, &goal_a)).WillOnce(Return(SpecScript{{2}, true}));
    EXPECT_CALL(spec_log, specialize(0u, 3u, &goal_b)).WillOnce(Return(SpecScript{{4}, true}));
    EXPECT_CALL(norm_log, normalize(_, _, _, _)).WillRepeatedly([&](const expr*, uint32_t, uint32_t, std::unordered_map<uint32_t, uint32_t>* translation) {
        uint32_t key = static_cast<uint32_t>(translation->size() + 100);
        translation->emplace(key, key);
        return &norm_a;
    });
    EXPECT_CALL(make_node, make(_, _, _)).WillOnce([&](std::vector<pud_specialization> specs, std::vector<const expr*> goals, uint32_t var_count) -> const pud_node* {
        EXPECT_EQ(specs.size(), 3u);
        EXPECT_EQ(goals.size(), 4u);
        EXPECT_EQ(var_count, 7u);
        return &made;
    });
    auto root = propagator.descent_roots()[0];
    auto at_a = propagator.descend(root, &step_a);
    ASSERT_TRUE(at_a.has_value());
    auto at_b = propagator.descend(*at_a, &step_b);
    ASSERT_TRUE(at_b.has_value());
    auto at_c = propagator.descend(*at_b, &step_c);
    ASSERT_TRUE(at_c.has_value());
    propagator.close_query(*at_c);
}

TEST_F(PudQueryPropagatorTest, CloseVarCountZeroWhenNothingIsNormalizedIntoANewVar) {
    pud_node step{};
    step.added_body_goals = {&goal_a};
    EXPECT_CALL(parent, get(&step)).WillOnce(Return(&g_dummy_root));
    EXPECT_CALL(norm_log, normalize(_, _, _, _)).WillOnce(Return(&norm_a));
    EXPECT_CALL(make_node, make(_, _, 0u)).WillOnce(Return(&made));
    auto root = propagator.descent_roots()[0];
    auto at_step = propagator.descend(root, &step);
    ASSERT_TRUE(at_step.has_value());
    EXPECT_EQ(propagator.close_query(*at_step), &made);
}

TEST_F(PudQueryPropagatorTest, HappyOpenQueryAfterZeroAddedVars) {
    pud_node child{};
    child.added_var_count = 0;
    child.added_specializations.push_back(pud_specialization{.var_idx = 1, .value = &spec_value});
    EXPECT_CALL(parent, get(&child)).WillOnce(Return(&g_dummy_root));
    EXPECT_CALL(spec_log, specialize(0u, 1u, &spec_value)).WillOnce(Return(SpecScript{{}, true}));
    auto root = propagator.descent_roots()[0];
    auto at_child = propagator.descend(root, &child);
    ASSERT_TRUE(at_child.has_value());
    // root.lvc=1 + child.added_var_count=0 = at_child.lvc=1 → query_frame_offset=1
    EXPECT_CALL(globalize, globalize(1u, 0u)).WillOnce(Return(1u));
    EXPECT_CALL(bind_log, bind(1u, &query_expr, 0u));
    propagator.open_query(*at_child, &query_expr)[0];
}
