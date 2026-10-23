#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/pud_changed_class_finder.hpp"
#include "infrastructure/ubs_bind_map.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_ubs_state.hpp"

namespace {

struct PudChangedClassFinderTest : public ::testing::Test {
    pud_changed_class_finder finder;
    expr leaf{expr::functor{7, {}}};

    pud_ubs_state equate(const pud_ubs_state& state, uint32_t var_a, uint32_t var_b) {
        ubs_bind_map bind_map{state};
        bind_map.equate(bind_map.find(var_a), bind_map.find(var_b));
        return std::move(bind_map).persist();
    }

    pud_ubs_state instantiate(const pud_ubs_state& state, uint32_t var) {
        ubs_bind_map bind_map{state};
        bind_map.instantiate(bind_map.find(var), framed_expr{&leaf, 0});
        return std::move(bind_map).persist();
    }
};

}

TEST_F(PudChangedClassFinderTest, AnUneditedStateHasNoChangedClasses) {
    const pud_ubs_state state = equate(pud_ubs_state{}, 1, 2);
    EXPECT_TRUE(finder.find_changed_classes(state, state).empty());
}

TEST_F(PudChangedClassFinderTest, EquatingTwoSingletonsChangesBothAsTheyWere) {
    const pud_ubs_state before = pud_ubs_state{};
    const std::vector<ubs_class> changed = finder.find_changed_classes(before, equate(before, 4, 2));
    EXPECT_EQ(changed, (std::vector<ubs_class>{
        ubs_class{.root = 2, .size = 1, .rep = 2, .value = std::nullopt},
        ubs_class{.root = 4, .size = 1, .rep = 4, .value = std::nullopt},
    }));
}

TEST_F(PudChangedClassFinderTest, AChangedClassIsReportedAsItWasBefore) {
    const pud_ubs_state before = equate(pud_ubs_state{}, 1, 3);
    const pud_ubs_state after = instantiate(before, 3);
    const std::vector<ubs_class> changed = finder.find_changed_classes(before, after);
    ASSERT_EQ(changed.size(), 1u);
    EXPECT_EQ(changed.front().rep, 1u);
    EXPECT_EQ(changed.front().size, 2u);
    EXPECT_FALSE(changed.front().value.has_value());
}

TEST_F(PudChangedClassFinderTest, ClassesTheEditsDidNotTouchAreNotReported) {
    const pud_ubs_state before = equate(equate(pud_ubs_state{}, 1, 2), 5, 6);
    const std::vector<ubs_class> changed = finder.find_changed_classes(before, instantiate(before, 9));
    EXPECT_EQ(changed, (std::vector<ubs_class>{ubs_class{.root = 9, .size = 1, .rep = 9, .value = std::nullopt}}));
}
