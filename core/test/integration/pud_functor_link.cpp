#include <gtest/gtest.h>
#include "infrastructure/pud_functor_linker.hpp"
#include "infrastructure/pud_link_table.hpp"
#include "infrastructure/pud_module.hpp"
#include "infrastructure/pud_symbol_table.hpp"
#include "value_objects/expr.hpp"

namespace {

struct PudFunctorLinkIntegrationTest : public ::testing::Test {
    pud_symbol_table symbols;
    pud_link_table links;
    pud_functor_linker<pud_symbol_table, pud_link_table> linker{symbols, links};
    expr head{expr::functor{0, {}}};
};

}

TEST_F(PudFunctorLinkIntegrationTest, TwoModulesShareOneGlobalIdForTheSameName) {
    pud_module left{1, {}, {rule(&head, {}, 0)}};
    pud_module right{2, {}, {rule(&head, {}, 0)}};
    left.append_functor_name("[]");
    left.append_functor_name("cons");
    left.append_functor_name("nat");
    right.append_functor_name("[]");
    right.append_functor_name("cons");
    right.append_functor_name("other");
    right.append_functor_name("nat");
    linker.link(left);
    linker.link(right);
    const uint32_t left_global = links.get_global_functor_id_from_local_functor_id(1, 2);
    const uint32_t right_global = links.get_global_functor_id_from_local_functor_id(2, 3);
    EXPECT_EQ(left_global, right_global);
    EXPECT_EQ(symbols.get_name_by_global_functor_id(left_global), "nat");
    EXPECT_NE(links.get_local_functor_id_from_global_functor_id(1, left_global),
        links.get_local_functor_id_from_global_functor_id(2, right_global));
}

TEST_F(PudFunctorLinkIntegrationTest, TheSameLocalIdWithDifferentNamesGetsTwoGlobalIds) {
    pud_module add_module{1, {}, {rule(&head, {}, 0)}};
    pud_module mul_module{2, {}, {rule(&head, {}, 0)}};
    add_module.append_functor_name("[]");
    add_module.append_functor_name("cons");
    add_module.append_functor_name("add");
    mul_module.append_functor_name("[]");
    mul_module.append_functor_name("cons");
    mul_module.append_functor_name("mul");
    linker.link(add_module);
    linker.link(mul_module);
    EXPECT_NE(
        links.get_global_functor_id_from_local_functor_id(1, 2),
        links.get_global_functor_id_from_local_functor_id(2, 2));
}
