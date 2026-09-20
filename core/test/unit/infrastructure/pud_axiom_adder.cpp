// pud_axiom_adder: packs a rule, inserts it, installs and attaches queries.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>
#include "infrastructure/pud_axiom_adder.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/om_interval.hpp"
#include "value_objects/om_label.hpp"
#include "value_objects/pud_db_node.hpp"
#include "value_objects/pud_rule_id.hpp"
#include "value_objects/rule.hpp"

using ::testing::DoAll;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::SaveArg;
using ::testing::_;

struct MockAddAxiom {
    MOCK_METHOD(const pud_rule_id*, add_axiom, (size_t, pud_db_node), ());
};

struct MockInstall {
    MOCK_METHOD(void, install, (const pud_rule_id*), ());
};

struct MockAttachAxiom {
    MOCK_METHOD(void, attach_axiom, (const pud_rule_id*), ());
};

using test_adder_t = pud_axiom_adder<
    NiceMock<MockAddAxiom>,
    NiceMock<MockInstall>,
    NiceMock<MockAttachAxiom>>;

struct PudAxiomAdderTest : public ::testing::Test {
    PudAxiomAdderTest()
        : head_{expr::functor{1, {}}}
        , body_{expr::functor{2, {}}}
        , axiom0_{pud_rule_id::axiom{0}}
        , axiom1_{pud_rule_id::axiom{1}}
        , adder_(add_axiom_, install_, attach_) {
        ON_CALL(add_axiom_, add_axiom(0, _)).WillByDefault(Return(&axiom0_));
        ON_CALL(add_axiom_, add_axiom(1, _)).WillByDefault(Return(&axiom1_));
    }

    expr head_;
    expr body_;
    pud_rule_id axiom0_;
    pud_rule_id axiom1_;
    NiceMock<MockAddAxiom> add_axiom_;
    NiceMock<MockInstall> install_;
    NiceMock<MockAttachAxiom> attach_;
    test_adder_t adder_;
};

TEST_F(PudAxiomAdderTest, AddAxiomPacksHeadBodyAndLvcAndReturnsId) {
    pud_db_node packed{
        om_interval{om_label(nullptr), om_label(nullptr)}, {}, {}, 0};
    EXPECT_CALL(add_axiom_, add_axiom(0, _)).WillOnce(DoAll(SaveArg<1>(&packed), Return(&axiom0_)));
    EXPECT_CALL(install_, install(&axiom0_));
    EXPECT_CALL(attach_, attach_axiom(&axiom0_));

    const pud_rule_id* id = adder_.add_axiom(rule{&head_, {&body_}, 3});
    EXPECT_EQ(id, &axiom0_);
    ASSERT_EQ(packed.added_unifications.size(), 1u);
    EXPECT_EQ(packed.added_unifications[0].var_idx, 0u);
    EXPECT_EQ(packed.added_unifications[0].value, &head_);
    ASSERT_EQ(packed.added_body_goals.size(), 1u);
    EXPECT_EQ(packed.added_body_goals[0], &body_);
    EXPECT_EQ(packed.lvc, 3u);
}

TEST_F(PudAxiomAdderTest, EntryIdxIncrementsOnEachAdd) {
    EXPECT_CALL(add_axiom_, add_axiom(0, _)).WillOnce(Return(&axiom0_));
    EXPECT_CALL(add_axiom_, add_axiom(1, _)).WillOnce(Return(&axiom1_));
    EXPECT_CALL(install_, install(_)).Times(2);
    EXPECT_CALL(attach_, attach_axiom(_)).Times(2);

    EXPECT_EQ(adder_.add_axiom(rule{&head_, {&body_}, 1}), &axiom0_);
    EXPECT_EQ(adder_.add_axiom(rule{&head_, {}, 1}), &axiom1_);
}
