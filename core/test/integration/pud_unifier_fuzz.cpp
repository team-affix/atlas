#include <cstdint>
#include <random>
#include <vector>
#include <gtest/gtest.h>
#include "functor_fixture.hpp"
#include "pud_term_stack.hpp"
#include "reference_pud_unify.hpp"

namespace {

enum class fuzz_node_kind { var, atom_a, atom_b, pair };

struct fuzz_term_builder {
    test_functors& functors;
    pud_term_stack& stack;
    std::mt19937& rng;

    const expr* make(fuzz_node_kind kind, uint32_t var_index) {
        if (kind == fuzz_node_kind::var)
            return stack.pool.make_var(var_index);
        if (kind == fuzz_node_kind::atom_a)
            return stack.pool.make_functor(functors.id("a"), {});
        if (kind == fuzz_node_kind::atom_b)
            return stack.pool.make_functor(functors.id("b"), {});
        const expr* left = make(static_cast<fuzz_node_kind>(rng() % 3), var_index);
        const expr* right = make(static_cast<fuzz_node_kind>(rng() % 3), var_index + 1u);
        return stack.pool.make_functor(functors.id("pair"), {left, right});
    }
};

struct fuzz_pair {
    framed_expr left;
    framed_expr right;
};

fuzz_pair make_fuzz_pair(test_functors& functors, pud_term_stack& stack, std::mt19937& rng) {
    fuzz_term_builder builder{functors, stack, rng};
    const uint32_t var_slot = static_cast<uint32_t>(rng() % 3u);
    const fuzz_node_kind left_kind = static_cast<fuzz_node_kind>(rng() % 4u);
    const fuzz_node_kind right_kind = static_cast<fuzz_node_kind>(rng() % 4u);
    const uint32_t left_frame = static_cast<uint32_t>(rng() % 4u);
    const uint32_t right_frame = static_cast<uint32_t>(rng() % 4u);
    const expr* left = builder.make(left_kind, var_slot);
    const expr* right = builder.make(right_kind, var_slot + 1u);
    return fuzz_pair{framed_expr{left, left_frame}, framed_expr{right, right_frame}};
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
        pud_term_stack prod;
        pud_term_stack ref;
        const fuzz_pair prod_pair = make_fuzz_pair(functors, prod, prod_rng);
        const fuzz_pair ref_pair = make_fuzz_pair(functors, ref, ref_rng);
        EXPECT_EQ(
            prod.unifier.unify(
                moduled_expr{prod_pair.left.skeleton, prod_pair.left.frame_offset, 0},
                moduled_expr{prod_pair.right.skeleton, prod_pair.right.frame_offset, 0}).has_value(),
            reference_pud_unify_terms(ref, ref_pair.left, ref_pair.right));
    }
}

TEST_F(PudUnifierFuzzIntegrationTest, HeavyVariableReuseMatchesReference) {
    std::mt19937 rng(17u);
    for (int trial = 0; trial < 30; ++trial) {
        pud_term_stack prod;
        pud_term_stack ref;
        const uint32_t pid = functors.id("reuse");
        std::vector<const expr*> args;
        for (int arg = 0; arg < 6; ++arg)
            args.push_back(prod.pool.make_var(static_cast<uint32_t>(arg % 2u)));
        const expr* left = prod.pool.make_functor(pid, args);
        std::vector<const expr*> mirror;
        for (int arg = 5; arg >= 0; --arg)
            mirror.push_back(prod.pool.make_var(static_cast<uint32_t>(arg % 2u)));
        const expr* right = prod.pool.make_functor(pid, mirror);
        std::vector<const expr*> ref_args;
        for (int arg = 0; arg < 6; ++arg)
            ref_args.push_back(ref.pool.make_var(static_cast<uint32_t>(arg % 2u)));
        const expr* ref_left = ref.pool.make_functor(pid, ref_args);
        std::vector<const expr*> ref_mirror;
        for (int arg = 5; arg >= 0; --arg)
            ref_mirror.push_back(ref.pool.make_var(static_cast<uint32_t>(arg % 2u)));
        const expr* ref_right = ref.pool.make_functor(pid, ref_mirror);
        const framed_expr lhs{left, 0};
        const framed_expr rhs{right, 0};
        const framed_expr ref_lhs{ref_left, 0};
        const framed_expr ref_rhs{ref_right, 0};
        EXPECT_EQ(
            prod.unifier.unify(moduled_expr{lhs.skeleton, lhs.frame_offset, 0}, moduled_expr{rhs.skeleton, rhs.frame_offset, 0}).has_value(),
            reference_pud_unify_terms(ref, ref_lhs, ref_rhs));
    }
}

TEST_F(PudUnifierFuzzIntegrationTest, RandomUnifySequenceMatchesReferenceAfterEachStep) {
    std::mt19937 rng(808u);
    pud_term_stack prod;
    pud_term_stack ref;
    const expr* prod_atoms[2] = {
        prod.pool.make_functor(functors.id("r"), {}),
        prod.pool.make_functor(functors.id("s"), {}),
    };
    const expr* ref_atoms[2] = {
        ref.pool.make_functor(functors.id("r"), {}),
        ref.pool.make_functor(functors.id("s"), {}),
    };
    for (int step = 0; step < 50; ++step) {
        const uint32_t left_var = static_cast<uint32_t>(rng() % 3u);
        const uint32_t right_var = static_cast<uint32_t>(rng() % 3u);
        const uint32_t left_frame = static_cast<uint32_t>(rng() % 5u);
        const uint32_t right_frame = static_cast<uint32_t>(rng() % 5u);
        const bool use_atom = (rng() % 4u) == 0u;
        const uint32_t atom_pick = rng() % 2u;
        framed_expr left{prod.pool.make_var(left_var), left_frame};
        framed_expr right{prod.pool.make_var(right_var), right_frame};
        framed_expr ref_left{ref.pool.make_var(left_var), left_frame};
        framed_expr ref_right{ref.pool.make_var(right_var), right_frame};
        if (use_atom) {
            right = framed_expr{prod_atoms[atom_pick], 0};
            ref_right = framed_expr{ref_atoms[atom_pick], 0};
        }
        EXPECT_EQ(
            prod.unifier.unify(
                moduled_expr{left.skeleton, left.frame_offset, 0},
                moduled_expr{right.skeleton, right.frame_offset, 0}).has_value(),
            reference_pud_unify_terms(ref, ref_left, ref_right));
    }
}

TEST_F(PudUnifierFuzzIntegrationTest, RandomFrameOffsetsMatchReference) {
    std::mt19937 rng(313u);
    for (int trial = 0; trial < 35; ++trial) {
        pud_term_stack prod;
        pud_term_stack ref;
        const uint32_t f_id = functors.id("frame");
        const expr* left = prod.pool.make_functor(
            f_id, {prod.pool.make_var(0u), prod.pool.make_var(1u)});
        const expr* right = prod.pool.make_functor(
            f_id, {prod.pool.make_var(0u), prod.pool.make_var(1u)});
        const expr* ref_left = ref.pool.make_functor(
            f_id, {ref.pool.make_var(0u), ref.pool.make_var(1u)});
        const expr* ref_right = ref.pool.make_functor(
            f_id, {ref.pool.make_var(0u), ref.pool.make_var(1u)});
        const uint32_t left_frame = static_cast<uint32_t>(rng() % 12u);
        const uint32_t right_frame = static_cast<uint32_t>(rng() % 12u);
        EXPECT_EQ(
            prod.unifier.unify(moduled_expr{left, left_frame, 0}, moduled_expr{right, right_frame, 0}).has_value(),
            reference_pud_unify_terms(
                ref, framed_expr{ref_left, left_frame}, framed_expr{ref_right, right_frame}));
    }
}

TEST_F(PudUnifierFuzzIntegrationTest, ShrinkingFindsSmallerReproWhenMismatchDetected) {
    pud_term_stack prod;
    pud_term_stack ref;
    const uint32_t pid = functors.id("shrink");
    const expr* big_left = prod.pool.make_functor(
        pid,
        {prod.pool.make_functor(
             pid,
             {prod.pool.make_var(0u), prod.pool.make_functor(functors.id("a"), {})}),
         prod.pool.make_var(1u)});
    const expr* big_right = prod.pool.make_functor(
        pid,
        {prod.pool.make_functor(
             pid,
             {prod.pool.make_functor(functors.id("b"), {}), prod.pool.make_var(1u)}),
         prod.pool.make_var(0u)});
    const expr* ref_big_left = ref.pool.make_functor(
        pid,
        {ref.pool.make_functor(
             pid,
             {ref.pool.make_var(0u), ref.pool.make_functor(functors.id("a"), {})}),
         ref.pool.make_var(1u)});
    const expr* ref_big_right = ref.pool.make_functor(
        pid,
        {ref.pool.make_functor(
             pid,
             {ref.pool.make_functor(functors.id("b"), {}), ref.pool.make_var(1u)}),
         ref.pool.make_var(0u)});
    const framed_expr lhs{big_left, 0};
    const framed_expr rhs{big_right, 0};
    const framed_expr ref_lhs{ref_big_left, 0};
    const framed_expr ref_rhs{ref_big_right, 0};
    const bool prod_ok = prod.unifier.unify(
        moduled_expr{lhs.skeleton, lhs.frame_offset, 0}, moduled_expr{rhs.skeleton, rhs.frame_offset, 0}).has_value();
    const bool ref_ok = reference_pud_unify_terms(ref, ref_lhs, ref_rhs);
    EXPECT_EQ(prod_ok, ref_ok);
    if (prod_ok != ref_ok) {
        pud_term_stack mini_prod;
        pud_term_stack mini_ref;
        const expr* minimal_left = mini_prod.pool.make_var(0u);
        const expr* minimal_right = mini_prod.pool.make_functor(functors.id("a"), {});
        const expr* ref_minimal_left = mini_ref.pool.make_var(0u);
        const expr* ref_minimal_right = mini_ref.pool.make_functor(functors.id("a"), {});
        EXPECT_EQ(
            mini_prod.unifier.unify(moduled_expr{minimal_left, 0, 0}, moduled_expr{minimal_right, 0, 0}).has_value(),
            reference_pud_unify_terms(
                mini_ref, framed_expr{ref_minimal_left, 0}, framed_expr{ref_minimal_right, 0}));
    }
}
