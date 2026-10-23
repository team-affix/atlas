#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_leaf_materializer.hpp"
#include "value_objects/pud_leaf_state.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockBindMap {};
struct MockNormalizer {
    MOCK_METHOD(const expr*, normalize, (framed_expr));
};
struct MockGetLeafState {
    MOCK_METHOD(const pud_leaf_state&, get, (pud_node_id));
};
struct MockMakeBindMap {
    MOCK_METHOD(MockBindMap&, make_bind_map, (const pud_ubs_state&));
};
struct MockMakeNormalizer {
    MOCK_METHOD(MockNormalizer&, make_normalizer, (MockBindMap&));
};

struct PudLeafMaterializerTest : public ::testing::Test {
    NiceMock<MockGetLeafState> states;
    MockBindMap bind_map;
    NiceMock<MockNormalizer> normalizer;
    NiceMock<MockMakeBindMap> make_bind_map;
    NiceMock<MockMakeNormalizer> make_normalizer;
    pud_leaf_materializer<NiceMock<MockGetLeafState>, NiceMock<MockMakeBindMap>, NiceMock<MockMakeNormalizer>> materializer{
        states, make_bind_map, make_normalizer};
    pud_leaf_state state{.bindings = {}, .watch = {}, .queries = {}};
    expr skeleton{expr::var{0}};
    expr materialized{expr::functor{3, {}}};
};

}

TEST_F(PudLeafMaterializerTest, ASkeletonIsNormalizedInTheLeafsBindings) {
    state.bindings.parents = state.bindings.parents.set(4, 2);
    ON_CALL(states, get(6)).WillByDefault(ReturnRef(state));
    EXPECT_CALL(make_bind_map, make_bind_map(state.bindings)).WillOnce(ReturnRef(bind_map));
    EXPECT_CALL(make_normalizer, make_normalizer(_)).WillOnce(ReturnRef(normalizer));
    EXPECT_CALL(normalizer, normalize((framed_expr{&skeleton, 0}))).WillOnce(Return(&materialized));
    EXPECT_EQ(materializer.materialize(6, &skeleton), &materialized);
}
