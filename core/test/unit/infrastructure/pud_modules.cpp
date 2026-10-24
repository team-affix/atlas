#include <optional>
#include <stdexcept>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "pud_test_node.hpp"
#include "infrastructure/pud_modules.hpp"
#include "value_objects/expr.hpp"

using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockModuleData {
    MOCK_METHOD(pud_module_id, get_module_id, (), (const));
    MOCK_METHOD(const std::vector<pud_module_id>&, get_dependencies, (), (const));
    MOCK_METHOD(size_t, count_axioms, (), (const));
    MOCK_METHOD(pud_node_id, store_inference, (pud_inference));
    MOCK_METHOD(void, mark_refuted, (pud_node_id));
    MOCK_METHOD(const rule&, get_axiom, (pud_node_id), (const));
    MOCK_METHOD(const pud_inference&, get_inference, (pud_node_id), (const));
    MOCK_METHOD(std::optional<pud_node_id>, get_parent, (pud_node_id), (const));
    MOCK_METHOD(const std::vector<pud_node_id>&, get_children, (pud_node_id), (const));
    MOCK_METHOD(bool, check_leaf, (pud_node_id), (const));
    MOCK_METHOD(bool, check_refuted, (pud_node_id), (const));
};

struct PudModulesTest : public ::testing::Test {
    pud_modules<NiceMock<MockModuleData>> modules;
    expr head{expr::functor{1, {}}};
    rule axiom{&head, {}, 0};
    pud_inference inference{.parent = 0, .call_site = 0, .substitutions = {}, .body_goals = {}, .var_count = 0};
    std::vector<pud_node_id> children{4, 5};
};

}

TEST_F(PudModulesTest, AModulesAxiomsAreItsFirstNodes) {
    NiceMock<MockModuleData> base_data;
    std::vector<pud_module_id> base_dependencies{};
    ON_CALL(base_data, get_module_id()).WillByDefault(Return(10));
    ON_CALL(base_data, get_dependencies()).WillByDefault(ReturnRef(base_dependencies));
    ON_CALL(base_data, count_axioms()).WillByDefault(Return(3));
    modules.register_module(base_data);
    EXPECT_THAT(
        modules.list_axioms(10),
        ElementsAre(
            pud_global_node_id{.module = 10, .node = 0},
            pud_global_node_id{.module = 10, .node = 1},
            pud_global_node_id{.module = 10, .node = 2}));
}

TEST_F(PudModulesTest, AModuleWithoutDependenciesSeesOnlyItsOwnAxioms) {
    NiceMock<MockModuleData> base_data;
    std::vector<pud_module_id> base_dependencies{};
    ON_CALL(base_data, get_module_id()).WillByDefault(Return(10));
    ON_CALL(base_data, get_dependencies()).WillByDefault(ReturnRef(base_dependencies));
    ON_CALL(base_data, count_axioms()).WillByDefault(Return(2));
    modules.register_module(base_data);
    EXPECT_THAT(
        modules.list_visible_axioms(10),
        ElementsAre(pud_global_node_id{.module = 10, .node = 0}, pud_global_node_id{.module = 10, .node = 1}));
}

TEST_F(PudModulesTest, AModuleSeesItsDependenciesAxiomsBeforeItsOwn) {
    NiceMock<MockModuleData> base_data;
    std::vector<pud_module_id> base_dependencies{};
    ON_CALL(base_data, get_module_id()).WillByDefault(Return(10));
    ON_CALL(base_data, get_dependencies()).WillByDefault(ReturnRef(base_dependencies));
    ON_CALL(base_data, count_axioms()).WillByDefault(Return(2));
    NiceMock<MockModuleData> dependent_data;
    std::vector<pud_module_id> dependent_dependencies{10};
    ON_CALL(dependent_data, get_module_id()).WillByDefault(Return(20));
    ON_CALL(dependent_data, get_dependencies()).WillByDefault(ReturnRef(dependent_dependencies));
    ON_CALL(dependent_data, count_axioms()).WillByDefault(Return(1));
    modules.register_module(base_data);
    modules.register_module(dependent_data);
    EXPECT_THAT(
        modules.list_visible_axioms(20),
        ElementsAre(
            pud_global_node_id{.module = 10, .node = 0},
            pud_global_node_id{.module = 10, .node = 1},
            pud_global_node_id{.module = 20, .node = 0}));
    EXPECT_THAT(modules.list_axioms(20), ElementsAre(pud_global_node_id{.module = 20, .node = 0}));
}

TEST_F(PudModulesTest, ADependencyNeverSeesItsDependentsAxioms) {
    NiceMock<MockModuleData> base_data;
    std::vector<pud_module_id> base_dependencies{};
    ON_CALL(base_data, get_module_id()).WillByDefault(Return(10));
    ON_CALL(base_data, get_dependencies()).WillByDefault(ReturnRef(base_dependencies));
    ON_CALL(base_data, count_axioms()).WillByDefault(Return(1));
    NiceMock<MockModuleData> dependent_data;
    std::vector<pud_module_id> dependent_dependencies{10};
    ON_CALL(dependent_data, get_module_id()).WillByDefault(Return(20));
    ON_CALL(dependent_data, get_dependencies()).WillByDefault(ReturnRef(dependent_dependencies));
    ON_CALL(dependent_data, count_axioms()).WillByDefault(Return(1));
    modules.register_module(base_data);
    modules.register_module(dependent_data);
    EXPECT_THAT(modules.list_visible_axioms(10), ElementsAre(pud_global_node_id{.module = 10, .node = 0}));
}

TEST_F(PudModulesTest, IndirectDependenciesAreSeenDependenciesFirst) {
    NiceMock<MockModuleData> base_data;
    std::vector<pud_module_id> base_dependencies{};
    ON_CALL(base_data, get_module_id()).WillByDefault(Return(10));
    ON_CALL(base_data, get_dependencies()).WillByDefault(ReturnRef(base_dependencies));
    ON_CALL(base_data, count_axioms()).WillByDefault(Return(1));
    NiceMock<MockModuleData> middle_data;
    std::vector<pud_module_id> middle_dependencies{10};
    ON_CALL(middle_data, get_module_id()).WillByDefault(Return(20));
    ON_CALL(middle_data, get_dependencies()).WillByDefault(ReturnRef(middle_dependencies));
    ON_CALL(middle_data, count_axioms()).WillByDefault(Return(1));
    NiceMock<MockModuleData> top_data;
    std::vector<pud_module_id> top_dependencies{20};
    ON_CALL(top_data, get_module_id()).WillByDefault(Return(30));
    ON_CALL(top_data, get_dependencies()).WillByDefault(ReturnRef(top_dependencies));
    ON_CALL(top_data, count_axioms()).WillByDefault(Return(1));
    modules.register_module(base_data);
    modules.register_module(middle_data);
    modules.register_module(top_data);
    EXPECT_THAT(
        modules.list_visible_axioms(30),
        ElementsAre(
            pud_global_node_id{.module = 10, .node = 0},
            pud_global_node_id{.module = 20, .node = 0},
            pud_global_node_id{.module = 30, .node = 0}));
}

TEST_F(PudModulesTest, ASharedDependencyIsSeenOnce) {
    NiceMock<MockModuleData> base_data;
    std::vector<pud_module_id> base_dependencies{};
    ON_CALL(base_data, get_module_id()).WillByDefault(Return(10));
    ON_CALL(base_data, get_dependencies()).WillByDefault(ReturnRef(base_dependencies));
    ON_CALL(base_data, count_axioms()).WillByDefault(Return(1));
    NiceMock<MockModuleData> left_data;
    std::vector<pud_module_id> left_dependencies{10};
    ON_CALL(left_data, get_module_id()).WillByDefault(Return(20));
    ON_CALL(left_data, get_dependencies()).WillByDefault(ReturnRef(left_dependencies));
    ON_CALL(left_data, count_axioms()).WillByDefault(Return(1));
    NiceMock<MockModuleData> right_data;
    std::vector<pud_module_id> right_dependencies{10};
    ON_CALL(right_data, get_module_id()).WillByDefault(Return(30));
    ON_CALL(right_data, get_dependencies()).WillByDefault(ReturnRef(right_dependencies));
    ON_CALL(right_data, count_axioms()).WillByDefault(Return(1));
    NiceMock<MockModuleData> joined_data;
    std::vector<pud_module_id> joined_dependencies{20, 30};
    ON_CALL(joined_data, get_module_id()).WillByDefault(Return(40));
    ON_CALL(joined_data, get_dependencies()).WillByDefault(ReturnRef(joined_dependencies));
    ON_CALL(joined_data, count_axioms()).WillByDefault(Return(1));
    modules.register_module(base_data);
    modules.register_module(left_data);
    modules.register_module(right_data);
    modules.register_module(joined_data);
    EXPECT_THAT(
        modules.list_visible_axioms(40),
        ElementsAre(
            pud_global_node_id{.module = 10, .node = 0},
            pud_global_node_id{.module = 20, .node = 0},
            pud_global_node_id{.module = 30, .node = 0},
            pud_global_node_id{.module = 40, .node = 0}));
}

TEST_F(PudModulesTest, SiblingModulesDoNotSeeEachOther) {
    NiceMock<MockModuleData> base_data;
    std::vector<pud_module_id> base_dependencies{};
    ON_CALL(base_data, get_module_id()).WillByDefault(Return(10));
    ON_CALL(base_data, get_dependencies()).WillByDefault(ReturnRef(base_dependencies));
    ON_CALL(base_data, count_axioms()).WillByDefault(Return(1));
    NiceMock<MockModuleData> left_data;
    std::vector<pud_module_id> left_dependencies{10};
    ON_CALL(left_data, get_module_id()).WillByDefault(Return(20));
    ON_CALL(left_data, get_dependencies()).WillByDefault(ReturnRef(left_dependencies));
    ON_CALL(left_data, count_axioms()).WillByDefault(Return(1));
    NiceMock<MockModuleData> right_data;
    std::vector<pud_module_id> right_dependencies{10};
    ON_CALL(right_data, get_module_id()).WillByDefault(Return(30));
    ON_CALL(right_data, get_dependencies()).WillByDefault(ReturnRef(right_dependencies));
    ON_CALL(right_data, count_axioms()).WillByDefault(Return(1));
    modules.register_module(base_data);
    modules.register_module(left_data);
    modules.register_module(right_data);
    EXPECT_THAT(
        modules.list_visible_axioms(20),
        ElementsAre(pud_global_node_id{.module = 10, .node = 0}, pud_global_node_id{.module = 20, .node = 0}));
    EXPECT_THAT(
        modules.list_visible_axioms(30),
        ElementsAre(pud_global_node_id{.module = 10, .node = 0}, pud_global_node_id{.module = 30, .node = 0}));
}

TEST_F(PudModulesTest, AModuleWithNoAxiomsSeesItsDependencies) {
    NiceMock<MockModuleData> base_data;
    std::vector<pud_module_id> base_dependencies{};
    ON_CALL(base_data, get_module_id()).WillByDefault(Return(10));
    ON_CALL(base_data, get_dependencies()).WillByDefault(ReturnRef(base_dependencies));
    ON_CALL(base_data, count_axioms()).WillByDefault(Return(1));
    NiceMock<MockModuleData> empty_data;
    std::vector<pud_module_id> empty_dependencies{10};
    ON_CALL(empty_data, get_module_id()).WillByDefault(Return(20));
    ON_CALL(empty_data, get_dependencies()).WillByDefault(ReturnRef(empty_dependencies));
    ON_CALL(empty_data, count_axioms()).WillByDefault(Return(0));
    modules.register_module(base_data);
    modules.register_module(empty_data);
    EXPECT_THAT(modules.list_axioms(20), IsEmpty());
    EXPECT_THAT(modules.list_visible_axioms(20), ElementsAre(pud_global_node_id{.module = 10, .node = 0}));
}

TEST_F(PudModulesTest, RegisteringAModuleBeforeItsDependencyFails) {
    NiceMock<MockModuleData> dependent_data;
    std::vector<pud_module_id> dependent_dependencies{10};
    ON_CALL(dependent_data, get_module_id()).WillByDefault(Return(20));
    ON_CALL(dependent_data, get_dependencies()).WillByDefault(ReturnRef(dependent_dependencies));
    ON_CALL(dependent_data, count_axioms()).WillByDefault(Return(1));
    EXPECT_THROW(modules.register_module(dependent_data), std::logic_error);
}

TEST_F(PudModulesTest, RegisteringAModuleTwiceFails) {
    NiceMock<MockModuleData> base_data;
    std::vector<pud_module_id> base_dependencies{};
    ON_CALL(base_data, get_module_id()).WillByDefault(Return(10));
    ON_CALL(base_data, get_dependencies()).WillByDefault(ReturnRef(base_dependencies));
    ON_CALL(base_data, count_axioms()).WillByDefault(Return(1));
    modules.register_module(base_data);
    EXPECT_THROW(modules.register_module(base_data), std::logic_error);
}

TEST_F(PudModulesTest, AnInferenceIsStoredInTheNamedModuleAndNamedThere) {
    NiceMock<MockModuleData> base_data;
    std::vector<pud_module_id> base_dependencies{};
    ON_CALL(base_data, get_module_id()).WillByDefault(Return(10));
    ON_CALL(base_data, get_dependencies()).WillByDefault(ReturnRef(base_dependencies));
    ON_CALL(base_data, count_axioms()).WillByDefault(Return(1));
    NiceMock<MockModuleData> other_data;
    std::vector<pud_module_id> other_dependencies{};
    ON_CALL(other_data, get_module_id()).WillByDefault(Return(20));
    ON_CALL(other_data, get_dependencies()).WillByDefault(ReturnRef(other_dependencies));
    ON_CALL(other_data, count_axioms()).WillByDefault(Return(1));
    modules.register_module(base_data);
    modules.register_module(other_data);
    EXPECT_CALL(other_data, store_inference(inference)).WillOnce(Return(1));
    EXPECT_CALL(base_data, store_inference).Times(0);
    EXPECT_EQ(modules.store_inference(20, inference), (pud_global_node_id{.module = 20, .node = 1}));
}

TEST_F(PudModulesTest, EveryLookupReachesTheNodesOwnModuleByItsLocalId) {
    NiceMock<MockModuleData> base_data;
    std::vector<pud_module_id> base_dependencies{};
    ON_CALL(base_data, get_module_id()).WillByDefault(Return(10));
    ON_CALL(base_data, get_dependencies()).WillByDefault(ReturnRef(base_dependencies));
    ON_CALL(base_data, count_axioms()).WillByDefault(Return(1));
    NiceMock<MockModuleData> other_data;
    std::vector<pud_module_id> other_dependencies{};
    ON_CALL(other_data, get_module_id()).WillByDefault(Return(20));
    ON_CALL(other_data, get_dependencies()).WillByDefault(ReturnRef(other_dependencies));
    ON_CALL(other_data, count_axioms()).WillByDefault(Return(1));
    modules.register_module(base_data);
    modules.register_module(other_data);
    EXPECT_CALL(other_data, get_axiom(0)).WillOnce(ReturnRef(axiom));
    EXPECT_CALL(other_data, get_inference(3)).WillOnce(ReturnRef(inference));
    EXPECT_CALL(other_data, get_children(3)).WillOnce(ReturnRef(children));
    EXPECT_CALL(other_data, check_leaf(3)).WillOnce(Return(true));
    EXPECT_CALL(other_data, check_refuted(3)).WillOnce(Return(true));
    EXPECT_CALL(other_data, mark_refuted(3));
    EXPECT_EQ(&modules.get_axiom(pud_global_node_id{.module = 20, .node = 0}), &axiom);
    EXPECT_EQ(&modules.get_inference(pud_global_node_id{.module = 20, .node = 3}), &inference);
    EXPECT_EQ(&modules.get_children(pud_global_node_id{.module = 20, .node = 3}), &children);
    EXPECT_TRUE(modules.check_leaf(pud_global_node_id{.module = 20, .node = 3}));
    EXPECT_TRUE(modules.check_refuted(pud_global_node_id{.module = 20, .node = 3}));
    modules.mark_refuted(pud_global_node_id{.module = 20, .node = 3});
}

TEST_F(PudModulesTest, AParentIsInItsChildsModule) {
    NiceMock<MockModuleData> other_data;
    std::vector<pud_module_id> other_dependencies{};
    ON_CALL(other_data, get_module_id()).WillByDefault(Return(20));
    ON_CALL(other_data, get_dependencies()).WillByDefault(ReturnRef(other_dependencies));
    ON_CALL(other_data, count_axioms()).WillByDefault(Return(1));
    modules.register_module(other_data);
    ON_CALL(other_data, get_parent(3)).WillByDefault(Return(std::optional<pud_node_id>{1}));
    EXPECT_EQ(
        modules.get_parent(pud_global_node_id{.module = 20, .node = 3}),
        (std::optional<pud_global_node_id>{pud_global_node_id{.module = 20, .node = 1}}));
}

TEST_F(PudModulesTest, AnAxiomHasNoParent) {
    NiceMock<MockModuleData> other_data;
    std::vector<pud_module_id> other_dependencies{};
    ON_CALL(other_data, get_module_id()).WillByDefault(Return(20));
    ON_CALL(other_data, get_dependencies()).WillByDefault(ReturnRef(other_dependencies));
    ON_CALL(other_data, count_axioms()).WillByDefault(Return(1));
    modules.register_module(other_data);
    ON_CALL(other_data, get_parent(0)).WillByDefault(Return(std::nullopt));
    EXPECT_EQ(modules.get_parent(pud_global_node_id{.module = 20, .node = 0}), std::nullopt);
}

TEST_F(PudModulesTest, TheSameLocalIdInTwoModulesNamesTwoNodes) {
    NiceMock<MockModuleData> base_data;
    std::vector<pud_module_id> base_dependencies{};
    ON_CALL(base_data, get_module_id()).WillByDefault(Return(10));
    ON_CALL(base_data, get_dependencies()).WillByDefault(ReturnRef(base_dependencies));
    ON_CALL(base_data, count_axioms()).WillByDefault(Return(1));
    NiceMock<MockModuleData> other_data;
    std::vector<pud_module_id> other_dependencies{};
    ON_CALL(other_data, get_module_id()).WillByDefault(Return(20));
    ON_CALL(other_data, get_dependencies()).WillByDefault(ReturnRef(other_dependencies));
    ON_CALL(other_data, count_axioms()).WillByDefault(Return(1));
    modules.register_module(base_data);
    modules.register_module(other_data);
    ON_CALL(base_data, check_refuted(0)).WillByDefault(Return(true));
    ON_CALL(other_data, check_refuted(0)).WillByDefault(Return(false));
    EXPECT_TRUE(modules.check_refuted(pud_global_node_id{.module = 10, .node = 0}));
    EXPECT_FALSE(modules.check_refuted(pud_global_node_id{.module = 20, .node = 0}));
}
