#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_root_opener.hpp"

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::InSequence;
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockMakeRoot {
    MOCK_METHOD(pud_node_id, make_root, (const expr*, const std::vector<const expr*>&, uint32_t));
};
struct MockAddAxiom {
    MOCK_METHOD(void, push_back, (pud_node_id));
};
struct MockActivateRoot {
    MOCK_METHOD(bool, activate_root, (pud_node_id));
};
struct MockRetire {
    MOCK_METHOD(std::vector<pud_node_id>, retire, (const std::vector<pud_node_id>&, const std::vector<pud_node_id>&));
};
struct MockCollectChange {
    MOCK_METHOD(pud_frontier_change, collect, (const std::vector<pud_node_id>&, const std::vector<pud_node_id>&));
};

using test_root_opener_t = pud_root_opener<
    NiceMock<MockMakeRoot>, NiceMock<MockAddAxiom>, NiceMock<MockActivateRoot>, NiceMock<MockRetire>, NiceMock<MockCollectChange>>;

struct PudRootOpenerTest : public ::testing::Test {
    NiceMock<MockMakeRoot> node_maker;
    NiceMock<MockAddAxiom> axioms;
    NiceMock<MockActivateRoot> activator;
    NiceMock<MockRetire> retirer;
    NiceMock<MockCollectChange> collector;
    test_root_opener_t opener{node_maker, axioms, activator, retirer, collector};
    expr head{expr::functor{1, {}}};
    expr goal{expr::functor{2, {}}};
};

}

TEST_F(PudRootOpenerTest, AnAxiomIsMadeAndListedButNotYetActivated) {
    EXPECT_CALL(node_maker, make_root(&head, std::vector<const expr*>{&goal}, 3u)).WillOnce(Return(7));
    EXPECT_CALL(axioms, push_back(7));
    EXPECT_CALL(activator, activate_root(_)).Times(0);
    EXPECT_EQ(opener.add_axiom(rule(&head, {&goal}, 3)), 7u);
}

TEST_F(PudRootOpenerTest, AConjectureIsMadeButNeverListedAsAnAxiom) {
    EXPECT_CALL(node_maker, make_root(&head, std::vector<const expr*>{&goal}, 1u)).WillOnce(Return(8));
    EXPECT_CALL(axioms, push_back(_)).Times(0);
    EXPECT_EQ(opener.add_conjecture(&head, {&goal}, 1), 8u);
}

TEST_F(PudRootOpenerTest, OpeningActivatesEveryAddedRootInOrderThenRetiresTheRefutedOnes) {
    ON_CALL(node_maker, make_root(_, _, _)).WillByDefault(Return(7));
    opener.add_axiom(rule(&head, {}, 0));
    ON_CALL(node_maker, make_root(_, _, _)).WillByDefault(Return(8));
    opener.add_conjecture(&head, {}, 0);
    InSequence in_order;
    EXPECT_CALL(activator, activate_root(7)).WillOnce(Return(true));
    EXPECT_CALL(activator, activate_root(8)).WillOnce(Return(false));
    EXPECT_CALL(retirer, retire(IsEmpty(), ElementsAre(7u))).WillOnce(Return(std::vector<pud_node_id>{7}));
    EXPECT_CALL(collector, collect(ElementsAre(7u, 8u), ElementsAre(7u)));
    opener.open();
}

TEST_F(PudRootOpenerTest, ARootIsOpenedOnlyOnce) {
    ON_CALL(node_maker, make_root(_, _, _)).WillByDefault(Return(7));
    opener.add_axiom(rule(&head, {}, 0));
    opener.open();
    EXPECT_CALL(activator, activate_root(_)).Times(0);
    opener.open();
}
