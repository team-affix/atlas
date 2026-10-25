#include <cstdint>
#include <optional>
#include <random>
#include <unordered_map>
#include <gtest/gtest.h>
#include "infrastructure/ubs_bind_map.hpp"
#include "reference_union_find.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/framed_functor.hpp"
#include "value_objects/pud_ubs_state.hpp"

namespace {

struct UbsBindMapTest : public ::testing::Test {
    ubs_bind_map bind_map{pud_ubs_state{}};
    expr leaf{expr::functor{7, {}}};
    expr other_leaf{expr::functor{8, {}}};

    std::optional<framed_expr> equate(uint32_t var_a, uint32_t var_b) {
        return bind_map.equate(bind_map.find(var_a), bind_map.find(var_b));
    }

    void instantiate(uint32_t var, framed_expr value) {
        bind_map.instantiate(bind_map.find(var), value);
    }
};

}

TEST_F(UbsBindMapTest, AFreshVarIsAnUnboundSingletonOfItsOwn) {
    EXPECT_EQ(bind_map.find(5), (ubs_class{.root = 5, .size = 1, .rep = 5, .value = std::nullopt}));
}

TEST_F(UbsBindMapTest, EquatedVarsShareOneClassRepresentedByTheLeast) {
    equate(9, 2);
    const ubs_class merged = bind_map.find(9);
    EXPECT_EQ(bind_map.find(2), merged);
    EXPECT_EQ(merged.rep, 2u);
    EXPECT_EQ(merged.size, 2u);
}

TEST_F(UbsBindMapTest, EquateArgumentOrderDoesNotChangeTheClass) {
    ubs_bind_map other{pud_ubs_state{}};
    equate(1, 2);
    other.equate(other.find(2), other.find(1));
    EXPECT_EQ(bind_map.find(1).rep, other.find(1).rep);
    EXPECT_EQ(bind_map.find(2).size, other.find(2).size);
}

TEST_F(UbsBindMapTest, EquatingClassesSumsTheirSizesAndKeepsTheLeastRepresentative) {
    equate(30, 31);
    equate(30, 32);
    equate(5, 6);
    equate(31, 6);
    for (uint32_t member : {5u, 6u, 30u, 31u, 32u}) {
        EXPECT_EQ(bind_map.find(member).rep, 5u) << member;
        EXPECT_EQ(bind_map.find(member).size, 5u) << member;
    }
}

TEST_F(UbsBindMapTest, EquatingABoundClassWithAnUnboundOneKeepsTheValue) {
    instantiate(4, framed_expr{&leaf, 3});
    EXPECT_FALSE(equate(1, 4).has_value());
    EXPECT_EQ(bind_map.find(1).value, framed_functor_from_framed_expr(framed_expr{&leaf, 3}));
    EXPECT_EQ(bind_map.find(4).rep, 1u);
}

TEST_F(UbsBindMapTest, EquatingALargerUnboundClassIntoABoundOneKeepsTheValue) {
    equate(1, 2);
    equate(1, 3);
    instantiate(9, framed_expr{&leaf, 0});
    EXPECT_FALSE(equate(1, 9).has_value());
    EXPECT_EQ(bind_map.find(9).value, framed_functor_from_framed_expr(framed_expr{&leaf, 0}));
    EXPECT_EQ(bind_map.find(2).value, framed_functor_from_framed_expr(framed_expr{&leaf, 0}));
}

TEST_F(UbsBindMapTest, EquatingTwoBoundClassesReturnsTheValueTheMergedClassDoesNotKeep) {
    instantiate(1, framed_expr{&leaf, 0});
    instantiate(2, framed_expr{&other_leaf, 0});
    const std::optional<framed_expr> displaced = equate(1, 2);
    const std::optional<framed_expr> kept = bind_map.find(2).value;
    ASSERT_TRUE(displaced.has_value());
    ASSERT_TRUE(kept.has_value());
    EXPECT_NE(displaced, kept);
    EXPECT_TRUE(displaced == (framed_expr{&leaf, 0}) || displaced == (framed_expr{&other_leaf, 0}));
    EXPECT_TRUE(kept == (framed_expr{&leaf, 0}) || kept == (framed_expr{&other_leaf, 0}));
}

TEST_F(UbsBindMapTest, InstantiateBindsEveryMemberOfTheClass) {
    equate(1, 2);
    equate(1, 3);
    instantiate(3, framed_expr{&leaf, 5});
    for (uint32_t member : {1u, 2u, 3u})
        EXPECT_EQ(bind_map.find(member).value, framed_functor_from_framed_expr(framed_expr{&leaf, 5})) << member;
}

TEST_F(UbsBindMapTest, InstantiateKeepsTheRepresentativeAndSize) {
    equate(1, 2);
    instantiate(2, framed_expr{&leaf, 0});
    EXPECT_EQ(bind_map.find(2).rep, 1u);
    EXPECT_EQ(bind_map.find(2).size, 2u);
}

TEST_F(UbsBindMapTest, AnAbsorbedBoundClassIsReadThroughItsNewRoot) {
    instantiate(4, framed_expr{&leaf, 0});
    equate(1, 2);
    equate(1, 3);
    equate(1, 4);
    EXPECT_EQ(bind_map.find(4).value, framed_functor_from_framed_expr(framed_expr{&leaf, 0}));
    EXPECT_EQ(bind_map.find(4).rep, 1u);
    EXPECT_EQ(bind_map.find(4).root, bind_map.find(1).root);
}

TEST_F(UbsBindMapTest, APersistedStateReadsBackInAFreshBindMap) {
    equate(5, 3);
    instantiate(5, framed_expr{&leaf, 4});
    const pud_ubs_state persisted = std::move(bind_map).persist();
    const ubs_bind_map reader{persisted};
    EXPECT_EQ(reader.find(5).rep, 3u);
    EXPECT_EQ(reader.find(3).value, framed_functor_from_framed_expr(framed_expr{&leaf, 4}));
    EXPECT_FALSE(reader.find(9).value.has_value());
}

TEST_F(UbsBindMapTest, EditsNeverChangeTheStateTheyStartedFrom) {
    equate(1, 2);
    const pud_ubs_state base = std::move(bind_map).persist();
    ubs_bind_map branch{base};
    branch.equate(branch.find(2), branch.find(0));
    const pud_ubs_state branched = std::move(branch).persist();
    EXPECT_EQ(ubs_bind_map{base}.find(2).rep, 1u);
    EXPECT_EQ(ubs_bind_map{branched}.find(2).rep, 0u);
    EXPECT_NE(base, branched);
}

TEST_F(UbsBindMapTest, EquivalentEditSequencesPersistEqualStates) {
    ubs_bind_map other{pud_ubs_state{}};
    equate(1, 2);
    instantiate(1, framed_expr{&leaf, 0});
    other.equate(other.find(1), other.find(2));
    other.instantiate(other.find(2), framed_expr{&leaf, 0});
    EXPECT_EQ(std::move(bind_map).persist(), std::move(other).persist());
}

TEST_F(UbsBindMapTest, RandomEditsAgreeWithAReferenceUnionFind) {
    std::mt19937 rng(2026u);
    constexpr uint32_t var_count = 40u;
    reference_variable_classes classes;
    std::unordered_map<uint32_t, framed_functor> bound_by_rep;
    for (uint32_t var = 0u; var < var_count; ++var)
        classes.touch(var);
    std::vector<expr> values;
    values.reserve(300);
    for (uint32_t step = 0; step < 300; ++step) {
        const uint32_t left = static_cast<uint32_t>(rng() % var_count);
        const uint32_t right = static_cast<uint32_t>(rng() % var_count);
        const bool bind_left = (rng() % 5u) == 0u;
        if (bind_left && !bind_map.find(left).value.has_value()) {
            values.push_back(expr{expr::functor{100u + step, {}}});
            const framed_expr value{&values.back(), 0u};
            instantiate(left, value);
            bound_by_rep[classes.min_index(left)] = framed_functor_from_framed_expr(value);
        }
        if (!bind_left && !classes.same_class(left, right)) {
            const uint32_t left_rep = classes.min_index(left);
            const uint32_t right_rep = classes.min_index(right);
            const std::optional<framed_expr> displaced = equate(left, right);
            classes.equate(left, right);
            const std::optional<framed_expr> kept = bind_map.find(left).value;
            const bool any_bound = bound_by_rep.contains(left_rep) || bound_by_rep.contains(right_rep);
            const bool both_bound = bound_by_rep.contains(left_rep) && bound_by_rep.contains(right_rep);
            EXPECT_EQ(kept.has_value(), any_bound);
            EXPECT_EQ(displaced.has_value(), both_bound);
            bound_by_rep.erase(left_rep);
            bound_by_rep.erase(right_rep);
            if (kept.has_value())
                bound_by_rep[classes.min_index(left)] = kept.value();
        }
        for (uint32_t var = 0u; var < var_count; ++var) {
            const ubs_class var_class = bind_map.find(var);
            EXPECT_EQ(var_class.rep, classes.min_index(var));
            EXPECT_EQ(var_class.size, static_cast<uint32_t>(classes.class_size(var)));
            const auto bound = bound_by_rep.find(classes.min_index(var));
            EXPECT_EQ(var_class.value, bound == bound_by_rep.end() ? std::nullopt : std::optional{bound->second});
        }
    }
}
