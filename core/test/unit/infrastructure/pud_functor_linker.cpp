#include <string>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_functor_linker.hpp"
#include "infrastructure/pud_module.hpp"
#include "value_objects/expr.hpp"

using ::testing::InSequence;
using ::testing::Return;

namespace {

struct MockInternFunctorName {
    MOCK_METHOD(uint32_t, intern_functor_name, (const std::string&));
};

struct MockInsertLink {
    MOCK_METHOD(void, insert_link, (pud_module_id, uint32_t, uint32_t));
};

struct PudFunctorLinkerTest : public ::testing::Test {
    MockInternFunctorName intern;
    MockInsertLink insert;
    pud_functor_linker<MockInternFunctorName, MockInsertLink> linker{intern, insert};
    expr head{expr::functor{0, {}}};
};

}

TEST_F(PudFunctorLinkerTest, AnEmptyNameVectorInsertsNoLinks) {
    pud_module module{3, {}, {rule(&head, {}, 0)}};
    EXPECT_CALL(intern, intern_functor_name).Times(0);
    EXPECT_CALL(insert, insert_link).Times(0);
    linker.link(module);
}

TEST_F(PudFunctorLinkerTest, AnIdPastTheNameVectorIsNotLinked) {
    expr inner{expr::functor{9, {}}};
    expr outer{expr::functor{5, {&inner}}};
    pud_module module{3, {}, {rule(&outer, {&outer}, 0)}};
    module.append_functor_name("[]");
    EXPECT_CALL(intern, intern_functor_name("[]")).WillOnce(Return(0u));
    EXPECT_CALL(insert, insert_link(3u, 0u, 0u));
    linker.link(module);
}

TEST_F(PudFunctorLinkerTest, EachStoredNameIsInternedAndLinkedAtItsLocalId) {
    pud_module module{3, {}, {rule(&head, {}, 0)}};
    module.append_functor_name("[]");
    module.append_functor_name("cons");
    module.append_functor_name("nat");
    InSequence order;
    EXPECT_CALL(intern, intern_functor_name("[]")).WillOnce(Return(0u));
    EXPECT_CALL(insert, insert_link(3u, 0u, 0u));
    EXPECT_CALL(intern, intern_functor_name("cons")).WillOnce(Return(1u));
    EXPECT_CALL(insert, insert_link(3u, 1u, 1u));
    EXPECT_CALL(intern, intern_functor_name("nat")).WillOnce(Return(5u));
    EXPECT_CALL(insert, insert_link(3u, 2u, 5u));
    linker.link(module);
}
