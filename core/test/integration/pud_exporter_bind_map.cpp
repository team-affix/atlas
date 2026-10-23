#include <cstdint>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include "functor_fixture.hpp"
#include "pud_term_stack.hpp"

namespace {

struct PudExporterBindMapIntegrationTest : public ::testing::Test {
    test_functors functors;
    pud_term_stack stack;
    std::unordered_map<uint32_t, uint32_t> translations;
    pud_term_stack::term_factory_t::exporter_t exporter{stack.terms.make_exporter(stack.bind_map, 10, translations)};

    const expr* var(uint32_t index) { return stack.pool.make_var(index); }
    const expr* functor(const char* name, std::vector<const expr*> args) {
        return stack.pool.make_functor(functors.id(name), args);
    }
    const expr* atom(const char* name) { return functor(name, {}); }

    void bind(uint32_t global, const expr* value, uint32_t frame_offset) {
        stack.instantiate(global, framed_expr{value, frame_offset});
    }

    const expr* export_at(const expr* skeleton, uint32_t frame_offset) {
        return exporter.export_expr(framed_expr{skeleton, frame_offset});
    }
};

}

TEST_F(PudExporterBindMapIntegrationTest, AnUnboundCallerVarIsKept) {
    EXPECT_EQ(export_at(var(3), 0), var(3));
    EXPECT_EQ(translations.size(), 0u);
}

TEST_F(PudExporterBindMapIntegrationTest, ABoundCallerVarIsKeptWithoutLookingItUp) {
    bind(3, functor("s", {var(15)}), 0);
    EXPECT_EQ(export_at(var(3), 0), var(3));
    EXPECT_EQ(translations.size(), 0u);
}

TEST_F(PudExporterBindMapIntegrationTest, UnboundCalleeVarsAreRenamedDenselyInFirstMetOrder) {
    EXPECT_EQ(export_at(functor("f", {var(7), var(4), var(7)}), 10), functor("f", {var(10), var(11), var(10)}));
    EXPECT_EQ(translations.size(), 2u);
}

TEST_F(PudExporterBindMapIntegrationTest, FreshVarsAreSharedAcrossExports) {
    export_at(var(5), 10);
    EXPECT_EQ(export_at(functor("g", {var(5), var(6)}), 10), functor("g", {var(10), var(11)}));
}

TEST_F(PudExporterBindMapIntegrationTest, ABoundCalleeVarIsReplacedByItsExportedValue) {
    bind(12, functor("s", {var(5)}), 10);
    EXPECT_EQ(export_at(var(2), 10), functor("s", {var(10)}));
}

TEST_F(PudExporterBindMapIntegrationTest, ACalleeVarMergedIntoACallerClassBecomesTheCallerRep) {
    stack.equate(14, 3);
    bind(3, atom("a"), 0);
    EXPECT_EQ(export_at(var(4), 10), var(3));
    EXPECT_EQ(translations.size(), 0u);
}

TEST_F(PudExporterBindMapIntegrationTest, CalleeBindingsAreFollowedUntilACallerVar) {
    bind(11, functor("s", {var(2)}), 10);
    bind(12, functor("s", {var(1)}), 0);
    EXPECT_EQ(export_at(var(1), 10), functor("s", {functor("s", {var(1)})}));
}

TEST_F(PudExporterBindMapIntegrationTest, ASharedBoundCalleeClassIsExportedOnce) {
    bind(11, functor("p", {var(4)}), 10);
    const expr* first = export_at(var(1), 10);
    const expr* second = export_at(functor("h", {var(1), var(1)}), 10);
    EXPECT_EQ(second, functor("h", {first, first}));
    EXPECT_EQ(translations.size(), 1u);
}

TEST_F(PudExporterBindMapIntegrationTest, AGroundTermIsReturnedAsTheSameInternedTerm) {
    const expr* ground = functor("p", {atom("a"), atom("b")});
    EXPECT_EQ(export_at(ground, 10), ground);
}
