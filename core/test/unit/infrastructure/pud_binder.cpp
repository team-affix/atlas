#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_binder.hpp"
#include "value_objects/pud_inference.hpp"
#include "pud_test_node.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockBindMap {
    MOCK_METHOD(ubs_class, find, (uint32_t));
    MOCK_METHOD(std::optional<framed_expr>, equate, (const ubs_class&, const ubs_class&));
    MOCK_METHOD(void, instantiate, (const ubs_class&, framed_expr));
    MOCK_METHOD(pud_ubs_state, persist, (), (ref(&&)));
};
struct MockMakeBindMap {
    MOCK_METHOD(MockBindMap&, make_bind_map, (const pud_ubs_state&));
};
struct MockGetInference {
    MOCK_METHOD(const pud_inference&, get_inference, (pud_global_node_id));
};

ubs_class singleton(uint32_t var) {
    return ubs_class{.root = var, .size = 1, .rep = var, .value = std::nullopt};
}

struct PudBinderTest : public ::testing::Test {
    NiceMock<MockBindMap> bind_map;
    NiceMock<MockMakeBindMap> make_bind_map;
    NiceMock<MockGetInference> inferences;
    pud_binder<NiceMock<MockMakeBindMap>, NiceMock<MockGetInference>> binder{make_bind_map, inferences};
    expr var_one{expr::var{1}};
    expr value{expr::functor{5, {}}};
    pud_inference child_inference{
        .parent = 0,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 3, .value = &var_one}, pud_substitution{.var_idx = 4, .value = &value}},
        .body_goals = {},
        .var_count = 0,
    };
    pud_ubs_state parent_state;
    pud_ubs_state child_state;

    void SetUp() override {
        child_state.parents = child_state.parents.set(3, 1);
        ON_CALL(make_bind_map, make_bind_map(_)).WillByDefault(ReturnRef(bind_map));
        ON_CALL(inferences, get_inference(pud_test_node(8))).WillByDefault(ReturnRef(child_inference));
        ON_CALL(bind_map, find(_)).WillByDefault(::testing::Invoke(&singleton));
        ON_CALL(std::move(bind_map), persist()).WillByDefault(Return(child_state));
    }
};

}

TEST_F(PudBinderTest, AVariableSubstitutionEquatesAndATermSubstitutionBindsInOrder) {
    InSequence in_order;
    EXPECT_CALL(make_bind_map, make_bind_map(parent_state));
    EXPECT_CALL(bind_map, equate(
        (ubs_class{.root = 3, .size = 1, .rep = 3, .value = std::nullopt}),
        (ubs_class{.root = 1, .size = 1, .rep = 1, .value = std::nullopt})));
    EXPECT_CALL(bind_map, instantiate(
        (ubs_class{.root = 4, .size = 1, .rep = 4, .value = std::nullopt}),
        (framed_expr{&value, 0})));
    EXPECT_EQ(binder.bind(parent_state, pud_test_node(8)), child_state);
}
