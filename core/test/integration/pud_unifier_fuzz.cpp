#include <cstdint>
#include <random>
#include <vector>
#include <gtest/gtest.h>
#include "functor_fixture.hpp"
#include "pud_unifier_bind_map_stack.hpp"
#include "reference_pud_unify.hpp"

namespace {

enum class fuzz_node_kind { var, atom_a, atom_b, pair };

struct fuzz_term_builder {
    test_functors& functors;
    pud_unifier_bind_map_stack& stack;
    std::mt19937& rng;

    const expr* make(fuzz_node_kind kind, uint32_t var_index) {
        if (kind == fuzz_node_kind::var)
            return stack.bind_backing.pool.make_var(var_index);
        if (kind == fuzz_node_kind::atom_a)
            return stack.bind_backing.pool.make_functor(functors.id("a"), {});
        if (kind == fuzz_node_kind::atom_b)
            return stack.bind_backing.pool.make_functor(functors.id("b"), {});
        const expr* left = make(static_cast<fuzz_node_kind>(rng() % 3), var_index);
        const expr* right = make(static_cast<fuzz_node_kind>(rng() % 3), var_index + 1u);
        return stack.bind_backing.pool.make_functor(functors.id("pair"), {left, right});
    }
};

struct fuzz_pair {
    framed_expr left;
    framed_expr right;
};

fuzz_pair make_fuzz_pair(test_functors& functors, pud_unifier_bind_map_stack& stack, std::mt19937& rng) {
    fuzz_term_builder builder{functors, stack, rng};
    const uint32_t var_slot = static_cast<uint32_t>(rng() % 3u);
    const fuzz_node_kind left_kind = static_cast<fuzz_node_kind>(rng() % 4u);
    const fuzz_node_kind right_kind = static_cast<fuzz_node_kind>(rng() % 4u);
    const uint32_t left_frame = static_cast<uint32_t>(rng() % 4u);
    const uint32_t right_frame = static_cast<uint32_t>(rng() % 4u);
    const expr* left = builder.make(left_kind, var_slot);
    const expr* right = builder.make(right_kind, var_slot + 1u);
    return fuzz_pair{pud_fe(left, left_frame), pud_fe(right, right_frame)};
}

} // namespace

struct PudUnifierFuzzIntegrationTest : public ::testing::Test {
    test_functors functors;
};

TEST_F(PudUnifierFuzzIntegrationTest, RandomTermsMatchReferenceUnifier) {
    std::mt19937 rng(99u);
    for (int trial = 0; trial < 40; ++trial) {
        const auto seed = rng();
        std::mt19937 prod_rng(seed);
        std::mt19937 ref_rng(seed);
        pud_unifier_bind_map_stack prod;
        pud_unifier_bind_map_stack ref;
        const fuzz_pair prod_pair = make_fuzz_pair(functors, prod, prod_rng);
        const fuzz_pair ref_pair = make_fuzz_pair(functors, ref, ref_rng);
        EXPECT_EQ(
            prod.pud_unifier.unify(prod_pair.left, prod_pair.right),
            reference_pud_unify_terms(ref, ref_pair.left, ref_pair.right));
    }
}

TEST_F(PudUnifierFuzzIntegrationTest, HeavyVariableReuseMatchesReference) {
    std::mt19937 rng(17u);
    for (int trial = 0; trial < 30; ++trial) {
        pud_unifier_bind_map_stack prod;
        pud_unifier_bind_map_stack ref;
        const uint32_t pid = functors.id("reuse");
        std::vector<const expr*> args;
        for (int arg = 0; arg < 6; ++arg)
            args.push_back(prod.bind_backing.pool.make_var(static_cast<uint32_t>(arg % 2u)));
        const expr* left = prod.bind_backing.pool.make_functor(pid, args);
        std::vector<const expr*> mirror;
        for (int arg = 5; arg >= 0; --arg)
            mirror.push_back(prod.bind_backing.pool.make_var(static_cast<uint32_t>(arg % 2u)));
        const expr* right = prod.bind_backing.pool.make_functor(pid, mirror);
        std::vector<const expr*> ref_args;
        for (int arg = 0; arg < 6; ++arg)
            ref_args.push_back(ref.bind_backing.pool.make_var(static_cast<uint32_t>(arg % 2u)));
        const expr* ref_left = ref.bind_backing.pool.make_functor(pid, ref_args);
        std::vector<const expr*> ref_mirror;
        for (int arg = 5; arg >= 0; --arg)
            ref_mirror.push_back(ref.bind_backing.pool.make_var(static_cast<uint32_t>(arg % 2u)));
        const expr* ref_right = ref.bind_backing.pool.make_functor(pid, ref_mirror);
        const framed_expr lhs{pud_fe(left, 0)};
        const framed_expr rhs{pud_fe(right, 0)};
        const framed_expr ref_lhs{pud_fe(ref_left, 0)};
        const framed_expr ref_rhs{pud_fe(ref_right, 0)};
        EXPECT_EQ(
            prod.pud_unifier.unify(lhs, rhs),
            reference_pud_unify_terms(ref, ref_lhs, ref_rhs));
    }
}

TEST_F(PudUnifierFuzzIntegrationTest, RandomUnifySequenceMatchesReferenceAfterEachStep) {
    std::mt19937 rng(808u);
    pud_unifier_bind_map_stack prod;
    pud_unifier_bind_map_stack ref;
    const expr* prod_atoms[2] = {
        prod.bind_backing.pool.make_functor(functors.id("r"), {}),
        prod.bind_backing.pool.make_functor(functors.id("s"), {}),
    };
    const expr* ref_atoms[2] = {
        ref.bind_backing.pool.make_functor(functors.id("r"), {}),
        ref.bind_backing.pool.make_functor(functors.id("s"), {}),
    };
    for (int step = 0; step < 50; ++step) {
        const uint32_t left_var = static_cast<uint32_t>(rng() % 3u);
        const uint32_t right_var = static_cast<uint32_t>(rng() % 3u);
        const uint32_t left_frame = static_cast<uint32_t>(rng() % 5u);
        const uint32_t right_frame = static_cast<uint32_t>(rng() % 5u);
        const bool use_atom = (rng() % 4u) == 0u;
        const uint32_t atom_pick = rng() % 2u;
        framed_expr left{pud_fe(prod.bind_backing.pool.make_var(left_var), left_frame)};
        framed_expr right{pud_fe(prod.bind_backing.pool.make_var(right_var), right_frame)};
        framed_expr ref_left{pud_fe(ref.bind_backing.pool.make_var(left_var), left_frame)};
        framed_expr ref_right{pud_fe(ref.bind_backing.pool.make_var(right_var), right_frame)};
        if (use_atom) {
            right = pud_fe(prod_atoms[atom_pick], 0);
            ref_right = pud_fe(ref_atoms[atom_pick], 0);
        }
        EXPECT_EQ(
            prod.pud_unifier.unify(left, right),
            reference_pud_unify_terms(ref, ref_left, ref_right));
    }
}

TEST_F(PudUnifierFuzzIntegrationTest, RandomFrameOffsetsMatchReference) {
    std::mt19937 rng(313u);
    for (int trial = 0; trial < 35; ++trial) {
        pud_unifier_bind_map_stack prod;
        pud_unifier_bind_map_stack ref;
        const uint32_t f_id = functors.id("frame");
        const expr* left = prod.bind_backing.pool.make_functor(
            f_id, {prod.bind_backing.pool.make_var(0u), prod.bind_backing.pool.make_var(1u)});
        const expr* right = prod.bind_backing.pool.make_functor(
            f_id, {prod.bind_backing.pool.make_var(0u), prod.bind_backing.pool.make_var(1u)});
        const expr* ref_left = ref.bind_backing.pool.make_functor(
            f_id, {ref.bind_backing.pool.make_var(0u), ref.bind_backing.pool.make_var(1u)});
        const expr* ref_right = ref.bind_backing.pool.make_functor(
            f_id, {ref.bind_backing.pool.make_var(0u), ref.bind_backing.pool.make_var(1u)});
        const uint32_t left_frame = static_cast<uint32_t>(rng() % 12u);
        const uint32_t right_frame = static_cast<uint32_t>(rng() % 12u);
        EXPECT_EQ(
            prod.pud_unifier.unify(pud_fe(left, left_frame), pud_fe(right, right_frame)),
            reference_pud_unify_terms(
                ref, pud_fe(ref_left, left_frame), pud_fe(ref_right, right_frame)));
    }
}

TEST_F(PudUnifierFuzzIntegrationTest, ShrinkingFindsSmallerReproWhenMismatchDetected) {
    pud_unifier_bind_map_stack prod;
    pud_unifier_bind_map_stack ref;
    const uint32_t pid = functors.id("shrink");
    const expr* big_left = prod.bind_backing.pool.make_functor(
        pid,
        {prod.bind_backing.pool.make_functor(
             pid,
             {prod.bind_backing.pool.make_var(0u), prod.bind_backing.pool.make_functor(functors.id("a"), {})}),
         prod.bind_backing.pool.make_var(1u)});
    const expr* big_right = prod.bind_backing.pool.make_functor(
        pid,
        {prod.bind_backing.pool.make_functor(
             pid,
             {prod.bind_backing.pool.make_functor(functors.id("b"), {}), prod.bind_backing.pool.make_var(1u)}),
         prod.bind_backing.pool.make_var(0u)});
    const expr* ref_big_left = ref.bind_backing.pool.make_functor(
        pid,
        {ref.bind_backing.pool.make_functor(
             pid,
             {ref.bind_backing.pool.make_var(0u), ref.bind_backing.pool.make_functor(functors.id("a"), {})}),
         ref.bind_backing.pool.make_var(1u)});
    const expr* ref_big_right = ref.bind_backing.pool.make_functor(
        pid,
        {ref.bind_backing.pool.make_functor(
             pid,
             {ref.bind_backing.pool.make_functor(functors.id("b"), {}), ref.bind_backing.pool.make_var(1u)}),
         ref.bind_backing.pool.make_var(0u)});
    const framed_expr lhs{pud_fe(big_left, 0)};
    const framed_expr rhs{pud_fe(big_right, 0)};
    const framed_expr ref_lhs{pud_fe(ref_big_left, 0)};
    const framed_expr ref_rhs{pud_fe(ref_big_right, 0)};
    const bool prod_ok = prod.pud_unifier.unify(lhs, rhs);
    const bool ref_ok = reference_pud_unify_terms(ref, ref_lhs, ref_rhs);
    EXPECT_EQ(prod_ok, ref_ok);
    if (prod_ok != ref_ok) {
        pud_unifier_bind_map_stack mini_prod;
        pud_unifier_bind_map_stack mini_ref;
        const expr* minimal_left = mini_prod.bind_backing.pool.make_var(0u);
        const expr* minimal_right = mini_prod.bind_backing.pool.make_functor(functors.id("a"), {});
        const expr* ref_minimal_left = mini_ref.bind_backing.pool.make_var(0u);
        const expr* ref_minimal_right = mini_ref.bind_backing.pool.make_functor(functors.id("a"), {});
        EXPECT_EQ(
            mini_prod.pud_unifier.unify(pud_fe(minimal_left, 0), pud_fe(minimal_right, 0)),
            reference_pud_unify_terms(
                mini_ref, pud_fe(ref_minimal_left, 0), pud_fe(ref_minimal_right, 0)));
    }
}
