#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include "pud_unifier_bind_map_stack.hpp"
#include "reference_union_find.hpp"
#include "ubs_bind_map_test_maps.hpp"

namespace {

uint32_t bind_map_rep_index(pud_unifier_bind_map_stack& stack, uint32_t var) {
    const expr* resolved = stack.bind_map.resolve(var);
    return std::get<expr::var>(resolved->content).index;
}

uint32_t pool_find_root(const std::unordered_map<uint32_t, uint32_t>& parents, uint32_t var) {
    while (parents.find(var) != parents.end())
        var = parents.at(var);
    return var;
}

} // namespace

struct UbsBindMapInvariantsIntegrationTest : public ::testing::Test {
    pud_unifier_bind_map_stack stack;
};

TEST_F(UbsBindMapInvariantsIntegrationTest, ThousandMemberClassSharesIdenticalRepresentativePointer) {
    for (uint32_t member = 1u; member < 1000u; ++member)
        stack.bind_map.equate(0u, member);
    const expr* representative = stack.bind_map.resolve(0u);
    for (uint32_t member = 0u; member < 1000u; ++member)
        EXPECT_EQ(stack.bind_map.resolve(member), representative);
}

TEST_F(UbsBindMapInvariantsIntegrationTest, ResolveAfterInstantiateOnDifferentMemberReturnsFunctor) {
    for (uint32_t member = 1u; member < 20u; ++member)
        stack.bind_map.equate(0u, member);
    const expr* bound = stack.bind_backing.pool.make_functor(42u, {});
    stack.bind_map.instantiate(17u, bound);
    const expr* via_root = stack.bind_map.resolve(0u);
    const expr* via_member = stack.bind_map.resolve(17u);
    EXPECT_EQ(via_root, bound);
    EXPECT_EQ(via_member, bound);
    EXPECT_NE(via_root, stack.bind_backing.pool.make_var(0u));
}

TEST_F(UbsBindMapInvariantsIntegrationTest, InstantiateThenResolveHundredMembersSharePointer) {
    for (uint32_t member = 1u; member < 100u; ++member)
        stack.bind_map.equate(0u, member);
    const expr* bound = stack.bind_backing.pool.make_functor(7u, {});
    stack.bind_map.instantiate(50u, bound);
    for (uint32_t member = 0u; member < 100u; ++member)
        EXPECT_EQ(stack.bind_map.resolve(member), bound);
}

TEST_F(UbsBindMapInvariantsIntegrationTest, InstantiateWhenRepIsCallerVarBindsCallerIndex) {
    const uint32_t caller = stack.globalize.globalize(4u, 0u);
    const uint32_t callee = stack.globalize.globalize(0u, 3u);
    stack.bind_map.equate(caller, callee);
    const expr* ground = stack.bind_backing.pool.make_functor(11u, {});
    stack.bind_map.instantiate(callee, ground);
    EXPECT_EQ(stack.bind_map.resolve(caller), ground);
    EXPECT_EQ(stack.bind_map.resolve(callee), ground);
}

TEST_F(UbsBindMapInvariantsIntegrationTest, RandomEquateAndInstantiateRespectReferenceClasses) {
    std::mt19937 rng(2026u);
    const uint32_t var_count = 40u;
    reference_variable_classes classes;
    for (uint32_t var = 0u; var < var_count; ++var)
        classes.touch(var);

    for (int step = 0; step < 300; ++step) {
        const bool do_instantiate = (rng() % 5u) == 0u;
        const uint32_t left = static_cast<uint32_t>(rng() % var_count);
        const uint32_t right = static_cast<uint32_t>(rng() % var_count);

        if (do_instantiate) {
            const expr* resolved = stack.bind_map.resolve(left);
            const bool unbound = std::holds_alternative<expr::var>(resolved->content);
            if (!unbound)
                continue;
            const expr* ground = stack.bind_backing.pool.make_functor(
                100u + static_cast<uint32_t>(step % 50u), {});
            stack.bind_map.instantiate(left, ground);
            continue;
        }

        const expr* left_resolved = stack.bind_map.resolve(left);
        const expr* right_resolved = stack.bind_map.resolve(right);
        const bool left_unbound = std::holds_alternative<expr::var>(left_resolved->content);
        const bool right_unbound = std::holds_alternative<expr::var>(right_resolved->content);
        if (!left_unbound || !right_unbound)
            continue;

        stack.bind_map.equate(left, right);
        classes.equate(left, right);

        for (uint32_t var = 0u; var < var_count; ++var) {
            const expr* resolved = stack.bind_map.resolve(var);
            if (!std::holds_alternative<expr::var>(resolved->content))
                continue;
            EXPECT_EQ(bind_map_rep_index(stack, var), classes.min_index(var));
            const expr* rep = stack.bind_map.resolve(var);
            for (uint32_t other = 0u; other < var_count; ++other) {
                if (!classes.same_class(var, other))
                    continue;
                EXPECT_EQ(stack.bind_map.resolve(other), rep);
            }
        }

        for (uint32_t var = 0u; var < var_count; ++var) {
            const uint32_t root = pool_find_root(stack.bind_backing.parents, var);
            const auto size_entry = stack.bind_backing.sizes.find(root);
            const uint32_t recorded = size_entry == stack.bind_backing.sizes.end()
                ? 1u
                : size_entry->second;
            EXPECT_EQ(recorded, static_cast<uint32_t>(classes.class_size(var)));
        }

        for (uint32_t var = 0u; var < var_count; ++var) {
            EXPECT_TRUE(reference_height_within_log_bound(
                stack.bind_backing.parents, classes, var));
        }
        EXPECT_TRUE(reference_parent_points_inside_class(stack.bind_backing.parents, classes));
    }
}
