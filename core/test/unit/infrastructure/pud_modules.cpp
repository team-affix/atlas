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
    MOCK_METHOD(pud_node_id, store_inference, (pud_node_delta));
    MOCK_METHOD(void, mark_refuted, (pud_node_id));
    MOCK_METHOD(const rule&, get_axiom, (pud_node_id), (const));
    MOCK_METHOD(const pud_node_delta&, get_inference, (pud_node_id), (const));
    MOCK_METHOD(std::optional<pud_node_id>, get_parent, (pud_node_id), (const));
    MOCK_METHOD(const std::vector<pud_node_id>&, get_children, (pud_node_id), (const));
    MOCK_METHOD(bool, check_leaf, (pud_node_id), (const));
    MOCK_METHOD(bool, check_refuted, (pud_node_id), (const));
};

// A module drawn on a mock: its id, its direct dependencies and how many axioms it has.
struct drawn_module {
    NiceMock<MockModuleData> data;
    std::vector<pud_module_id> dependencies;

    drawn_module(pud_module_id id, std::vector<pud_module_id> depends_on, size_t axiom_count)
        : dependencies(std::move(depends_on)) {
        ON_CALL(data, get_module_id()).WillByDefault(Return(id));
        ON_CALL(data, get_dependencies()).WillByDefault(ReturnRef(dependencies));
        ON_CALL(data, count_axioms()).WillByDefault(Return(axiom_count));
    }
};

struct PudModulesTest : public ::testing::Test {
    pud_modules<NiceMock<MockModuleData>> modules;
    expr head{expr::functor{1, {}}};
    rule axiom{&head, {}, 0};
    pud_node_delta inference{.parent = 0, .call_site = 0, .substitutions = {}, .body_goals = {}, .var_count = 0};
    std::vector<pud_node_id> children{4, 5};
};

pud_global_node_id at(pud_module_id module, pud_node_id node) {
    return pud_global_node_id{.module = module, .node = node};
}

}

TEST_F(PudModulesTest, AModulesAxiomsAreItsFirstNodes) {
    drawn_module base{10, {}, 3};
    modules.register_module(base.data);
    EXPECT_THAT(modules.list_axioms(10), ElementsAre(at(10, 0), at(10, 1), at(10, 2)));
}

TEST_F(PudModulesTest, AModuleWithoutDependenciesSeesOnlyItsOwnAxioms) {
    drawn_module base{10, {}, 2};
    modules.register_module(base.data);
    EXPECT_THAT(modules.list_visible_axioms(10), ElementsAre(at(10, 0), at(10, 1)));
}

TEST_F(PudModulesTest, AModuleSeesItsDependenciesAxiomsBeforeItsOwn) {
    drawn_module base{10, {}, 2};
    drawn_module dependent{20, {10}, 1};
    modules.register_module(base.data);
    modules.register_module(dependent.data);
    EXPECT_THAT(modules.list_visible_axioms(20), ElementsAre(at(10, 0), at(10, 1), at(20, 0)));
    EXPECT_THAT(modules.list_axioms(20), ElementsAre(at(20, 0)));
}

TEST_F(PudModulesTest, ADependencyNeverSeesItsDependentsAxioms) {
    drawn_module base{10, {}, 1};
    drawn_module dependent{20, {10}, 1};
    modules.register_module(base.data);
    modules.register_module(dependent.data);
    EXPECT_THAT(modules.list_visible_axioms(10), ElementsAre(at(10, 0)));
}

TEST_F(PudModulesTest, IndirectDependenciesAreSeenDependenciesFirst) {
    drawn_module base{10, {}, 1};
    drawn_module middle{20, {10}, 1};
    drawn_module top{30, {20}, 1};
    modules.register_module(base.data);
    modules.register_module(middle.data);
    modules.register_module(top.data);
    EXPECT_THAT(modules.list_visible_axioms(30), ElementsAre(at(10, 0), at(20, 0), at(30, 0)));
}

TEST_F(PudModulesTest, ASharedDependencyIsSeenOnce) {
    drawn_module base{10, {}, 1};
    drawn_module left{20, {10}, 1};
    drawn_module right{30, {10}, 1};
    drawn_module joined{40, {20, 30}, 1};
    modules.register_module(base.data);
    modules.register_module(left.data);
    modules.register_module(right.data);
    modules.register_module(joined.data);
    EXPECT_THAT(modules.list_visible_axioms(40), ElementsAre(at(10, 0), at(20, 0), at(30, 0), at(40, 0)));
}

TEST_F(PudModulesTest, SiblingModulesDoNotSeeEachOther) {
    drawn_module base{10, {}, 1};
    drawn_module left{20, {10}, 1};
    drawn_module right{30, {10}, 1};
    modules.register_module(base.data);
    modules.register_module(left.data);
    modules.register_module(right.data);
    EXPECT_THAT(modules.list_visible_axioms(20), ElementsAre(at(10, 0), at(20, 0)));
    EXPECT_THAT(modules.list_visible_axioms(30), ElementsAre(at(10, 0), at(30, 0)));
}

TEST_F(PudModulesTest, AModuleWithNoAxiomsSeesItsDependencies) {
    drawn_module base{10, {}, 1};
    drawn_module empty{20, {10}, 0};
    modules.register_module(base.data);
    modules.register_module(empty.data);
    EXPECT_THAT(modules.list_axioms(20), IsEmpty());
    EXPECT_THAT(modules.list_visible_axioms(20), ElementsAre(at(10, 0)));
}

TEST_F(PudModulesTest, RegisteringAModuleBeforeItsDependencyFails) {
    drawn_module dependent{20, {10}, 1};
    EXPECT_THROW(modules.register_module(dependent.data), std::logic_error);
}

TEST_F(PudModulesTest, RegisteringAModuleTwiceFails) {
    drawn_module base{10, {}, 1};
    modules.register_module(base.data);
    EXPECT_THROW(modules.register_module(base.data), std::logic_error);
}

TEST_F(PudModulesTest, AnInferenceIsStoredInTheNamedModuleAndNamedThere) {
    drawn_module base{10, {}, 1};
    drawn_module other{20, {}, 1};
    modules.register_module(base.data);
    modules.register_module(other.data);
    EXPECT_CALL(other.data, store_inference(inference)).WillOnce(Return(1));
    EXPECT_CALL(base.data, store_inference).Times(0);
    EXPECT_EQ(modules.store_inference(20, inference), at(20, 1));
}

TEST_F(PudModulesTest, EveryLookupReachesTheNodesOwnModuleByItsLocalId) {
    drawn_module base{10, {}, 1};
    drawn_module other{20, {}, 1};
    modules.register_module(base.data);
    modules.register_module(other.data);
    EXPECT_CALL(other.data, get_axiom(0)).WillOnce(ReturnRef(axiom));
    EXPECT_CALL(other.data, get_inference(3)).WillOnce(ReturnRef(inference));
    EXPECT_CALL(other.data, get_children(3)).WillOnce(ReturnRef(children));
    EXPECT_CALL(other.data, check_leaf(3)).WillOnce(Return(true));
    EXPECT_CALL(other.data, check_refuted(3)).WillOnce(Return(true));
    EXPECT_CALL(other.data, mark_refuted(3));
    EXPECT_EQ(&modules.get_axiom(at(20, 0)), &axiom);
    EXPECT_EQ(&modules.get_inference(at(20, 3)), &inference);
    EXPECT_EQ(&modules.get_children(at(20, 3)), &children);
    EXPECT_TRUE(modules.check_leaf(at(20, 3)));
    EXPECT_TRUE(modules.check_refuted(at(20, 3)));
    modules.mark_refuted(at(20, 3));
}

TEST_F(PudModulesTest, AParentIsInItsChildsModule) {
    drawn_module other{20, {}, 1};
    modules.register_module(other.data);
    ON_CALL(other.data, get_parent(3)).WillByDefault(Return(std::optional<pud_node_id>{1}));
    EXPECT_EQ(modules.get_parent(at(20, 3)), std::optional<pud_global_node_id>{at(20, 1)});
}

TEST_F(PudModulesTest, AnAxiomHasNoParent) {
    drawn_module other{20, {}, 1};
    modules.register_module(other.data);
    ON_CALL(other.data, get_parent(0)).WillByDefault(Return(std::nullopt));
    EXPECT_EQ(modules.get_parent(at(20, 0)), std::nullopt);
}

TEST_F(PudModulesTest, TheSameLocalIdInTwoModulesNamesTwoNodes) {
    drawn_module base{10, {}, 1};
    drawn_module other{20, {}, 1};
    modules.register_module(base.data);
    modules.register_module(other.data);
    ON_CALL(base.data, check_refuted(0)).WillByDefault(Return(true));
    ON_CALL(other.data, check_refuted(0)).WillByDefault(Return(false));
    EXPECT_TRUE(modules.check_refuted(at(10, 0)));
    EXPECT_FALSE(modules.check_refuted(at(20, 0)));
}
