#include <array>
#include <unordered_set>
#include <gtest/gtest.h>
#include "infrastructure/globalizer.hpp"
#include "infrastructure/unifier.hpp"
#include "infrastructure/bind_map.hpp"
#include "functor_fixture.hpp"

namespace {

bool run_unify(unifier<bind_map>& u, framed_expr lhs, framed_expr rhs,
               std::unordered_set<uint32_t>& vars_touched) {
    auto task = u.unify(lhs, rhs);
    while (!task.done()) {
        task.resume();
        if (task.has_yield())
            vars_touched.insert(task.consume_yield());
    }
    return task.result();
}

// Wrap bare pointer in global frame.
framed_expr fe(const expr* e) { return {e, 0}; }

} // namespace

struct UnifierBindMapIntegrationTest : public ::testing::Test {
protected:
    test_functors functors;
    void SetUp() override {
        u = std::make_unique<unifier<bind_map>>(g, &bm);
    }

    globalizer g;
    bind_map bm{g};
    std::unique_ptr<unifier<bind_map>> u;
    std::unordered_set<uint32_t> vars_touched;
    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr var2{expr::var{2}};
    expr var3{expr::var{3}};
    expr var4{expr::var{4}};
    expr var5{expr::var{5}};
    expr func{expr::functor{functors.id("f"), {}}};
    expr func2{expr::functor{functors.id("g"), {}}};

    const expr* whnf(const expr* e) { return bm.whnf({e, 0}).skeleton; }
};

TEST_F(UnifierBindMapIntegrationTest, UnifyVarAndFunctorBindsVarInRealBindMap) {
    EXPECT_TRUE(run_unify(*u, fe(&var0), fe(&func), vars_touched));
    EXPECT_EQ(vars_touched, (std::unordered_set<uint32_t>{0}));
    EXPECT_EQ(whnf(&var0), &func);
}

TEST_F(UnifierBindMapIntegrationTest, YoungerRhsBindsToOlderLhs) {
    EXPECT_TRUE(run_unify(*u, fe(&var0), fe(&var1), vars_touched));
    EXPECT_EQ(vars_touched, (std::unordered_set<uint32_t>{0, 1}));
    EXPECT_EQ(whnf(&var1), &var0);
    EXPECT_EQ(whnf(&var0), &var0);
}

TEST_F(UnifierBindMapIntegrationTest, YoungerLhsBindsToOlderRhs) {
    EXPECT_TRUE(run_unify(*u, fe(&var1), fe(&var0), vars_touched));
    EXPECT_EQ(vars_touched, (std::unordered_set<uint32_t>{0, 1}));
    EXPECT_EQ(whnf(&var1), &var0);
    EXPECT_EQ(whnf(&var0), &var0);
}

TEST_F(UnifierBindMapIntegrationTest, UnifyAlreadyBoundVarThroughWhnfSucceeds) {
    bm.bind(0, {&func, 0});
    EXPECT_TRUE(run_unify(*u, fe(&var0), fe(&func), vars_touched));
    EXPECT_TRUE(vars_touched.empty());
}

TEST_F(UnifierBindMapIntegrationTest, UnifyChainedVarsThroughBindMapSucceeds) {
    std::unordered_set<uint32_t> vars_touched2;
    EXPECT_TRUE(run_unify(*u, fe(&var0), fe(&var1), vars_touched));
    EXPECT_TRUE(run_unify(*u, fe(&var1), fe(&func), vars_touched2));
    EXPECT_EQ(whnf(&var0), &func);
}

TEST_F(UnifierBindMapIntegrationTest, UnifyTwoVarChainsMergesToOldestRepr) {
    bm.bind(1, {&var0, 0});
    bm.bind(2, {&var1, 0});
    bm.bind(4, {&var3, 0});
    bm.bind(5, {&var4, 0});

    EXPECT_TRUE(run_unify(*u, fe(&var2), fe(&var5), vars_touched));
    EXPECT_EQ(vars_touched, (std::unordered_set<uint32_t>{0, 3}));
    EXPECT_EQ(whnf(&var2), &var0);
    EXPECT_EQ(whnf(&var5), &var0);
}

TEST_F(UnifierBindMapIntegrationTest, ManyScrambledUnificationsAllWhnfToOldestVar) {
    constexpr uint32_t n = 12;
    std::array<expr, n> vars;
    for (uint32_t i = 0; i < n; ++i)
        vars[i] = expr{expr::var{i}};

    constexpr std::array<std::pair<uint32_t, uint32_t>, 12> pairs = {{
        {7, 3}, {11, 2}, {5, 9}, {1, 8}, {4, 6}, {10, 0},
        {3, 11}, {6, 2}, {9, 1}, {8, 4}, {2, 5}, {10, 7},
    }};

    for (const auto& [a, b] : pairs)
        EXPECT_TRUE(run_unify(*u, fe(&vars[a]), fe(&vars[b]), vars_touched));

    for (const expr& v : vars)
        EXPECT_EQ(whnf(&v), &vars[0]);
}

TEST_F(UnifierBindMapIntegrationTest, UnifyVarToFunctorContainingSameVarFails) {
    expr f_var0{expr::functor{functors.id("f"), {&var0}}};
    EXPECT_FALSE(run_unify(*u, fe(&var0), fe(&f_var0), vars_touched));
    EXPECT_TRUE(vars_touched.empty());
    EXPECT_EQ(whnf(&var0), &var0);
}

TEST_F(UnifierBindMapIntegrationTest, UnifyBinaryFunctorsWithVarArgsBindsBoth) {
    expr lhs{expr::functor{functors.id("f"), {&var0, &var1}}};
    expr rhs{expr::functor{functors.id("f"), {&func, &func2}}};
    EXPECT_TRUE(run_unify(*u, fe(&lhs), fe(&rhs), vars_touched));
    EXPECT_EQ(vars_touched, (std::unordered_set<uint32_t>{0, 1}));
    EXPECT_EQ(whnf(&var0), &func);
    EXPECT_EQ(whnf(&var1), &func2);
}

TEST_F(UnifierBindMapIntegrationTest, UnifyTernaryFunctorsWithVarArgsBindsAll) {
    expr lhs{expr::functor{functors.id("f"), {&var0, &var1, &var2}}};
    expr rhs{expr::functor{functors.id("f"), {&func, &func2, &func}}};
    EXPECT_TRUE(run_unify(*u, fe(&lhs), fe(&rhs), vars_touched));
    EXPECT_EQ(vars_touched, (std::unordered_set<uint32_t>{0, 1, 2}));
    EXPECT_EQ(whnf(&var0), &func);
    EXPECT_EQ(whnf(&var1), &func2);
    EXPECT_EQ(whnf(&var2), &func);
}

TEST_F(UnifierBindMapIntegrationTest, UnifyDepth2FunctorsBindsInnerVar) {
    expr f_var0{expr::functor{functors.id("f"), {&var0}}};
    expr f_func{expr::functor{functors.id("f"), {&func}}};
    expr lhs{expr::functor{functors.id("f"), {&f_var0}}};
    expr rhs{expr::functor{functors.id("f"), {&f_func}}};
    EXPECT_TRUE(run_unify(*u, fe(&lhs), fe(&rhs), vars_touched));
    EXPECT_EQ(vars_touched, (std::unordered_set<uint32_t>{0}));
    EXPECT_EQ(whnf(&var0), &func);
}
