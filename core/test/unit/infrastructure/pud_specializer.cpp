#include <cstdint>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_specializer.hpp"

using ::testing::_;
using ::testing::ByMove;
using ::testing::Return;

namespace {

coroutine<uint32_t, bool> scripted_unify(std::vector<uint32_t> reps, bool ok) {
    for (uint32_t rep : reps)
        co_yield rep;
    co_return ok;
}

} // namespace

struct MockMakeVar {
    MOCK_METHOD(const expr*, make_var, (uint32_t));
};

struct MockUnify {
    MOCK_METHOD((coroutine<uint32_t, bool>), unify, (framed_expr, framed_expr));
};

struct PudSpecializerTest : public ::testing::Test {
    MockMakeVar make_var;
    MockUnify unify;
    pud_specializer<MockMakeVar, MockUnify> specializer{make_var, unify};
    expr var_expr{expr::var{0}};
    expr value_expr{expr::var{9}};

    std::vector<uint32_t> drive(uint32_t frame_offset, std::vector<uint32_t> reps, bool ok, bool* result) {
        EXPECT_CALL(make_var, make_var(3u)).WillOnce(Return(&var_expr));
        EXPECT_CALL(unify, unify(_, _)).WillOnce(Return(ByMove(scripted_unify(std::move(reps), ok))));
        auto task = specializer.specialize(frame_offset, pud_specialization{.var_idx = 3, .value = &value_expr});
        std::vector<uint32_t> yielded;
        while (auto rep = task.next())
            yielded.push_back(*rep);
        *result = task.result();
        return yielded;
    }
};

TEST_F(PudSpecializerTest, RepEqualToFrameOffsetIsNotYielded) {
    bool ok = false;
    std::vector<uint32_t> yielded = drive(5, {5}, true, &ok);
    EXPECT_TRUE(yielded.empty());
    EXPECT_TRUE(ok);
}

TEST_F(PudSpecializerTest, RepAboveOffsetIsNotYielded) {
    bool ok = false;
    std::vector<uint32_t> yielded = drive(5, {8}, true, &ok);
    EXPECT_TRUE(yielded.empty());
    EXPECT_TRUE(ok);
}

TEST_F(PudSpecializerTest, UnifyFailureResultIsFalse) {
    bool ok = true;
    drive(5, {}, false, &ok);
    EXPECT_FALSE(ok);
}

TEST_F(PudSpecializerTest, RepsStrictlyBelowOffsetAreYieldedThenTrue) {
    bool ok = false;
    std::vector<uint32_t> yielded = drive(5, {1, 4}, true, &ok);
    EXPECT_EQ(yielded, (std::vector<uint32_t>{1, 4}));
    EXPECT_TRUE(ok);
}
