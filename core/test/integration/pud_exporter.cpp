#include <cstdint>
#include <vector>
#include <gtest/gtest.h>
#include "functor_fixture.hpp"
#include "infrastructure/pud_exporter.hpp"
#include "pud_unifier_bind_map_stack.hpp"

namespace {

struct PudExporterIntegrationTest : public ::testing::Test {
    using exporter_t = pud_exporter<
        globalizer, expr_pool, expr_pool, pud_unifier_bind_map_stack::bind_map_t,
        pud_unifier_bind_map_stack::bind_map_t, pud_unifier_bind_map_stack::bind_map_t>;

    test_functors functors;
    pud_unifier_bind_map_stack stack;
    exporter_t exporter{
        stack.globalize, stack.bind_backing.pool, stack.bind_backing.pool,
        stack.bind_map, stack.bind_map, stack.bind_map, 10};

    const expr* var(uint32_t index) { return stack.bind_backing.pool.make_var(index); }
    const expr* functor(const char* name, std::vector<const expr*> args) {
        return stack.bind_backing.pool.make_functor(functors.id(name), args);
    }
    const expr* atom(const char* name) { return functor(name, {}); }

    void bind(uint32_t global, const expr* value, uint32_t frame_offset) {
        stack.bind_map.instantiate(global, framed_expr{value, frame_offset});
    }

    const expr* export_at(const expr* skeleton, uint32_t frame_offset) {
        return exporter.export_expr(framed_expr{skeleton, frame_offset});
    }
};

}

TEST_F(PudExporterIntegrationTest, AnUnboundCallerVarIsKept) {
    EXPECT_EQ(export_at(var(3), 0), var(3));
    EXPECT_EQ(exporter.fresh_var_count(), 0u);
}

TEST_F(PudExporterIntegrationTest, ABoundCallerVarIsKeptWithoutLookingItUp) {
    bind(3, functor("s", {var(15)}), 0);
    EXPECT_EQ(export_at(var(3), 0), var(3));
    EXPECT_EQ(exporter.fresh_var_count(), 0u);
}

TEST_F(PudExporterIntegrationTest, UnboundCalleeVarsAreRenamedDenselyInFirstMetOrder) {
    EXPECT_EQ(export_at(functor("f", {var(7), var(4), var(7)}), 10), functor("f", {var(10), var(11), var(10)}));
    EXPECT_EQ(exporter.fresh_var_count(), 2u);
}

TEST_F(PudExporterIntegrationTest, FreshVarsAreSharedAcrossExports) {
    export_at(var(5), 10);
    EXPECT_EQ(export_at(functor("g", {var(5), var(6)}), 10), functor("g", {var(10), var(11)}));
}

TEST_F(PudExporterIntegrationTest, ABoundCalleeVarIsReplacedByItsExportedValue) {
    bind(12, functor("s", {var(5)}), 10);
    EXPECT_EQ(export_at(var(2), 10), functor("s", {var(10)}));
}

TEST_F(PudExporterIntegrationTest, ACalleeVarMergedIntoACallerClassBecomesTheCallerRep) {
    stack.bind_map.equate(14, 3);
    bind(3, atom("a"), 0);
    EXPECT_EQ(export_at(var(4), 10), var(3));
    EXPECT_EQ(exporter.fresh_var_count(), 0u);
}

TEST_F(PudExporterIntegrationTest, CalleeBindingsAreFollowedUntilACallerVar) {
    bind(11, functor("s", {var(2)}), 10);
    bind(12, functor("s", {var(1)}), 0);
    EXPECT_EQ(export_at(var(1), 10), functor("s", {functor("s", {var(1)})}));
}

TEST_F(PudExporterIntegrationTest, ASharedBoundCalleeClassIsExportedOnce) {
    bind(11, functor("p", {var(4)}), 10);
    const expr* first = export_at(var(1), 10);
    const expr* second = export_at(functor("h", {var(1), var(1)}), 10);
    EXPECT_EQ(second, functor("h", {first, first}));
    EXPECT_EQ(exporter.fresh_var_count(), 1u);
}

TEST_F(PudExporterIntegrationTest, AGroundTermIsReturnedAsTheSameInternedTerm) {
    const expr* ground = functor("p", {atom("a"), atom("b")});
    EXPECT_EQ(export_at(ground, 10), ground);
}
