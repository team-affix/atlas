#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_acceptor.hpp"
#include "value_objects/pud_goal_list.hpp"
#include "value_objects/rule.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

using edited_t = std::optional<std::vector<ubs_class>>;
using translations_t = std::unordered_map<uint32_t, uint32_t>;

struct MockGetNodeGoals {
    MOCK_METHOD(const pud_goal_list&, get, (pud_node_id));
};
struct MockGetAxiom {
    MOCK_METHOD(const rule&, get_axiom, (pud_node_id));
};
struct MockGetInference {
    MOCK_METHOD(const pud_node_delta&, get_inference, (pud_node_id));
};
struct MockMakeVar {
    MOCK_METHOD(const expr*, make_var, (uint32_t));
};
struct MockBindMap {
    MOCK_METHOD(ubs_class, find, (uint32_t));
};
struct MockMakeBindMap {
    MOCK_METHOD(MockBindMap&, make_bind_map, (const pud_ubs_state&));
};
struct MockCycleDetector {};
struct MockMakeCycleDetector {
    MOCK_METHOD(MockCycleDetector&, make_cycle_detector, (MockBindMap&));
};
struct MockUnifier {
    MOCK_METHOD(edited_t, unify, (framed_expr, framed_expr));
};
struct MockMakeUnifier {
    MOCK_METHOD(MockUnifier&, make_unifier, (MockBindMap&, MockCycleDetector&));
};
struct MockExporter {
    MOCK_METHOD(const expr*, export_expr, (framed_expr));
};
struct MockMakeExporter {
    MOCK_METHOD(MockExporter&, make_exporter, (MockBindMap&, uint32_t, translations_t&));
};

ubs_class singleton(uint32_t var) {
    return ubs_class{.root = var, .size = 1, .rep = var, .value = std::nullopt};
}

using test_acceptor_t = pud_acceptor<
    NiceMock<MockGetNodeGoals>, NiceMock<MockGetAxiom>, NiceMock<MockGetInference>,
    NiceMock<MockMakeVar>, NiceMock<MockMakeBindMap>, NiceMock<MockMakeCycleDetector>, NiceMock<MockMakeUnifier>,
    NiceMock<MockMakeExporter>>;

// Caller 3 has lvc 10 and resolves its goal 1 against the candidate path 2 -> 6; node 6 replays one
// substitution of its variable 1.
struct PudAcceptorTest : public ::testing::Test {
    NiceMock<MockGetNodeGoals> goals;
    NiceMock<MockGetAxiom> axioms;
    NiceMock<MockGetInference> inferences;
    NiceMock<MockMakeVar> make_var;
    NiceMock<MockBindMap> accepted;
    NiceMock<MockMakeBindMap> make_bind_map;
    MockCycleDetector cycle_detector;
    NiceMock<MockMakeCycleDetector> make_cycle_detector;
    NiceMock<MockUnifier> unifier;
    NiceMock<MockMakeUnifier> make_unifier;
    NiceMock<MockExporter> exporter;
    NiceMock<MockMakeExporter> make_exporter;
    test_acceptor_t acceptor{
        goals, axioms, inferences, make_var, make_bind_map, make_cycle_detector, make_unifier, make_exporter};
    expr call_site{expr::functor{1, {}}};
    expr axiom_head{expr::functor{1, {}}};
    expr callee_goal{expr::functor{2, {}}};
    expr exported_goal{expr::functor{3, {}}};
    expr value{expr::functor{4, {}}};
    expr exported_value{expr::functor{5, {}}};
    expr substituted_value{expr::functor{6, {}}};
    expr var_one{expr::var{1}};
    pud_ubs_state caller_state;
    pud_goal_list callee_goals = pud_goal_list{.goals = {}, .next_id = 0}.adding(&callee_goal);
    rule axiom{&axiom_head, {}, 0};
    pud_node_delta node_inference{
        .parent = 2,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 1, .value = &substituted_value}},
        .body_goals = {},
        .var_count = 0,
    };
    std::vector<pud_node_id> path{2, 6};
    framed_expr entering_call_site{&call_site, 0};
    framed_expr entered_head{&axiom_head, 10};
    framed_expr replayed_var{&var_one, 10};
    framed_expr replayed_value{&substituted_value, 10};

    MockExporter& export_three_fresh_variables(MockBindMap&, uint32_t, translations_t& translations) {
        translations.emplace(12, 10);
        translations.emplace(13, 11);
        translations.emplace(14, 12);
        return exporter;
    }

    void SetUp() override {
        caller_state.parents = caller_state.parents.set(1, 0);
        ON_CALL(axioms, get_axiom(2)).WillByDefault(ReturnRef(axiom));
        ON_CALL(inferences, get_inference(6)).WillByDefault(ReturnRef(node_inference));
        ON_CALL(make_bind_map, make_bind_map(_)).WillByDefault(ReturnRef(accepted));
        ON_CALL(make_cycle_detector, make_cycle_detector(_)).WillByDefault(ReturnRef(cycle_detector));
        ON_CALL(make_unifier, make_unifier(_, _)).WillByDefault(ReturnRef(unifier));
        ON_CALL(unifier, unify(_, _)).WillByDefault(Return(edited_t{std::vector<ubs_class>{}}));
        ON_CALL(make_exporter, make_exporter(_, _, _)).WillByDefault(ReturnRef(exporter));
        ON_CALL(accepted, find(_)).WillByDefault(Invoke(&singleton));
        ON_CALL(goals, get(6)).WillByDefault(ReturnRef(callee_goals));
        ON_CALL(exporter, export_expr((framed_expr{&callee_goal, 10}))).WillByDefault(Return(&exported_goal));
        ON_CALL(exporter, export_expr((framed_expr{&value, 10}))).WillByDefault(Return(&exported_value));
        ON_CALL(make_var, make_var(1)).WillByDefault(Return(&var_one));
    }

    void edit_on_entering(std::vector<ubs_class> edited) {
        ON_CALL(unifier, unify(entering_call_site, entered_head)).WillByDefault(Return(edited_t{std::move(edited)}));
    }

    pud_node_delta accept() {
        return acceptor.accept(3, 1, caller_state, 10, &call_site, path);
    }
};

}

TEST_F(PudAcceptorTest, TheCallSiteEntersThePathsAxiomAtTheLvcThenEachNodeBelowReplaysItsSubstitutions) {
    EXPECT_CALL(make_bind_map, make_bind_map(caller_state));
    InSequence in_order;
    EXPECT_CALL(unifier, unify(entering_call_site, entered_head));
    EXPECT_CALL(unifier, unify(replayed_var, replayed_value));
    accept();
}

TEST_F(PudAcceptorTest, TheInferenceIsOfTheCallerOnItsCallSiteWithTheCalleesOpenGoalsExported) {
    EXPECT_CALL(make_exporter, make_exporter(_, 10u, _))
        .WillOnce(Invoke(this, &PudAcceptorTest::export_three_fresh_variables));
    EXPECT_EQ(
        accept(),
        (pud_node_delta{.parent = 3, .call_site = 1, .substitutions = {}, .body_goals = {&exported_goal}, .var_count = 3}));
}

TEST_F(PudAcceptorTest, ACallerClassMergedUnderASmallerRepresentativeIsSubstitutedByIt) {
    edit_on_entering({singleton(4)});
    ON_CALL(accepted, find(4)).WillByDefault(Return(ubs_class{.root = 1, .size = 2, .rep = 1, .value = std::nullopt}));
    EXPECT_EQ(accept().substitutions, (std::vector<pud_substitution>{{.var_idx = 4, .value = &var_one}}));
}

TEST_F(PudAcceptorTest, ACallerClassBoundByTheResolutionIsSubstitutedByItsExportedValue) {
    edit_on_entering({singleton(4)});
    ON_CALL(accepted, find(4)).WillByDefault(Return(ubs_class{.root = 4, .size = 1, .rep = 4, .value = framed_expr{&value, 10}}));
    EXPECT_EQ(accept().substitutions, (std::vector<pud_substitution>{{.var_idx = 4, .value = &exported_value}}));
}

TEST_F(PudAcceptorTest, CalleeClassesAndCallerClassesAlreadyBoundWhenEditedAreNotSubstituted) {
    const ubs_class callee{.root = 12, .size = 1, .rep = 12, .value = std::nullopt};
    const ubs_class bound_before{.root = 3, .size = 1, .rep = 3, .value = framed_expr{&value, 0}};
    edit_on_entering({callee, bound_before});
    ON_CALL(accepted, find(12)).WillByDefault(Return(ubs_class{.root = 12, .size = 1, .rep = 12, .value = framed_expr{&value, 10}}));
    EXPECT_TRUE(accept().substitutions.empty());
}

TEST_F(PudAcceptorTest, AClassThatOnlyGainedCalleeVariablesIsNotSubstituted) {
    edit_on_entering({singleton(4)});
    ON_CALL(accepted, find(4)).WillByDefault(Return(ubs_class{.root = 12, .size = 2, .rep = 4, .value = std::nullopt}));
    EXPECT_TRUE(accept().substitutions.empty());
}

TEST_F(PudAcceptorTest, CallerClassesEditedAnywhereOnThePathAreSubstitutedOnceInRepresentativeOrder) {
    edit_on_entering({singleton(7)});
    ON_CALL(unifier, unify(replayed_var, replayed_value))
        .WillByDefault(Return(edited_t{std::vector<ubs_class>{singleton(4), singleton(7)}}));
    ON_CALL(accepted, find(7)).WillByDefault(Return(ubs_class{.root = 2, .size = 3, .rep = 1, .value = std::nullopt}));
    ON_CALL(accepted, find(4)).WillByDefault(Return(ubs_class{.root = 2, .size = 3, .rep = 1, .value = std::nullopt}));
    EXPECT_EQ(accept().substitutions,
        (std::vector<pud_substitution>{{.var_idx = 4, .value = &var_one}, {.var_idx = 7, .value = &var_one}}));
}
