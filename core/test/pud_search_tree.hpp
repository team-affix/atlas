#ifndef PUD_SEARCH_TREE_HPP
#define PUD_SEARCH_TREE_HPP

#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>
#include <gmock/gmock.h>
#include "pud_test_node.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_global_node_id_hash.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_replay.hpp"

struct MockDescendReplay {
    MOCK_METHOD(std::optional<pud_replay>, descend, (const pud_replay&, pud_global_node_id));
};
struct MockCheckNodeLeaf {
    MOCK_METHOD(bool, check_leaf, (pud_global_node_id), (const));
};
struct MockCheckNodeRefuted {
    MOCK_METHOD(bool, check_refuted, (pud_global_node_id), (const));
};
struct MockGetChildren {
    MOCK_METHOD(const std::vector<pud_node_id>&, get_children, (pud_global_node_id));
};

inline pud_replay pud_replay_at(pud_node_id node, uint32_t frame_offset) {
    return pud_replay{.node = pud_test_node(node), .frame_offset = frame_offset, .ubs_state = {}};
}

inline std::optional<pud_replay> pud_enter_child(const pud_replay& parent, pud_global_node_id child) {
    return pud_replay{.node = child, .frame_offset = parent.frame_offset, .ubs_state = parent.ubs_state};
}

// A database tree drawn on the search mocks, in the test module: each node's children, which nodes
// are live leaves or refuted, and which children a query cannot step into. Every query steps into any
// other child.
struct pud_search_tree {
    ::testing::NiceMock<MockDescendReplay> descend;
    ::testing::NiceMock<MockCheckNodeLeaf> leaves;
    ::testing::NiceMock<MockCheckNodeRefuted> refuted;
    ::testing::NiceMock<MockGetChildren> children;
    std::unordered_map<pud_node_id, std::vector<pud_node_id>> child_lists;

    pud_search_tree() {
        ON_CALL(descend, descend(::testing::_, ::testing::_)).WillByDefault(::testing::Invoke(&pud_enter_child));
        ON_CALL(leaves, check_leaf(::testing::_)).WillByDefault(::testing::Return(false));
        ON_CALL(refuted, check_refuted(::testing::_)).WillByDefault(::testing::Return(false));
    }

    void branch(pud_node_id parent, std::vector<pud_node_id> kids) {
        child_lists[parent] = std::move(kids);
        ON_CALL(children, get_children(pud_test_node(parent))).WillByDefault(::testing::ReturnRef(child_lists[parent]));
        ON_CALL(leaves, check_leaf(pud_test_node(parent))).WillByDefault(::testing::Return(false));
    }

    void leaf(pud_node_id node) {
        child_lists[node] = {};
        ON_CALL(children, get_children(pud_test_node(node))).WillByDefault(::testing::ReturnRef(child_lists[node]));
        ON_CALL(leaves, check_leaf(pud_test_node(node))).WillByDefault(::testing::Return(true));
    }

    void refute(pud_node_id node) {
        ON_CALL(refuted, check_refuted(pud_test_node(node))).WillByDefault(::testing::Return(true));
    }

    void block(pud_node_id child) {
        ON_CALL(descend, descend(::testing::_, pud_test_node(child))).WillByDefault(::testing::Return(std::nullopt));
    }
};

#endif
