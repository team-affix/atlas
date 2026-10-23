#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_binder.hpp"

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
struct MockGetNodeAddedSubstitutions {
    MOCK_METHOD(const std::vector<pud_substitution>&, get, (pud_node_id));
};

ubs_class singleton(uint32_t var) {
    return ubs_class{.root = var, .size = 1, .rep = var, .value = std::nullopt};
}

struct PudBinderTest : public ::testing::Test {
    NiceMock<MockBindMap> bind_map;
    NiceMock<MockMakeBindMap> make_bind_map;
    NiceMock<MockGetNodeAddedSubstitutions> substitutions;
    pud_binder<NiceMock<MockMakeBindMap>, NiceMock<MockGetNodeAddedSubstitutions>> binder{make_bind_map, substitutions};
    expr var_one{expr::var{1}};
    expr value{expr::functor{5, {}}};
    std::vector<pud_substitution> child_substitutions{
        pud_substitution{.var_idx = 3, .value = &var_one},
        pud_substitution{.var_idx = 4, .value = &value},
    };
    pud_ubs_state parent_state;
    pud_ubs_state child_state;

    void SetUp() override {
        child_state.parents = child_state.parents.set(3, 1);
        ON_CALL(make_bind_map, make_bind_map(_)).WillByDefault(ReturnRef(bind_map));
        ON_CALL(substitutions, get(8)).WillByDefault(ReturnRef(child_substitutions));
        ON_CALL(bind_map, find(_)).WillByDefault(::testing::Invoke(&singleton));
        ON_CALL(std::move(bind_map), persist()).WillByDefault(Return(child_state));
    }
};

}

TEST_F(PudBinderTest, AVariableSubstitutionEquatesAndATermSubstitutionBindsInOrder) {
    InSequence in_order;
    EXPECT_CALL(make_bind_map, make_bind_map(parent_state));
    EXPECT_CALL(bind_map, equate(singleton(3), singleton(1)));
    EXPECT_CALL(bind_map, instantiate(singleton(4), (framed_expr{&value, 0})));
    EXPECT_EQ(binder.bind(parent_state, 8), child_state);
}
