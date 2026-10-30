#include <cstdint>
#include <vector>
#include <gtest/gtest.h>
#include "functor_fixture.hpp"
#include "pud_term_stack.hpp"
#include "reference_pud_normalize.hpp"

// The reference normalizer the deep-terms test checks the real normalizer against, checked on
// hand-worked bindings made at frame offset 0.
struct ReferencePudNormalizeTest : public ::testing::Test {
    test_functors functors;
    pud_term_stack stack;

    const expr* var(uint32_t index) { return stack.pool.make_var(index); }
    const expr* atom(const char* name) { return stack.pool.make_functor(functors.id(name), {}); }
    const expr* functor(const char* name, std::vector<const expr*> args) {
        return stack.pool.make_functor(functors.id(name), std::move(args));
    }
};

TEST_F(ReferencePudNormalizeTest, AGroundTermIsItself) {
    EXPECT_EQ(reference_pud_normalize(stack, functor("f", {atom("a")})), functor("f", {atom("a")}));
}

TEST_F(ReferencePudNormalizeTest, AnUnboundVariableIsItsRepresentative) {
    stack.bind_map.equate(stack.bind_map.find(2), stack.bind_map.find(5));
    EXPECT_EQ(reference_pud_normalize(stack, var(5)), var(2));
    EXPECT_EQ(reference_pud_normalize(stack, var(2)), var(2));
}

TEST_F(ReferencePudNormalizeTest, ABoundVariableIsReplacedByItsValue) {
    stack.bind_map.instantiate(stack.bind_map.find(0), moduled_functor{std::get_if<expr::functor>(&(atom("a"))->content), 0, 0});
    EXPECT_EQ(reference_pud_normalize(stack, functor("f", {var(0), var(1)})), functor("f", {atom("a"), var(1)}));
}

TEST_F(ReferencePudNormalizeTest, ChainsOfBoundVariablesAreFollowedToTheEnd) {
    stack.bind_map.instantiate(stack.bind_map.find(0), moduled_functor{std::get_if<expr::functor>(&(functor("s", {var(1)}))->content), 0, 0});
    stack.bind_map.instantiate(stack.bind_map.find(1), moduled_functor{std::get_if<expr::functor>(&(functor("s", {var(2)}))->content), 0, 0});
    stack.bind_map.instantiate(stack.bind_map.find(2), moduled_functor{std::get_if<expr::functor>(&(atom("z"))->content), 0, 0});
    EXPECT_EQ(reference_pud_normalize(stack, var(0)), functor("s", {functor("s", {atom("z")})}));
}

TEST_F(ReferencePudNormalizeTest, AVariableBoundToAMergedClassReadsAsItsRepresentative) {
    stack.bind_map.equate(stack.bind_map.find(3), stack.bind_map.find(4));
    stack.bind_map.instantiate(stack.bind_map.find(0), moduled_functor{std::get_if<expr::functor>(&(functor("g", {var(4)}))->content), 0, 0});
    EXPECT_EQ(reference_pud_normalize(stack, var(0)), functor("g", {var(3)}));
}
