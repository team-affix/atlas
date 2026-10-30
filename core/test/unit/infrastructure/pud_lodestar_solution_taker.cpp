#include <optional>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_lodestar_solution_taker.hpp"

using ::testing::InSequence;
using ::testing::Return;

namespace {

struct MockFindSolvedLeaf {
    MOCK_METHOD(std::optional<pud_global_node_id>, find, ());
};

struct MockReadSolutionTerm {
    MOCK_METHOD(pud_solution_term, read, (pud_global_node_id));
};

struct MockNormalizeSolution {
    MOCK_METHOD(const expr*, normalize, (const expr*, pud_module_id, const pud_ubs_state&));
};

struct MockCountNodeDepth {
    MOCK_METHOD(size_t, count, (pud_global_node_id));
};

struct MockSetSolution {
    MOCK_METHOD(void, set, (const expr*, size_t));
};

struct MockEraseFrontierNode {
    MOCK_METHOD(void, erase, (pud_global_node_id));
};

struct PudLodestarSolutionTakerTest : public ::testing::Test {
    MockFindSolvedLeaf finder;
    MockReadSolutionTerm reader;
    MockNormalizeSolution normalizer;
    MockCountNodeDepth depths;
    MockSetSolution store;
    MockEraseFrontierNode frontier;
    pud_lodestar_solution_taker<
        MockFindSolvedLeaf, MockReadSolutionTerm, MockNormalizeSolution, MockCountNodeDepth, MockSetSolution,
        MockEraseFrontierNode>
        taker{finder, reader, normalizer, depths, store, frontier};
    expr head{expr::functor{2, {}}};
    expr solution{expr::functor{9, {}}};
    pud_global_node_id leaf{.module = 7, .node = 4};
    pud_solution_term term{.head = &head, .module = 3, .bindings = {}};
};

}

TEST_F(PudLodestarSolutionTakerTest, NoSolvedLeafTakesNothing) {
    EXPECT_CALL(finder, find()).WillOnce(Return(std::nullopt));
    EXPECT_CALL(reader, read).Times(0);
    EXPECT_FALSE(taker.take());
}

TEST_F(PudLodestarSolutionTakerTest, ASolvedLeafIsNormalizedStoredAndErased) {
    InSequence order;
    EXPECT_CALL(finder, find()).WillOnce(Return(leaf));
    EXPECT_CALL(reader, read(leaf)).WillOnce(Return(term));
    EXPECT_CALL(normalizer, normalize(&head, 3u, term.bindings)).WillOnce(Return(&solution));
    EXPECT_CALL(depths, count(leaf)).WillOnce(Return(2u));
    EXPECT_CALL(store, set(&solution, 2u));
    EXPECT_CALL(frontier, erase(leaf));
    EXPECT_TRUE(taker.take());
}
