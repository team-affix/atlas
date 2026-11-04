#include <deque>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_demoduler.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/moduled_expr.hpp"
#include "value_objects/moduled_functor.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;

namespace {

struct MockGetGlobalFunctorId {
    MOCK_METHOD(uint32_t, get_global_functor_id_from_local_functor_id, (pud_module_id, uint32_t));
};

struct MockMakeFunctor {
    MOCK_METHOD(const expr*, make_functor, (uint32_t, (const std::vector<const expr*>&)));
};

struct MockMakeVar {
    MOCK_METHOD(const expr*, make_var, (uint32_t));
};

using demoduler_t = pud_demoduler<MockGetGlobalFunctorId, MockMakeVar, MockMakeFunctor>;

struct PudDemodulerTest : public ::testing::Test {
    MockGetGlobalFunctorId global_ids;
    MockMakeFunctor make_functor;
    MockMakeVar make_var;
    demoduler_t demoduler{global_ids, make_var, make_functor};
    std::deque<expr> owned;

    const expr* var(uint32_t index) {
        owned.push_back(expr{expr::var{index}});
        return &owned.back();
    }

    const expr* functor(uint32_t id, std::vector<const expr*> args) {
        owned.push_back(expr{expr::functor{id, std::move(args)}});
        return &owned.back();
    }
};

}

TEST_F(PudDemodulerTest, ACompositeTermIsRewrittenWithGlobalFunctorIds) {
    const expr* variable = var(0);
    const expr* inner = functor(6, {variable});
    const expr* right = functor(7, {});
    const expr* term = functor(5, {inner, right});
    const expr* global_variable = var(0);
    const expr* global_inner = functor(16, {global_variable});
    const expr* global_right = functor(17, {});
    const expr* global_term = functor(15, {global_inner, global_right});
    InSequence order;
    EXPECT_CALL(make_var, make_var(0u)).WillOnce(Return(global_variable));
    EXPECT_CALL(global_ids, get_global_functor_id_from_local_functor_id(4u, 6u)).WillOnce(Return(16u));
    EXPECT_CALL(make_functor, make_functor(16u, std::vector<const expr*>{global_variable})).WillOnce(Return(global_inner));
    EXPECT_CALL(global_ids, get_global_functor_id_from_local_functor_id(4u, 7u)).WillOnce(Return(17u));
    EXPECT_CALL(make_functor, make_functor(17u, std::vector<const expr*>{})).WillOnce(Return(global_right));
    EXPECT_CALL(global_ids, get_global_functor_id_from_local_functor_id(4u, 5u)).WillOnce(Return(15u));
    EXPECT_CALL(make_functor, make_functor(15u, std::vector<const expr*>{global_inner, global_right})).WillOnce(Return(global_term));
    EXPECT_EQ(demoduler.demodule(moduled_expr{term, 3, 4}), global_term);
}

TEST_F(PudDemodulerTest, TheSameLocalIdInAnotherModuleCanMapToAnotherGlobalId) {
    const expr* term = functor(5, {});
    const expr* global_term = functor(20, {});
    EXPECT_CALL(global_ids, get_global_functor_id_from_local_functor_id(9u, 5u)).WillOnce(Return(20u));
    EXPECT_CALL(make_functor, make_functor(20u, std::vector<const expr*>{})).WillOnce(Return(global_term));
    EXPECT_EQ(demoduler.demodule(moduled_expr{term, 0, 9}), global_term);
}

TEST_F(PudDemodulerTest, ATermNestedFarDeeperThanTheCallStackCouldRecurseIsRewritten) {
    const size_t depth = 200000;
    const expr* term = functor(7, {});
    for (size_t level = 0; level < depth; ++level)
        term = functor(5, {term});
    const expr* global_leaf = functor(17, {});
    const expr* global_term = functor(15, {global_leaf});
    EXPECT_CALL(global_ids, get_global_functor_id_from_local_functor_id(4u, 7u)).WillOnce(Return(17u));
    EXPECT_CALL(global_ids, get_global_functor_id_from_local_functor_id(4u, 5u))
        .Times(static_cast<int>(depth)).WillRepeatedly(Return(15u));
    EXPECT_CALL(make_functor, make_functor(17u, _)).WillOnce(Return(global_leaf));
    EXPECT_CALL(make_functor, make_functor(15u, _)).Times(static_cast<int>(depth)).WillRepeatedly(Return(global_term));
    EXPECT_EQ(demoduler.demodule(moduled_expr{term, 0, 4}), global_term);
}
