// pud_axiom_adder: add_axiom(const rule&) intern’s, installs queries, patches existing leaves.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>
#include "infrastructure/pud_axiom_adder.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/om_interval.hpp"
#include "value_objects/pud_candidate_search_context.hpp"
#include "value_objects/pud_candidate_search_result.hpp"
#include "value_objects/pud_db_node.hpp"
#include "value_objects/pud_query.hpp"
#include "value_objects/pud_rule_id.hpp"
#include "value_objects/rule.hpp"

using ::testing::DoAll;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::SaveArg;
using ::testing::_;

struct MockAddAxiom {
    MOCK_METHOD(const pud_rule_id*, add_axiom, (size_t, pud_db_node), ());
};

struct MockGetNode {
    MOCK_METHOD(const pud_db_node&, get_node, (const pud_rule_id*), ());
};

struct MockAllocateChildInterval {
    MOCK_METHOD(om_interval, allocate_child_of, (const om_interval&), ());
};

struct MockReplaceLeafQueries {
    MOCK_METHOD(void, replace_leaf_queries, (const pud_rule_id*, std::vector<pud_query>), ());
};

struct MockGetLeafQueries {
    MOCK_METHOD(const std::vector<pud_query*>&, get, (const pud_rule_id*), ());
};

struct MockBindQuery {
    MOCK_METHOD(void, bind_query, (pud_query&, uint32_t), ());
};

struct MockReinit {
    MOCK_METHOD(void, reinit, (pud_query&, uint32_t), ());
};

struct MockResumeCandidateSearch {
    MOCK_METHOD(pud_candidate_search_result, resume, (pud_candidate_search_context&), ());
};

struct MockOrderedRoots {
    MOCK_METHOD(std::vector<const pud_rule_id*>, ordered_roots, (), ());
};

struct MockOrderedLeaves {
    MOCK_METHOD(std::vector<const pud_rule_id*>, ordered_leaves, (), ());
};

struct MockWatch {
    MOCK_METHOD(void, watch, (const pud_rule_id*, pud_query*), ());
};

using test_adder_t = pud_axiom_adder<
    NiceMock<MockAddAxiom>,
    NiceMock<MockGetNode>,
    NiceMock<MockAllocateChildInterval>,
    NiceMock<MockReplaceLeafQueries>,
    NiceMock<MockGetLeafQueries>,
    NiceMock<MockBindQuery>,
    NiceMock<MockReinit>,
    NiceMock<MockResumeCandidateSearch>,
    NiceMock<MockOrderedRoots>,
    NiceMock<MockOrderedLeaves>,
    NiceMock<MockWatch>>;

struct PudAxiomAdderTest : public ::testing::Test {
    PudAxiomAdderTest()
        : open_(10)
        , close_(40)
        , nested_open_(15)
        , nested_close_(20)
        , interval_{om_label(&open_), om_label(&close_)}
        , nested_{om_label(&nested_open_), om_label(&nested_close_)}
        , head_{expr::functor{1, {}}}
        , body_{expr::functor{2, {}}}
        , axiom0_{pud_rule_id::axiom{0}}
        , axiom1_{pud_rule_id::axiom{1}}
        , axiom0_node_{interval_, {{0, &head_}}, {&body_}, 1}
        , axiom1_node_{interval_, {{0, &head_}}, {}, 1}
        , axiom0_query_{nested_, &body_, {pud_candidate_search_context{&axiom0_, {}}}, 1}
        , axiom0_query_ptrs_{&axiom0_query_}
        , empty_query_ptrs_{}
        , adder_(add_axiom_, get_node_, allocate_child_, replace_queries_, get_queries_,
                 bind_query_, reinit_, resume_, ordered_roots_, ordered_leaves_, watch_) {
        ON_CALL(get_node_, get_node(&axiom0_)).WillByDefault(ReturnRef(axiom0_node_));
        ON_CALL(get_node_, get_node(&axiom1_)).WillByDefault(ReturnRef(axiom1_node_));
        ON_CALL(get_queries_, get(&axiom0_)).WillByDefault(ReturnRef(axiom0_query_ptrs_));
        ON_CALL(get_queries_, get(&axiom1_)).WillByDefault(ReturnRef(empty_query_ptrs_));
        ON_CALL(allocate_child_, allocate_child_of(_)).WillByDefault(Return(nested_));
        ON_CALL(add_axiom_, add_axiom(0, _)).WillByDefault(Return(&axiom0_));
        ON_CALL(add_axiom_, add_axiom(1, _)).WillByDefault(Return(&axiom1_));
        ON_CALL(ordered_roots_, ordered_roots())
            .WillByDefault(Return(std::vector<const pud_rule_id*>{&axiom0_}));
        ON_CALL(ordered_leaves_, ordered_leaves())
            .WillByDefault(Return(std::vector<const pud_rule_id*>{&axiom0_}));
        ON_CALL(resume_, resume(_)).WillByDefault(Return(
            pud_candidate_search_result{pud_candidate_search_result::self_witness{}}));
    }

    uint64_t open_;
    uint64_t close_;
    uint64_t nested_open_;
    uint64_t nested_close_;
    om_interval interval_;
    om_interval nested_;
    expr head_;
    expr body_;
    pud_rule_id axiom0_;
    pud_rule_id axiom1_;
    pud_db_node axiom0_node_;
    pud_db_node axiom1_node_;
    pud_query axiom0_query_;
    std::vector<pud_query*> axiom0_query_ptrs_;
    std::vector<pud_query*> empty_query_ptrs_;
    NiceMock<MockAddAxiom> add_axiom_;
    NiceMock<MockGetNode> get_node_;
    NiceMock<MockAllocateChildInterval> allocate_child_;
    NiceMock<MockReplaceLeafQueries> replace_queries_;
    NiceMock<MockGetLeafQueries> get_queries_;
    NiceMock<MockBindQuery> bind_query_;
    NiceMock<MockReinit> reinit_;
    NiceMock<MockResumeCandidateSearch> resume_;
    NiceMock<MockOrderedRoots> ordered_roots_;
    NiceMock<MockOrderedLeaves> ordered_leaves_;
    NiceMock<MockWatch> watch_;
    test_adder_t adder_;
};

TEST_F(PudAxiomAdderTest, AddAxiomPacksHeadBodyAndLvcAndReturnsId) {
    pud_db_node packed{interval_, {}, {}, 0};
    EXPECT_CALL(add_axiom_, add_axiom(0, _)).WillOnce(DoAll(SaveArg<1>(&packed), Return(&axiom0_)));

    const pud_rule_id* id = adder_.add_axiom(rule{&head_, {&body_}, 3});
    EXPECT_EQ(id, &axiom0_);
    ASSERT_EQ(packed.added_unifications.size(), 1u);
    EXPECT_EQ(packed.added_unifications[0].var_idx, 0u);
    EXPECT_EQ(packed.added_unifications[0].value, &head_);
    ASSERT_EQ(packed.added_body_goals.size(), 1u);
    EXPECT_EQ(packed.added_body_goals[0], &body_);
    EXPECT_EQ(packed.lvc, 3u);
}

TEST_F(PudAxiomAdderTest, AddAxiomInstallsOneQueryPerBodyGoal) {
    std::vector<pud_query> installed;
    EXPECT_CALL(replace_queries_, replace_leaf_queries(&axiom0_, _))
        .WillOnce(SaveArg<1>(&installed));
    EXPECT_CALL(resume_, resume(_));
    EXPECT_CALL(watch_, watch(&axiom0_, &axiom0_query_));

    adder_.add_axiom(rule{&head_, {&body_}, 1});

    ASSERT_EQ(installed.size(), 1u);
    EXPECT_EQ(installed[0].body_goal, &body_);
    ASSERT_EQ(installed[0].axiom_contexts.size(), 1u);
    EXPECT_EQ(installed[0].axiom_contexts[0].cursor, &axiom0_);
    EXPECT_EQ(installed[0].frame_offset, 1u);
}

TEST_F(PudAxiomAdderTest, SecondAxiomAppendsContextOnExistingLeafQuery) {
    adder_.add_axiom(rule{&head_, {&body_}, 1});
    ASSERT_EQ(axiom0_query_.axiom_contexts.size(), 1u);

    ON_CALL(ordered_roots_, ordered_roots())
        .WillByDefault(Return(std::vector<const pud_rule_id*>{&axiom0_, &axiom1_}));
    ON_CALL(ordered_leaves_, ordered_leaves())
        .WillByDefault(Return(std::vector<const pud_rule_id*>{&axiom0_, &axiom1_}));

    adder_.add_axiom(rule{&head_, {}, 1});

    ASSERT_EQ(axiom0_query_.axiom_contexts.size(), 2u);
    EXPECT_EQ(axiom0_query_.axiom_contexts[1].cursor, &axiom1_);
}

TEST_F(PudAxiomAdderTest, EntryIdxIncrementsOnEachAdd) {
    EXPECT_CALL(add_axiom_, add_axiom(0, _)).WillOnce(Return(&axiom0_));
    EXPECT_CALL(add_axiom_, add_axiom(1, _)).WillOnce(Return(&axiom1_));
    ON_CALL(ordered_roots_, ordered_roots())
        .WillByDefault(Return(std::vector<const pud_rule_id*>{&axiom0_, &axiom1_}));
    ON_CALL(ordered_leaves_, ordered_leaves())
        .WillByDefault(Return(std::vector<const pud_rule_id*>{&axiom0_, &axiom1_}));

    EXPECT_EQ(adder_.add_axiom(rule{&head_, {&body_}, 1}), &axiom0_);
    EXPECT_EQ(adder_.add_axiom(rule{&head_, {}, 1}), &axiom1_);
}
