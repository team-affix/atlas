// copier renames variables through i_var_sequencer and pools copied expressions via
// i_make_var + i_make_functor while maintaining a translation_map. Tests mock sequencing and pooling.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/copier.hpp"
#include "interfaces/i_var_sequencer.hpp"
#include "interfaces/i_make_functor.hpp"
#include "interfaces/i_make_var.hpp"

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::NiceMock;
using ::testing::Return;

struct MockVarSequencer : public i_var_sequencer {
    MOCK_METHOD(uint32_t, next, (), (override));
};

struct MockExprPool
    : i_make_functor
    , i_make_var {
    MOCK_METHOD(const expr*, make, (const std::string&, const std::vector<const expr*>&), (override));
    MOCK_METHOD(const expr*, make, (uint32_t), (override));
};

struct CopierUnitTest : public ::testing::Test {
protected:
    void SetUp() override {
        ON_CALL(seq, next()).WillByDefault([this]() { return next_index++; });
        ON_CALL(pool, make(0)).WillByDefault(Return(&pooled_v0));
        ON_CALL(pool, make(1)).WillByDefault(Return(&pooled_v1));
        ON_CALL(pool, make(2)).WillByDefault(Return(&pooled_v2));
    }

    NiceMock<MockVarSequencer> seq;
    NiceMock<MockExprPool> pool;
    copier cp{seq, pool, pool};

    uint32_t next_index = 0;

    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr var2{expr::var{2}};

    expr pooled_v0{expr::var{0}};
    expr pooled_v1{expr::var{1}};
    expr pooled_v2{expr::var{2}};
    expr pooled_v7{expr::var{7}};
    expr pooled_f{expr::functor{"f", {}}};
    expr pooled_g{expr::functor{"g", {}}};
};

TEST_F(CopierUnitTest, FirstVarAllocatesFreshIndex) {
    EXPECT_CALL(seq, next()).WillOnce(Return(7u));
    EXPECT_CALL(pool, make(7u)).WillOnce(Return(&pooled_v7));

    translation_map map;
    EXPECT_EQ(cp.copy(&var0, map), &pooled_v7);
    EXPECT_EQ(map.at(0), 7u);
}

TEST_F(CopierUnitTest, SecondOccurrenceOfSameVarReusesMapping) {
    translation_map map;
    const expr* p1 = cp.copy(&var0, map);
    const expr* p2 = cp.copy(&var0, map);
    EXPECT_EQ(p1, p2);
    EXPECT_EQ(p1, &pooled_v0);
    EXPECT_EQ(map.size(), 1u);
}

TEST_F(CopierUnitTest, DistinctVarsGetDistinctIndices) {
    translation_map map;
    const expr* p0 = cp.copy(&var0, map);
    const expr* p1 = cp.copy(&var1, map);
    EXPECT_EQ(p0, &pooled_v0);
    EXPECT_EQ(p1, &pooled_v1);
    EXPECT_EQ(map.at(0), 0u);
    EXPECT_EQ(map.at(1), 1u);
}

TEST_F(CopierUnitTest, FunctorCopyReturnsPooledNodeWithCopiedArgs) {
    expr f{expr::functor{"f", {&var0, &var1}}};
    translation_map map;

    EXPECT_CALL(pool, make("f", ElementsAre(&pooled_v0, &pooled_v1)))
        .WillOnce(Return(&pooled_f));

    EXPECT_EQ(cp.copy(&f, map), &pooled_f);
}

TEST_F(CopierUnitTest, TernaryFunctorCopyCapturesAllArgs) {
    expr f3{expr::functor{"f", {&var0, &var1, &var2}}};
    translation_map map;

    EXPECT_CALL(pool, make("f", ElementsAre(&pooled_v0, &pooled_v1, &pooled_v2)))
        .WillOnce(Return(&pooled_f));

    EXPECT_EQ(cp.copy(&f3, map), &pooled_f);
    EXPECT_EQ(map.size(), 3u);
}

TEST_F(CopierUnitTest, NestedFunctorCopyUsesInnerPooledArg) {
    expr g{expr::functor{"g", {&var0}}};
    expr f{expr::functor{"f", {&g}}};
    translation_map map;

    EXPECT_CALL(pool, make("g", ElementsAre(&pooled_v0)))
        .WillOnce(Return(&pooled_g));
    EXPECT_CALL(pool, make("f", ElementsAre(&pooled_g)))
        .WillOnce(Return(&pooled_f));

    EXPECT_EQ(cp.copy(&f, map), &pooled_f);
}
