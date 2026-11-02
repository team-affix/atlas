#include <cstdint>
#include <variant>
#include <gtest/gtest.h>
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/pud_demoduler.hpp"
#include "infrastructure/pud_link_table.hpp"
#include "infrastructure/pud_normalizer_factory.hpp"
#include "infrastructure/pud_solution_normalizer.hpp"
#include "infrastructure/ubs_bind_map.hpp"

namespace {

using demoduler_t = pud_demoduler<pud_link_table, expr_pool, expr_pool>;
using normalizer_factory_t = pud_normalizer_factory<globalizer, expr_pool, expr_pool, demoduler_t>;

// Module 1 links local 5 to global 21; module 4 links local 8 to global 20 and local 9 to global 30.
struct PudSolutionNormalizerIntegrationTest : public ::testing::Test {
    expr_pool pool;
    globalizer globalize;
    pud_link_table links;
    demoduler_t demoduler{links, pool, pool};
    normalizer_factory_t normalizers{globalize, pool, pool, demoduler};
    pud_solution_normalizer<demoduler_t, normalizer_factory_t> normalizer{demoduler, normalizers};

    void SetUp() override {
        links.insert_link(1, 5, 21);
        links.insert_link(4, 8, 20);
        links.insert_link(4, 9, 30);
    }
};

}

TEST_F(PudSolutionNormalizerIntegrationTest, AnUnboundVariableBecomesItsRepresentative) {
    EXPECT_EQ(normalizer.normalize(pool.make_var(0), 1, pud_ubs_state{}), pool.make_var(0));
}

TEST_F(PudSolutionNormalizerIntegrationTest, ABoundVariableIsReadInTheBindingsModule) {
    const expr* foreign_atom = pool.make_functor(8, {pool.make_var(1)});
    ubs_bind_map bindings{pud_ubs_state{}};
    bindings.instantiate(
        bindings.find(0), moduled_functor{std::get_if<expr::functor>(&foreign_atom->content), 0, 4});
    const pud_ubs_state state = std::move(bindings).persist();
    EXPECT_EQ(normalizer.normalize(pool.make_var(0), 1, state), pool.make_functor(20, {pool.make_var(1)}));
}

TEST_F(PudSolutionNormalizerIntegrationTest, TheHeadsOwnFunctorIsTranslatedInItsModule) {
    const expr* head = pool.make_functor(5, {pool.make_var(0)});
    EXPECT_EQ(normalizer.normalize(head, 1, pud_ubs_state{}), pool.make_functor(21, {pool.make_var(0)}));
}

// Each variable i is bound to succ(var 0) read at frame i + 1, so variable 0 normalizes to a chain of
// succ that is as deep as the binding chain is long. The walk must not recurse once per link.
TEST_F(PudSolutionNormalizerIntegrationTest, ALongBindingChainNormalizesWithoutRecursingPerLink) {
    constexpr uint32_t k_depth = 200000;
    const expr* succ = pool.make_functor(9, {pool.make_var(0)});
    const expr::functor* succ_functor = std::get_if<expr::functor>(&succ->content);
    ubs_bind_map bindings{pud_ubs_state{}};
    for (uint32_t link = 0; link < k_depth; ++link)
        bindings.instantiate(bindings.find(link), moduled_functor{succ_functor, link + 1, 4});
    const pud_ubs_state state = std::move(bindings).persist();
    const expr* normalized = normalizer.normalize(pool.make_var(0), 1, state);
    uint32_t succ_count = 0;
    while (const expr::functor* layer = std::get_if<expr::functor>(&normalized->content)) {
        ASSERT_EQ(layer->id, 30u);
        normalized = layer->args.at(0);
        ++succ_count;
    }
    EXPECT_EQ(succ_count, k_depth);
    EXPECT_EQ(normalized, pool.make_var(k_depth));
}
