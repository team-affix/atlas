#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_acceptor.hpp"
#include "value_objects/pud_goal_list.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockGetNodeGoals {
    MOCK_METHOD(const pud_goal_list&, get, (pud_node_id));
};
struct MockEnterReplay {
    MOCK_METHOD(std::optional<pud_replay>, enter, (const pud_ubs_state&, const expr*, uint32_t, pud_node_id));
};
struct MockDescendReplay {
    MOCK_METHOD(std::optional<pud_replay>, descend, (const pud_replay&, pud_node_id));
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
struct MockExporter {
    MOCK_METHOD(const expr*, export_expr, (framed_expr));
    MOCK_METHOD(uint32_t, fresh_var_count, (), (const));
};
struct MockMakeExporter {
    MOCK_METHOD(MockExporter&, make_exporter, (MockBindMap&, uint32_t));
};
struct MockFindChangedClasses {
    MOCK_METHOD(std::vector<ubs_class>, find_changed_classes, (const pud_ubs_state&, const pud_ubs_state&));
};

ubs_class singleton(uint32_t var) {
    return ubs_class{.root = var, .size = 1, .rep = var, .value = std::nullopt};
}

using test_acceptor_t = pud_acceptor<
    NiceMock<MockGetNodeGoals>, NiceMock<MockEnterReplay>, NiceMock<MockDescendReplay>, NiceMock<MockMakeVar>,
    NiceMock<MockMakeBindMap>, NiceMock<MockMakeExporter>, NiceMock<MockFindChangedClasses>>;

// The caller has lvc 10 and resolves its goal against the candidate path 2 -> 6.
struct PudAcceptorTest : public ::testing::Test {
    NiceMock<MockGetNodeGoals> goals;
    NiceMock<MockEnterReplay> enter;
    NiceMock<MockDescendReplay> descend;
    NiceMock<MockMakeVar> make_var;
    NiceMock<MockBindMap> accepted;
    NiceMock<MockMakeBindMap> make_bind_map;
    NiceMock<MockExporter> exporter;
    NiceMock<MockMakeExporter> make_exporter;
    NiceMock<MockFindChangedClasses> changed;
    test_acceptor_t acceptor{goals, enter, descend, make_var, make_bind_map, make_exporter, changed};
    expr call_site{expr::functor{1, {}}};
    expr callee_goal{expr::functor{2, {}}};
    expr exported_goal{expr::functor{3, {}}};
    expr value{expr::functor{4, {}}};
    expr exported_value{expr::functor{5, {}}};
    expr var_one{expr::var{1}};
    pud_ubs_state caller_state;
    pud_ubs_state entered_state;
    pud_ubs_state accepted_state;
    pud_goal_list callee_goals = pud_goal_list{.goals = {}, .next_id = 0}.adding(&callee_goal);
    std::vector<pud_node_id> path{2, 6};

    void SetUp() override {
        entered_state.parents = entered_state.parents.set(1, 0);
        accepted_state.parents = accepted_state.parents.set(2, 0);
        ON_CALL(enter, enter(_, _, _, 2)).WillByDefault(Return(pud_replay{.node = 2, .frame_offset = 10, .ubs_state = entered_state}));
        ON_CALL(descend, descend(_, 6)).WillByDefault(Return(pud_replay{.node = 6, .frame_offset = 10, .ubs_state = accepted_state}));
        ON_CALL(make_bind_map, make_bind_map(_)).WillByDefault(ReturnRef(accepted));
        ON_CALL(make_exporter, make_exporter(_, _)).WillByDefault(ReturnRef(exporter));
        ON_CALL(accepted, find(_)).WillByDefault(Invoke(&singleton));
        ON_CALL(goals, get(6)).WillByDefault(ReturnRef(callee_goals));
        ON_CALL(exporter, export_expr((framed_expr{&callee_goal, 10}))).WillByDefault(Return(&exported_goal));
        ON_CALL(exporter, export_expr((framed_expr{&value, 10}))).WillByDefault(Return(&exported_value));
        ON_CALL(make_var, make_var(1)).WillByDefault(Return(&var_one));
    }

    pud_child_delta accept() {
        return acceptor.accept(caller_state, 10, &call_site, path);
    }
};

}

TEST_F(PudAcceptorTest, TheCallSiteEntersThePathsAxiomAtTheLvcAndThePathReplaysFromThere) {
    InSequence in_order;
    EXPECT_CALL(enter, enter(caller_state, &call_site, 10u, 2u));
    EXPECT_CALL(descend, descend(::testing::Field(&pud_replay::node, 2u), 6u));
    EXPECT_CALL(changed, find_changed_classes(caller_state, accepted_state));
    accept();
}

TEST_F(PudAcceptorTest, TheCalleesOpenGoalsAreExportedAndItsFreshVariablesCounted) {
    ON_CALL(exporter, fresh_var_count()).WillByDefault(Return(3));
    EXPECT_CALL(make_exporter, make_exporter(_, 10u));
    EXPECT_EQ(accept(), (pud_child_delta{.substitutions = {}, .body_goals = {&exported_goal}, .var_count = 3}));
}

TEST_F(PudAcceptorTest, ACallerClassMergedUnderASmallerRepresentativeIsSubstitutedByIt) {
    ON_CALL(changed, find_changed_classes(_, _)).WillByDefault(Return(std::vector<ubs_class>{singleton(4)}));
    ON_CALL(accepted, find(4)).WillByDefault(Return(ubs_class{.root = 1, .size = 2, .rep = 1, .value = std::nullopt}));
    EXPECT_EQ(accept().substitutions, (std::vector<pud_substitution>{{.var_idx = 4, .value = &var_one}}));
}

TEST_F(PudAcceptorTest, ACallerClassBoundByTheResolutionIsSubstitutedByItsExportedValue) {
    ON_CALL(changed, find_changed_classes(_, _)).WillByDefault(Return(std::vector<ubs_class>{singleton(4)}));
    ON_CALL(accepted, find(4)).WillByDefault(Return(ubs_class{.root = 4, .size = 1, .rep = 4, .value = framed_expr{&value, 10}}));
    EXPECT_EQ(accept().substitutions, (std::vector<pud_substitution>{{.var_idx = 4, .value = &exported_value}}));
}

TEST_F(PudAcceptorTest, CalleeClassesAndCallerClassesBoundBeforeAreNotSubstituted) {
    const ubs_class callee{.root = 12, .size = 1, .rep = 12, .value = std::nullopt};
    const ubs_class bound_before{.root = 3, .size = 1, .rep = 3, .value = framed_expr{&value, 0}};
    ON_CALL(changed, find_changed_classes(_, _)).WillByDefault(Return(std::vector<ubs_class>{callee, bound_before}));
    ON_CALL(accepted, find(12)).WillByDefault(Return(ubs_class{.root = 12, .size = 1, .rep = 12, .value = framed_expr{&value, 10}}));
    EXPECT_TRUE(accept().substitutions.empty());
}

TEST_F(PudAcceptorTest, AClassThatOnlyGainedCalleeVariablesIsNotSubstituted) {
    ON_CALL(changed, find_changed_classes(_, _)).WillByDefault(Return(std::vector<ubs_class>{singleton(4)}));
    ON_CALL(accepted, find(4)).WillByDefault(Return(ubs_class{.root = 12, .size = 2, .rep = 4, .value = std::nullopt}));
    EXPECT_TRUE(accept().substitutions.empty());
}

TEST_F(PudAcceptorTest, SubstitutionsComeInRepresentativeOrder) {
    const ubs_class later{.root = 2, .size = 2, .rep = 7, .value = std::nullopt};
    ON_CALL(changed, find_changed_classes(_, _)).WillByDefault(Return(std::vector<ubs_class>{later, singleton(4)}));
    ON_CALL(accepted, find(7)).WillByDefault(Return(ubs_class{.root = 2, .size = 3, .rep = 1, .value = std::nullopt}));
    ON_CALL(accepted, find(4)).WillByDefault(Return(ubs_class{.root = 2, .size = 3, .rep = 1, .value = std::nullopt}));
    EXPECT_EQ(accept().substitutions,
        (std::vector<pud_substitution>{{.var_idx = 4, .value = &var_one}, {.var_idx = 7, .value = &var_one}}));
}
