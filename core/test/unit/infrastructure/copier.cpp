#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../../../core/hpp/infrastructure/copier.hpp"
#include "../../../core/hpp/interfaces/i_var_sequencer.hpp"
#include "../../../core/hpp/interfaces/i_expr_pool.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

class MockVarSequencer : public i_var_sequencer {
public:
    MOCK_METHOD(uint32_t, next, (), (override));
};

class MockExprPool : public i_expr_pool {
public:
    MOCK_METHOD(const expr*, functor, (const std::string&, std::vector<const expr*>), (override));
    MOCK_METHOD(const expr*, var, (uint32_t), (override));
    MOCK_METHOD(const expr*, import, (const expr*), (override));
};

class CopierUnitTest : public ::testing::Test {
protected:
    void SetUp() override {
        next_index = 0;
        ON_CALL(seq, next()).WillByDefault([this]() { return next_index++; });
        ON_CALL(pool, var(_)).WillByDefault([this](uint32_t index) -> const expr* {
            switch (index) {
                case 0: return &pooled_v0;
                case 1: return &pooled_v1;
                case 2: return &pooled_v2;
                case 7: return &pooled_v7;
                default: return nullptr;
            }
        });
        ON_CALL(pool, functor(_, _)).WillByDefault([this](const std::string& name,
            const std::vector<const expr*>&) -> const expr* {
            if (name == "g")
                return &pooled_g;
            if (name == "f")
                return &pooled_f;
            return nullptr;
        });
    }

    NiceMock<MockVarSequencer> seq;
    NiceMock<MockExprPool> pool;
    copier cp{seq, pool};

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

// ---------------------------------------------------------------------------
// Variables
// ---------------------------------------------------------------------------

TEST_F(CopierUnitTest, FirstVarAllocatesFreshIndex) {
    next_index = 7;
    ON_CALL(seq, next()).WillByDefault(Return(7u));

    translation_map map;
    EXPECT_EQ(cp.copy(&var0, map), &pooled_v7);
    EXPECT_EQ(map.at(0), 7u);
}

TEST_F(CopierUnitTest, SecondOccurrenceOfSameVarReusesMapping) {
    translation_map map;
    const expr* p1 = cp.copy(&var0, map);
    const expr* p2 = cp.copy(&var0, map);
    EXPECT_EQ(p1, p2);
    EXPECT_EQ(map.size(), 1u);
}

TEST_F(CopierUnitTest, DistinctVarsGetDistinctIndices) {
    translation_map map;
    const expr* p0 = cp.copy(&var0, map);
    const expr* p1 = cp.copy(&var1, map);
    EXPECT_NE(p0, p1);
    EXPECT_EQ(map.at(0), 0u);
    EXPECT_EQ(map.at(1), 1u);
}

// ---------------------------------------------------------------------------
// Functors
// ---------------------------------------------------------------------------

TEST_F(CopierUnitTest, FunctorCopyReturnsPooledNodeWithCopiedArgs) {
    expr f{expr::functor{"f", {&var0, &var1}}};
    translation_map map;
    std::vector<const expr*> captured_args;

    ON_CALL(pool, functor("f", _))
        .WillByDefault([&](const std::string&, const std::vector<const expr*>& args) {
            captured_args = args;
            return &pooled_f;
        });

    EXPECT_EQ(cp.copy(&f, map), &pooled_f);
    ASSERT_EQ(captured_args.size(), 2u);
    EXPECT_EQ(captured_args[0], &pooled_v0);
    EXPECT_EQ(captured_args[1], &pooled_v1);
}

TEST_F(CopierUnitTest, TernaryFunctorCopyCapturesAllArgs) {
    expr f3{expr::functor{"f", {&var0, &var1, &var2}}};
    translation_map map;
    std::vector<const expr*> captured_args;

    ON_CALL(pool, functor("f", _))
        .WillByDefault([&](const std::string&, const std::vector<const expr*>& args) {
            captured_args = args;
            return &pooled_f;
        });

    EXPECT_EQ(cp.copy(&f3, map), &pooled_f);
    ASSERT_EQ(captured_args.size(), 3u);
    EXPECT_EQ(captured_args[0], &pooled_v0);
    EXPECT_EQ(captured_args[1], &pooled_v1);
    EXPECT_EQ(captured_args[2], &pooled_v2);
    EXPECT_EQ(map.size(), 3u);
}

TEST_F(CopierUnitTest, NestedFunctorCopyUsesInnerPooledArg) {
    expr g{expr::functor{"g", {&var0}}};
    expr f{expr::functor{"f", {&g}}};
    translation_map map;
    std::vector<const expr*> captured_f_args;

    ON_CALL(pool, functor("f", _))
        .WillByDefault([&](const std::string&, const std::vector<const expr*>& args) {
            captured_f_args = args;
            return &pooled_f;
        });

    EXPECT_EQ(cp.copy(&f, map), &pooled_f);
    ASSERT_EQ(captured_f_args.size(), 1u);
    EXPECT_EQ(captured_f_args[0], &pooled_g);
}
