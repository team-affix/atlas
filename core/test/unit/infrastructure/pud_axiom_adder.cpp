// pud_axiom_adder: packs a rule, inserts it, adopts queries.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <random>
#include <sstream>
#include <vector>
#include "infrastructure/pud_axiom_adder.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_added_unification.hpp"
#include "value_objects/pud_rule_id.hpp"
#include "value_objects/rule.hpp"

using ::testing::DoAll;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::SaveArg;
using ::testing::_;

struct MockAddAxiom {
    MOCK_METHOD(const pud_rule_id*, add_axiom,
                (size_t,
                 (std::vector<pud_added_unification>),
                 (std::vector<const expr*>),
                 uint32_t), ());
};

struct MockAdoptAxiom {
    MOCK_METHOD(void, adopt_axiom, (const pud_rule_id*), ());
};

using test_adder_t = pud_axiom_adder<
    NiceMock<MockAddAxiom>,
    NiceMock<MockAdoptAxiom>>;

struct PudAxiomAdderTest : public ::testing::Test {
    PudAxiomAdderTest()
        : head_{expr::functor{1, {}}}
        , body_{expr::functor{2, {}}}
        , axiom0_{pud_rule_id::axiom{0}}
        , axiom1_{pud_rule_id::axiom{1}}
        , adder_(add_axiom_, adopt_) {
        ON_CALL(add_axiom_, add_axiom(0, _, _, _)).WillByDefault(Return(&axiom0_));
        ON_CALL(add_axiom_, add_axiom(1, _, _, _)).WillByDefault(Return(&axiom1_));
    }

    expr head_;
    expr body_;
    pud_rule_id axiom0_;
    pud_rule_id axiom1_;
    NiceMock<MockAddAxiom> add_axiom_;
    NiceMock<MockAdoptAxiom> adopt_;
    test_adder_t adder_;
};

TEST_F(PudAxiomAdderTest, AddAxiomPacksHeadBodyAndLvcAndReturnsId) {
    std::vector<pud_added_unification> packed_unifs;
    std::vector<const expr*> packed_body;
    uint32_t packed_lvc = 0;
    EXPECT_CALL(add_axiom_, add_axiom(0, _, _, _))
        .WillOnce(DoAll(SaveArg<1>(&packed_unifs),
                        SaveArg<2>(&packed_body),
                        SaveArg<3>(&packed_lvc),
                        Return(&axiom0_)));
    EXPECT_CALL(adopt_, adopt_axiom(&axiom0_));

    const pud_rule_id* id = adder_.add_axiom(rule{&head_, {&body_}, 3});
    EXPECT_EQ(id, &axiom0_);
    ASSERT_EQ(packed_unifs.size(), 1u);
    EXPECT_EQ(packed_unifs[0].var_idx, 0u);
    EXPECT_EQ(packed_unifs[0].value, &head_);
    ASSERT_EQ(packed_body.size(), 1u);
    EXPECT_EQ(packed_body[0], &body_);
    EXPECT_EQ(packed_lvc, 3u);
}

TEST_F(PudAxiomAdderTest, EntryIdxIncrementsOnEachAdd) {
    EXPECT_CALL(add_axiom_, add_axiom(0, _, _, _)).WillOnce(Return(&axiom0_));
    EXPECT_CALL(add_axiom_, add_axiom(1, _, _, _)).WillOnce(Return(&axiom1_));
    EXPECT_CALL(adopt_, adopt_axiom(_)).Times(2);

    EXPECT_EQ(adder_.add_axiom(rule{&head_, {&body_}, 1}), &axiom0_);
    EXPECT_EQ(adder_.add_axiom(rule{&head_, {}, 1}), &axiom1_);
}

TEST_F(PudAxiomAdderTest, EmptyBodyPacksEmptyGoalVector) {
    std::vector<const expr*> packed_body;
    EXPECT_CALL(add_axiom_, add_axiom(0, _, _, _))
        .WillOnce(DoAll(SaveArg<2>(&packed_body), Return(&axiom0_)));
    adder_.add_axiom(rule{&head_, {}, 2});
    EXPECT_TRUE(packed_body.empty());
}

TEST_F(PudAxiomAdderTest, ZeroVarCountPackedAsLvc) {
    uint32_t packed_lvc = 1;
    EXPECT_CALL(add_axiom_, add_axiom(0, _, _, _))
        .WillOnce(DoAll(SaveArg<3>(&packed_lvc), Return(&axiom0_)));
    adder_.add_axiom(rule{&head_, {&body_}, 0});
    EXPECT_EQ(packed_lvc, 0u);
}

TEST_F(PudAxiomAdderTest, EntryIdxIsMonotonicAcrossManyAdds) {
    std::vector<pud_rule_id> ids;
    ids.reserve(20);
    for (int idx = 0; idx < 20; ++idx)
        ids.push_back(pud_rule_id{pud_rule_id::axiom{static_cast<size_t>(idx)}});
    for (int idx = 0; idx < 20; ++idx) {
        EXPECT_CALL(add_axiom_, add_axiom(static_cast<size_t>(idx), _, _, _))
            .WillOnce(Return(&ids[static_cast<size_t>(idx)]));
        EXPECT_CALL(adopt_, adopt_axiom(&ids[static_cast<size_t>(idx)]));
    }
    for (int idx = 0; idx < 20; ++idx)
        EXPECT_EQ(adder_.add_axiom(rule{&head_, {&body_}, 1}), &ids[static_cast<size_t>(idx)]);
}

TEST_F(PudAxiomAdderTest, FuzzAddAxiom) {
    constexpr uint32_t k_seed = 42;
    std::mt19937 rng{k_seed};
    std::uniform_int_distribution<int> body_len(0, 4);
    std::uniform_int_distribution<uint32_t> var_count(0, 8);
    std::ostringstream log;
    std::vector<pud_rule_id> ids;
    ids.reserve(40);
    for (int step = 0; step < 40; ++step) {
        ids.push_back(pud_rule_id{pud_rule_id::axiom{static_cast<size_t>(step)}});
        const int len = body_len(rng);
        const uint32_t lvc = var_count(rng);
        log << step << ':' << len << ',' << lvc << ' ';
        std::vector<const expr*> packed_body;
        uint32_t packed_lvc = 99;
        EXPECT_CALL(add_axiom_, add_axiom(static_cast<size_t>(step), _, _, _))
            .WillOnce(DoAll(SaveArg<2>(&packed_body), SaveArg<3>(&packed_lvc),
                            Return(&ids.back())));
        EXPECT_CALL(adopt_, adopt_axiom(&ids.back()));
        std::vector<const expr*> body(static_cast<size_t>(len), &body_);
        adder_.add_axiom(rule{&head_, body, lvc});
        EXPECT_EQ(packed_body.size(), static_cast<size_t>(len))
            << "seed " << k_seed << " log " << log.str();
        EXPECT_EQ(packed_lvc, lvc) << "seed " << k_seed << " log " << log.str();
    }
}
