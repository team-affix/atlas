#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_node_depth_seeder.hpp"

using ::testing::NiceMock;
using ::testing::ReturnRef;

namespace {

pud_global_node_id node(pud_node_id id) {
    return pud_global_node_id{.module = 7, .node = id};
}

struct MockListAxioms {
    MOCK_METHOD(const std::vector<pud_global_node_id>&, list_axioms, (pud_module_id), (const));
};
struct MockGetChildren {
    MOCK_METHOD(const std::vector<pud_node_id>&, get_children, (pud_global_node_id), (const));
};
struct MockInsertNodeDepth {
    MOCK_METHOD(void, insert, (pud_global_node_id, size_t));
};

// Module 7: axioms 0 and 1; axiom 0 has children 2 and 3; node 3 has child 4.
struct PudNodeDepthSeederTest : public ::testing::Test {
    NiceMock<MockListAxioms> axioms;
    NiceMock<MockGetChildren> children;
    NiceMock<MockInsertNodeDepth> depths;
    pud_node_depth_seeder<NiceMock<MockListAxioms>, NiceMock<MockGetChildren>, NiceMock<MockInsertNodeDepth>>
        seeder{axioms, children, depths};
    std::vector<pud_global_node_id> module_axioms{node(0), node(1)};
    std::vector<pud_node_id> under_axiom{2, 3};
    std::vector<pud_node_id> under_three{4};
    std::vector<pud_node_id> none;

    void SetUp() override {
        ON_CALL(axioms, list_axioms(7)).WillByDefault(ReturnRef(module_axioms));
        ON_CALL(children, get_children(::testing::_)).WillByDefault(ReturnRef(none));
        ON_CALL(children, get_children(node(0))).WillByDefault(ReturnRef(under_axiom));
        ON_CALL(children, get_children(node(3))).WillByDefault(ReturnRef(under_three));
    }
};

}

TEST_F(PudNodeDepthSeederTest, AxiomsAreAtDepthZeroAndEachInferenceIsOneDeeperThanItsParent) {
    EXPECT_CALL(depths, insert(node(0), 0u));
    EXPECT_CALL(depths, insert(node(1), 0u));
    EXPECT_CALL(depths, insert(node(2), 1u));
    EXPECT_CALL(depths, insert(node(3), 1u));
    EXPECT_CALL(depths, insert(node(4), 2u));
    seeder.seed(7);
}
