#include <cstdint>
#include <vector>
#include <gtest/gtest.h>
#include "functor_fixture.hpp"
#include "reference_pud_canonical.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/ubs_bind_map.hpp"

// The canonical form the structure watcher fuzz test decides "changed structure" with, checked on
// hand-worked goals and bindings.
struct ReferencePudCanonicalTest : public ::testing::Test {
    test_functors functors;
    expr_pool exprs;
    ubs_bind_map bind_map{pud_ubs_state{}};

    const expr* var(uint32_t index) { return exprs.make_var(index); }
    const expr* atom(const char* name) { return exprs.make_functor(functors.id(name), {}); }
    const expr* functor(const char* name, std::vector<const expr*> args) {
        return exprs.make_functor(functors.id(name), std::move(args));
    }
};

TEST_F(ReferencePudCanonicalTest, AFunctorIsItsIdThenItsArityThenItsArguments) {
    const uint32_t f = functors.id("f");
    const uint32_t a = functors.id("a");
    EXPECT_EQ(
        reference_pud_canonical(pud_ubs_state{}, functor("f", {atom("a")})),
        (std::vector<uint32_t>{f, 1, a, 0}));
}

TEST_F(ReferencePudCanonicalTest, UnboundVariablesAreNumberedByFirstOccurrence) {
    const uint32_t f = functors.id("f");
    EXPECT_EQ(
        reference_pud_canonical(pud_ubs_state{}, functor("f", {var(7), var(2), var(7)})),
        (std::vector<uint32_t>{
            f, 3, k_reference_rep_token_base + 0, k_reference_rep_token_base + 1, k_reference_rep_token_base + 0}));
}

TEST_F(ReferencePudCanonicalTest, RenamingVariablesKeepsTheForm) {
    EXPECT_EQ(
        reference_pud_canonical(pud_ubs_state{}, functor("f", {var(1), var(2)})),
        reference_pud_canonical(pud_ubs_state{}, functor("f", {var(5), var(9)})));
}

TEST_F(ReferencePudCanonicalTest, MergingTwoVariablesOfOneGoalChangesTheForm) {
    const expr* goal = functor("f", {var(1), var(2)});
    const std::vector<uint32_t> before = reference_pud_canonical(pud_ubs_state{}, goal);
    bind_map.equate(bind_map.find(1), bind_map.find(2));
    const pud_ubs_state merged = std::move(bind_map).persist();
    EXPECT_NE(reference_pud_canonical(merged, goal), before);
    EXPECT_EQ(reference_pud_canonical(merged, goal), reference_pud_canonical(pud_ubs_state{}, functor("f", {var(1), var(1)})));
}

TEST_F(ReferencePudCanonicalTest, MergingAVariableWithOneOutsideTheGoalKeepsTheForm) {
    const expr* goal = functor("f", {var(1)});
    const std::vector<uint32_t> before = reference_pud_canonical(pud_ubs_state{}, goal);
    bind_map.equate(bind_map.find(1), bind_map.find(8));
    EXPECT_EQ(reference_pud_canonical(std::move(bind_map).persist(), goal), before);
}

TEST_F(ReferencePudCanonicalTest, ABoundVariableReadsAsItsValue) {
    bind_map.instantiate(bind_map.find(1), framed_functor{std::get_if<expr::functor>(&(functor("g", {var(4)}))->content), 0});
    EXPECT_EQ(
        reference_pud_canonical(std::move(bind_map).persist(), functor("f", {var(1)})),
        reference_pud_canonical(pud_ubs_state{}, functor("f", {functor("g", {var(4)})})));
}

TEST_F(ReferencePudCanonicalTest, InstantiatingAVariableOfTheGoalChangesTheForm) {
    const expr* goal = functor("f", {var(1)});
    const std::vector<uint32_t> before = reference_pud_canonical(pud_ubs_state{}, goal);
    bind_map.instantiate(bind_map.find(1), framed_functor{std::get_if<expr::functor>(&(atom("a"))->content), 0});
    EXPECT_NE(reference_pud_canonical(std::move(bind_map).persist(), goal), before);
}
