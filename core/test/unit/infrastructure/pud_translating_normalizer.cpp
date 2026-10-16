#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "functor_fixture.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/pud_translating_normalizer.hpp"

using ::testing::_;
using ::testing::NiceMock;

namespace {

struct MockResolve {
    MOCK_METHOD(const expr*, resolve, (uint32_t));
};

struct PudTranslatingNormalizerTest : public ::testing::Test {
    test_functors functors;
    expr_pool exprs;
    globalizer globalize;
    NiceMock<MockResolve> resolve;
    pud_translating_normalizer<globalizer, expr_pool, expr_pool, NiceMock<MockResolve>>
        normalizer{globalize, exprs, exprs, resolve};
    std::unordered_map<uint32_t, const expr*> bound;
    std::unordered_map<uint32_t, uint32_t> translation;
    uint32_t fresh_offset = 10;

    const expr* var(uint32_t index) { return exprs.make_var(index); }
    const expr* functor(const char* name, std::vector<const expr*> args) {
        return exprs.make_functor(functors.id(name), args);
    }

    void SetUp() override {
        ON_CALL(resolve, resolve(_)).WillByDefault([&](uint32_t index) {
            auto found = bound.find(index);
            return found == bound.end() ? var(index) : found->second;
        });
    }

    const expr* normalize(const expr* skeleton, uint32_t frame_offset) {
        return normalizer.normalize(framed_expr{skeleton, frame_offset}, fresh_offset, translation);
    }
};

TEST_F(PudTranslatingNormalizerTest, UnboundRepresentativeBelowTheOffsetIsKept) {
    EXPECT_EQ(normalize(var(3), 0), var(3));
    EXPECT_TRUE(translation.empty());
}

TEST_F(PudTranslatingNormalizerTest, RepresentativeAtTheOffsetIsRenamedToTheOffset) {
    EXPECT_EQ(normalize(var(0), 10), var(10));
}

TEST_F(PudTranslatingNormalizerTest, RepresentativesAboveTheOffsetAreRenamedDenselyInFirstMetOrder) {
    EXPECT_EQ(normalize(functor("f", {var(7), var(4), var(7)}), 10), functor("f", {var(10), var(11), var(10)}));
}

TEST_F(PudTranslatingNormalizerTest, TranslationIsSharedAcrossCalls) {
    normalize(var(5), 10);
    EXPECT_EQ(normalize(functor("g", {var(5), var(6)}), 10), functor("g", {var(10), var(11)}));
}

TEST_F(PudTranslatingNormalizerTest, BoundVariableIsReplacedByItsNormalizedValue) {
    bound[2] = functor("s", {var(15)});
    EXPECT_EQ(normalize(var(2), 0), functor("s", {var(10)}));
}

TEST_F(PudTranslatingNormalizerTest, VariableMergedIntoAnOlderRepresentativeUsesThatRepresentative) {
    bound[14] = var(3);
    EXPECT_EQ(normalize(var(4), 10), var(3));
    EXPECT_TRUE(translation.empty());
}

TEST_F(PudTranslatingNormalizerTest, NestedBindingsAreFollowedAllTheWay) {
    bound[1] = functor("s", {var(2)});
    bound[2] = functor("s", {var(3)});
    bound[3] = functor("z", {});
    EXPECT_EQ(normalize(var(1), 0), functor("s", {functor("s", {functor("z", {})})}));
}

TEST_F(PudTranslatingNormalizerTest, GroundTermIsReturnedAsTheSameInternedTerm) {
    const expr* ground = functor("p", {functor("a", {}), functor("b", {})});
    EXPECT_EQ(normalize(ground, 10), ground);
}

TEST_F(PudTranslatingNormalizerTest, FrameOffsetIsAppliedToEveryVariable) {
    bound[13] = functor("a", {});
    EXPECT_EQ(normalize(functor("f", {var(3), var(1)}), 10), functor("f", {functor("a", {}), var(10)}));
}

}  // namespace
