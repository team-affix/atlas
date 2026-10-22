#include <cstdint>
#include <random>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include "pud_unifier_bind_map_stack.hpp"
#include "reference_union_find.hpp"
#include "ubs_bind_map_test_maps.hpp"

TEST(UbsBindMapInvariantsIntegrationTest, RandomEquateAndInstantiateRespectReferenceClasses) {
    pud_unifier_bind_map_stack stack;
    std::mt19937 rng(2026u);
    const uint32_t var_count = 40u;
    reference_variable_classes classes;
    std::unordered_map<uint32_t, framed_expr> bound_by_min;
    for (uint32_t var = 0u; var < var_count; ++var)
        classes.touch(var);

    for (int step = 0; step < 300; ++step) {
        const uint32_t left = static_cast<uint32_t>(rng() % var_count);
        const uint32_t right = static_cast<uint32_t>(rng() % var_count);
        const bool do_instantiate = (rng() % 5u) == 0u;

        if (do_instantiate) {
            if (stack.bind_map.find_value(left).has_value())
                continue;
            const framed_expr value{stack.bind_backing.pool.make_functor(100u + static_cast<uint32_t>(step), {}), 0u};
            stack.bind_map.instantiate(left, value);
            bound_by_min[classes.min_index(left)] = value;
        } else {
            if (classes.same_class(left, right))
                continue;
            const uint32_t left_min = classes.min_index(left);
            const uint32_t right_min = classes.min_index(right);
            const bool left_bound = bound_by_min.contains(left_min);
            const bool right_bound = bound_by_min.contains(right_min);
            stack.bind_map.equate(left, right);
            classes.equate(left, right);
            const uint32_t merged_min = classes.min_index(left);
            if (left_bound || right_bound) {
                const std::optional<framed_expr> kept = stack.bind_map.find_value(left);
                ASSERT_TRUE(kept.has_value());
                EXPECT_TRUE((left_bound && kept == bound_by_min.at(left_min))
                    || (right_bound && kept == bound_by_min.at(right_min)));
                bound_by_min.erase(left_min);
                bound_by_min.erase(right_min);
                bound_by_min[merged_min] = kept.value();
            }
        }

        for (uint32_t var = 0u; var < var_count; ++var) {
            EXPECT_EQ(stack.bind_map.find_rep(var), classes.min_index(var));
            const auto bound = bound_by_min.find(classes.min_index(var));
            if (bound == bound_by_min.end())
                EXPECT_FALSE(stack.bind_map.find_value(var).has_value());
            else
                EXPECT_EQ(stack.bind_map.find_value(var), bound->second);
            const uint32_t* size = stack.bind_backing.sizes.get_size(stack.bind_map.find(var));
            EXPECT_EQ(size ? *size : 1u, static_cast<uint32_t>(classes.class_size(var)));
            EXPECT_TRUE(reference_height_within_log_bound(stack.bind_backing.parents.map, classes, var));
        }
        EXPECT_TRUE(reference_parent_points_inside_class(stack.bind_backing.parents.map, classes));
    }
}
