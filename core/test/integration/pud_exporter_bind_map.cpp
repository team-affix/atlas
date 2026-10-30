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
    pud_term_stack::exporter_factory_t::exporter_t exporter{
        stack.exporters.make_exporter(stack.bind_map, 10, translations)};

    const expr* var(uint32_t index) { return stack.pool.make_var(index); }
    const expr* functor(const char* name, std::vector<const expr*> args) {
        return stack.pool.make_functor(functors.id(name), args);
    }
    const expr* atom(const char* name) { return functor(name, {}); }
};

}

TEST_F(PudExporterBindMapIntegrationTest, AnUnboundCallerVarIsKept) {
    EXPECT_EQ(exporter.export_expr(moduled_expr{var(3), 0u, 0}, 0), var(3));
    EXPECT_EQ(translations.size(), 0u);
}

TEST_F(PudExporterBindMapIntegrationTest, ABoundCallerVarIsKeptWithoutLookingItUp) {
    stack.bind_map.instantiate(stack.bind_map.find(3u), moduled_functor{std::get_if<expr::functor>(&(functor("s", {var(15)}))->content), 0u, 0});
    EXPECT_EQ(exporter.export_expr(moduled_expr{var(3), 0u, 0}, 0), var(3));
    EXPECT_EQ(translations.size(), 0u);
}

TEST_F(PudExporterBindMapIntegrationTest, UnboundCalleeVarsAreRenamedDenselyInFirstMetOrder) {
    EXPECT_EQ(
        exporter.export_expr(moduled_expr{functor("f", {var(7), var(4), var(7)}), 10u, 0}, 0),
        functor("f", {var(10), var(11), var(10)}));
    EXPECT_EQ(translations.size(), 2u);
}

TEST_F(PudExporterBindMapIntegrationTest, FreshVarsAreSharedAcrossExports) {
    exporter.export_expr(moduled_expr{var(5), 10u, 0}, 0);
    EXPECT_EQ(
        exporter.export_expr(moduled_expr{functor("g", {var(5), var(6)}), 10u, 0}, 0),
        functor("g", {var(10), var(11)}));
}

TEST_F(PudExporterBindMapIntegrationTest, ABoundCalleeVarIsReplacedByItsExportedValue) {
    stack.bind_map.instantiate(stack.bind_map.find(12u), moduled_functor{std::get_if<expr::functor>(&(functor("s", {var(5)}))->content), 10u, 0});
    EXPECT_EQ(exporter.export_expr(moduled_expr{var(2), 10u, 0}, 0), functor("s", {var(10)}));
}

TEST_F(PudExporterBindMapIntegrationTest, ACalleeVarMergedIntoACallerClassBecomesTheCallerRep) {
    stack.bind_map.equate(stack.bind_map.find(14u), stack.bind_map.find(3u));
    stack.bind_map.instantiate(stack.bind_map.find(3u), moduled_functor{std::get_if<expr::functor>(&(atom("a"))->content), 0u, 0});
    EXPECT_EQ(exporter.export_expr(moduled_expr{var(4), 10u, 0}, 0), var(3));
    EXPECT_EQ(translations.size(), 0u);
}

TEST_F(PudExporterBindMapIntegrationTest, CalleeBindingsAreFollowedUntilACallerVar) {
    stack.bind_map.instantiate(stack.bind_map.find(11u), moduled_functor{std::get_if<expr::functor>(&(functor("s", {var(2)}))->content), 10u, 0});
    stack.bind_map.instantiate(stack.bind_map.find(12u), moduled_functor{std::get_if<expr::functor>(&(functor("s", {var(1)}))->content), 0u, 0});
    EXPECT_EQ(
        exporter.export_expr(moduled_expr{var(1), 10u, 0}, 0),
        functor("s", {functor("s", {var(1)})}));
}

TEST_F(PudExporterBindMapIntegrationTest, ASharedBoundCalleeClassIsExportedOnce) {
    stack.bind_map.instantiate(stack.bind_map.find(11u), moduled_functor{std::get_if<expr::functor>(&(functor("p", {var(4)}))->content), 10u, 0});
    const expr* first = exporter.export_expr(moduled_expr{var(1), 10u, 0}, 0);
    const expr* second = exporter.export_expr(moduled_expr{functor("h", {var(1), var(1)}), 10u, 0}, 0);
    EXPECT_EQ(second, functor("h", {first, first}));
    EXPECT_EQ(translations.size(), 1u);
}

TEST_F(PudExporterBindMapIntegrationTest, AGroundTermIsReturnedAsTheSameInternedTerm) {
    const expr* ground = functor("p", {atom("a"), atom("b")});
    EXPECT_EQ(exporter.export_expr(moduled_expr{ground, 10u, 0}, 0), ground);
}
