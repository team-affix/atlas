// pud_candidate_rebaser: spine theorem, pin F, then resume until stable.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <optional>
#include "infrastructure/pud_candidate_rebaser.hpp"
#include "value_objects/pud_candidate_search_context.hpp"
#include "value_objects/pud_lineage.hpp"
#include "value_objects/pud_witness_pair.hpp"
#include "value_objects/pud_witness_search_context.hpp"

using ::testing::NiceMock;
using ::testing::Return;
using ::testing::_;

struct MockEnterPath {
    MOCK_METHOD(const pud_lineage*, enter_to,
                (const pud_lineage*, size_t, uint32_t, const pud_lineage*), ());
};

struct MockResumeWitnessSearch {
    MOCK_METHOD(void, resume, (pud_witness_search_context&), ());
};

struct MockResumeCandidateSearch {
    MOCK_METHOD(void, resume, (pud_candidate_search_context&), ());
};

struct MockGetParent {
    MOCK_METHOD(const pud_lineage*, get, (const pud_lineage*), ());
};

using test_rebaser_t = pud_candidate_rebaser<
    NiceMock<MockEnterPath>,
    NiceMock<MockResumeWitnessSearch>,
    NiceMock<MockResumeCandidateSearch>,
    NiceMock<MockGetParent>>;

struct PudCandidateRebaserTest : public ::testing::Test {
    PudCandidateRebaserTest()
        : leaf_{pud_lineage::axiom{99}}
        , a0_{pud_lineage::axiom{0}}
        , c0_{pud_lineage::inference{&a0_, 0, &a0_}}
        , c1_{pud_lineage::inference{&a0_, 1, &a0_}}
        , g0_{pud_lineage::inference{&c0_, 0, &a0_}}
        , rebaser_(enter_path_, witness_, candidate_, get_parent_) {
        ON_CALL(get_parent_, get(&a0_)).WillByDefault(Return(nullptr));
        ON_CALL(get_parent_, get(&c0_)).WillByDefault(Return(&a0_));
        ON_CALL(get_parent_, get(&c1_)).WillByDefault(Return(&a0_));
        ON_CALL(get_parent_, get(&g0_)).WillByDefault(Return(&c0_));
        ON_CALL(enter_path_, enter_to(_, _, _, _))
            .WillByDefault([](const pud_lineage*, size_t, uint32_t, const pud_lineage* dest) {
                return dest;
            });
        ON_CALL(witness_, resume(_)).WillByDefault([](pud_witness_search_context& side) {
            if (side.current == nullptr)
                return;
        });
        ON_CALL(candidate_, resume(_)).WillByDefault([](pud_candidate_search_context&) {});
    }

    pud_witness_search_context make_pin(const pud_lineage* search_root,
                                        const pud_lineage* current) {
        return pud_witness_search_context{&a0_, 0, 1, search_root, current};
    }

    pud_lineage leaf_;
    pud_lineage a0_;
    pud_lineage c0_;
    pud_lineage c1_;
    pud_lineage g0_;
    NiceMock<MockEnterPath> enter_path_;
    NiceMock<MockResumeWitnessSearch> witness_;
    NiceMock<MockResumeCandidateSearch> candidate_;
    NiceMock<MockGetParent> get_parent_;
    test_rebaser_t rebaser_;
};

TEST_F(PudCandidateRebaserTest, SpineFailReturnsNullopt) {
    EXPECT_CALL(enter_path_, enter_to(&leaf_, 0, 2, &a0_)).WillOnce(Return(nullptr));
    EXPECT_CALL(witness_, resume(_)).Times(0);
    EXPECT_CALL(candidate_, resume(_)).Times(0);
    const std::optional<pud_candidate_search_context> out = rebaser_.rebase(
        &leaf_, 0, 2, &a0_, std::nullopt);
    EXPECT_FALSE(out.has_value());
}

TEST_F(PudCandidateRebaserTest, NullCursorReturnsNullopt) {
    EXPECT_CALL(enter_path_, enter_to(_, _, _, _)).Times(0);
    const std::optional<pud_candidate_search_context> out = rebaser_.rebase(
        &leaf_, 0, 2, nullptr, std::nullopt);
    EXPECT_FALSE(out.has_value());
}

TEST_F(PudCandidateRebaserTest, SelfWitnessAfterSpineCallsCandidateResume) {
    EXPECT_CALL(enter_path_, enter_to(&leaf_, 0, 2, &a0_)).WillOnce(Return(&a0_));
    EXPECT_CALL(candidate_, resume(_)).WillOnce([this](pud_candidate_search_context& ctx) {
        EXPECT_EQ(ctx.cursor, &a0_);
        EXPECT_FALSE(ctx.witnesses.has_value());
    });
    const std::optional<pud_candidate_search_context> out = rebaser_.rebase(
        &leaf_, 0, 2, &a0_, std::nullopt);
    ASSERT_TRUE(out.has_value());
    EXPECT_EQ(out->cursor, &a0_);
    EXPECT_FALSE(out->witnesses.has_value());
}

TEST_F(PudCandidateRebaserTest, KeptPinsStayLive) {
    pud_witness_pair pins{make_pin(&c0_, &c0_), make_pin(&c1_, &c1_)};
    EXPECT_CALL(enter_path_, enter_to(&leaf_, 0, 2, &a0_)).WillOnce(Return(&a0_));
    EXPECT_CALL(enter_path_, enter_to(&leaf_, 0, 2, &c0_)).WillOnce(Return(&c0_));
    EXPECT_CALL(enter_path_, enter_to(&leaf_, 0, 2, &c1_)).WillOnce(Return(&c1_));
    const std::optional<pud_candidate_search_context> out = rebaser_.rebase(
        &leaf_, 0, 2, &a0_, pins);
    ASSERT_TRUE(out.has_value());
    ASSERT_TRUE(out->witnesses.has_value());
    EXPECT_EQ(out->witnesses->a.current, &c0_);
    EXPECT_EQ(out->witnesses->b.current, &c1_);
}

TEST_F(PudCandidateRebaserTest, ShortPinResumesFromFailedChild) {
    pud_witness_pair pins{make_pin(&c0_, &g0_), make_pin(&c1_, &c1_)};
    EXPECT_CALL(enter_path_, enter_to(&leaf_, 0, 2, &a0_)).WillOnce(Return(&a0_));
    EXPECT_CALL(enter_path_, enter_to(&leaf_, 0, 2, &g0_)).WillOnce(Return(&c0_));
    EXPECT_CALL(enter_path_, enter_to(&leaf_, 0, 2, &c1_)).WillOnce(Return(&c1_));
    EXPECT_CALL(witness_, resume(_)).WillOnce([this](pud_witness_search_context& side) {
        EXPECT_EQ(side.current, &g0_);
        side.current = nullptr;
    }).WillOnce([this](pud_witness_search_context& side) {
        EXPECT_EQ(side.current, &c1_);
    });
    EXPECT_CALL(candidate_, resume(_)).WillOnce([this](pud_candidate_search_context& ctx) {
        ctx.witnesses->a.current = &c1_;
        ctx.witnesses->a.search_root = &c1_;
        ctx.witnesses->b.current = &c0_;
        ctx.witnesses->b.search_root = &c0_;
    });
    const std::optional<pud_candidate_search_context> out = rebaser_.rebase(
        &leaf_, 0, 2, &a0_, pins);
    ASSERT_TRUE(out.has_value());
    ASSERT_TRUE(out->witnesses.has_value());
    EXPECT_NE(out->witnesses->a.current, nullptr);
    EXPECT_NE(out->witnesses->b.current, nullptr);
}

TEST_F(PudCandidateRebaserTest, CandidateRefuteAfterPinsIsNullopt) {
    pud_witness_pair pins{make_pin(&c0_, &c0_), make_pin(&c1_, &c1_)};
    EXPECT_CALL(candidate_, resume(_)).WillOnce([](pud_candidate_search_context& ctx) {
        ctx.cursor = nullptr;
        ctx.witnesses.reset();
    });
    const std::optional<pud_candidate_search_context> out = rebaser_.rebase(
        &leaf_, 0, 2, &a0_, pins);
    EXPECT_FALSE(out.has_value());
}
