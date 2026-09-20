// pud_unfolder: yields unit/refuted from dirty leaves; never nested unfold.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <unordered_map>
#include <variant>
#include <vector>
#include "infrastructure/pud_unfolder.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/om_interval.hpp"
#include "value_objects/pud_candidate_search_context.hpp"
#include "value_objects/pud_candidate_search_result.hpp"
#include "value_objects/pud_db_node.hpp"
#include "value_objects/pud_forced_unfold.hpp"
#include "value_objects/pud_query.hpp"
#include "value_objects/pud_rule_id.hpp"

using ::testing::ElementsAre;
using ::testing::InSequence;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::_;

struct MockGetNode {
    MOCK_METHOD(const pud_db_node&, get_node, (const pud_rule_id*), ());
};

struct MockUnifyCallee {
    MOCK_METHOD(bool, unify_callee,
                (pud_query&, const pud_rule_id*, (std::vector<uint32_t>&), om_interval&), ());
};

struct MockNormalize {
    MOCK_METHOD(const expr*, normalize,
                (pud_query&, om_interval, framed_expr, uint32_t,
                 (std::unordered_map<uint32_t, uint32_t>&)), ());
};

struct MockMakeVar {
    MOCK_METHOD(const expr*, make_var, (uint32_t), ());
};

struct MockAddInference {
    MOCK_METHOD(const pud_rule_id*, add_inference,
                (const pud_rule_id*, size_t, const pud_rule_id*, pud_db_node), ());
};

struct MockLinkChildren {
    MOCK_METHOD(void, link_children,
                (const pud_rule_id*, (const std::vector<const pud_rule_id*>&)), ());
};

struct MockEffectiveBody {
    MOCK_METHOD((std::vector<const expr*>), effective_body, (const pud_rule_id*), ());
};

struct MockGetLeafQueries {
    MOCK_METHOD(const std::vector<pud_query*>&, get, (const pud_rule_id*), ());
};

struct MockClearLeafQueries {
    MOCK_METHOD(void, clear, (const pud_rule_id*), ());
};

struct MockForkChild {
    MOCK_METHOD(void, fork_child,
                (const pud_rule_id*, (const std::vector<pud_query>&)), ());
};

struct MockInvalidateLeaf {
    MOCK_METHOD(void, invalidate_leaf, (const pud_rule_id*), ());
};

struct MockResumeCandidateSearch {
    MOCK_METHOD(pud_candidate_search_result, resume,
                (pud_query&, pud_candidate_search_context&), ());
};

struct MockTakeDirtyLeaves {
    MOCK_METHOD((std::vector<const pud_rule_id*>), take_dirty_leaves, (), ());
};

using test_unfolder_t = pud_unfolder<
    NiceMock<MockGetNode>,
    NiceMock<MockUnifyCallee>,
    NiceMock<MockNormalize>,
    NiceMock<MockMakeVar>,
    NiceMock<MockAddInference>,
    NiceMock<MockLinkChildren>,
    NiceMock<MockEffectiveBody>,
    NiceMock<MockGetLeafQueries>,
    NiceMock<MockClearLeafQueries>,
    NiceMock<MockForkChild>,
    NiceMock<MockInvalidateLeaf>,
    NiceMock<MockResumeCandidateSearch>,
    NiceMock<MockTakeDirtyLeaves>>;

struct PudUnfolderTest : public ::testing::Test {
    PudUnfolderTest()
        : open_(10)
        , close_(40)
        , nested_open_(15)
        , nested_close_(20)
        , interval_{om_label(&open_), om_label(&close_)}
        , nested_{om_label(&nested_open_), om_label(&nested_close_)}
        , body_{expr::functor{1, {}}}
        , var0_{expr::var{0}}
        , leaf_{pud_rule_id::axiom{0}}
        , callee_a_{pud_rule_id::axiom{1}}
        , callee_b_{pud_rule_id::axiom{2}}
        , callee_c_{pud_rule_id::axiom{3}}
        , child_{pud_rule_id::inference{&leaf_, 0, &leaf_}}
        , child_a_{pud_rule_id::inference{&leaf_, 0, &callee_a_}}
        , child_b_{pud_rule_id::inference{&leaf_, 0, &callee_b_}}
        , parent_node_{interval_, {}, {&body_}, 1}
        , child_node_{nested_, {}, {&body_}, 1}
        , parent_query_{interval_, &body_, {pud_candidate_search_context{&leaf_, {}}}, 1}
        , child_query_{nested_, &body_, {pud_candidate_search_context{&leaf_, {}}}, 1}
        , parent_query_ptrs_{&parent_query_}
        , child_query_ptrs_{&child_query_}
        , unfolder_(get_node_, unify_callee_, normalize_, make_var_,
                    add_inference_, link_children_, effective_body_,
                    get_queries_, clear_queries_, fork_child_,
                    invalidate_, resume_, take_dirty_) {
        ON_CALL(get_node_, get_node(&leaf_)).WillByDefault(ReturnRef(parent_node_));
        ON_CALL(get_node_, get_node(&child_)).WillByDefault(ReturnRef(child_node_));
        ON_CALL(get_node_, get_node(&child_a_)).WillByDefault(ReturnRef(child_node_));
        ON_CALL(get_node_, get_node(&child_b_)).WillByDefault(ReturnRef(child_node_));
        ON_CALL(get_queries_, get(&leaf_)).WillByDefault(ReturnRef(parent_query_ptrs_));
        ON_CALL(get_queries_, get(&child_)).WillByDefault(ReturnRef(child_query_ptrs_));
        ON_CALL(get_queries_, get(&child_a_)).WillByDefault(ReturnRef(child_query_ptrs_));
        ON_CALL(get_queries_, get(&child_b_)).WillByDefault(ReturnRef(child_query_ptrs_));
        ON_CALL(unify_callee_, unify_callee(_, _, _, _)).WillByDefault(Return(true));
        ON_CALL(make_var_, make_var(_)).WillByDefault(Return(&var0_));
        ON_CALL(normalize_, normalize(_, _, _, _, _)).WillByDefault(Return(&body_));
        ON_CALL(effective_body_, effective_body(_)).WillByDefault(
            Return(std::vector<const expr*>{&body_}));
        ON_CALL(add_inference_, add_inference(_, _, _, _)).WillByDefault(Return(&child_));
        ON_CALL(resume_, resume(_, _)).WillByDefault(Return(
            pud_candidate_search_result{pud_candidate_search_result::self_witness{}}));
        ON_CALL(take_dirty_, take_dirty_leaves()).WillByDefault(
            Return(std::vector<const pud_rule_id*>{&child_}));
    }

    struct unfold_out {
        std::vector<pud_forced_unfold> yields;
        std::vector<const pud_rule_id*> children;
    };

    unfold_out drain(
            coroutine<pud_forced_unfold, std::vector<const pud_rule_id*>> task) {
        unfold_out out;
        while (!task.done()) {
            task.resume();
            if (task.has_yield())
                out.yields.push_back(task.consume_yield());
        }
        out.children = task.result();
        return out;
    }

    pud_candidate_search_result skip_refuted_callee(pud_query&,
                                                    pud_candidate_search_context& ctx) {
        if (ctx.cursor == &callee_c_)
            return pud_candidate_search_result{
                pud_candidate_search_result::axiom_refuted{}};
        return pud_candidate_search_result{
            pud_candidate_search_result::self_witness{}};
    }

    uint64_t open_;
    uint64_t close_;
    uint64_t nested_open_;
    uint64_t nested_close_;
    om_interval interval_;
    om_interval nested_;
    expr body_;
    expr var0_;
    pud_rule_id leaf_;
    pud_rule_id callee_a_;
    pud_rule_id callee_b_;
    pud_rule_id callee_c_;
    pud_rule_id child_;
    pud_rule_id child_a_;
    pud_rule_id child_b_;
    pud_db_node parent_node_;
    pud_db_node child_node_;
    pud_query parent_query_;
    pud_query child_query_;
    std::vector<pud_query*> parent_query_ptrs_;
    std::vector<pud_query*> child_query_ptrs_;
    NiceMock<MockGetNode> get_node_;
    NiceMock<MockUnifyCallee> unify_callee_;
    NiceMock<MockNormalize> normalize_;
    NiceMock<MockMakeVar> make_var_;
    NiceMock<MockAddInference> add_inference_;
    NiceMock<MockLinkChildren> link_children_;
    NiceMock<MockEffectiveBody> effective_body_;
    NiceMock<MockGetLeafQueries> get_queries_;
    NiceMock<MockClearLeafQueries> clear_queries_;
    NiceMock<MockForkChild> fork_child_;
    NiceMock<MockInvalidateLeaf> invalidate_;
    NiceMock<MockResumeCandidateSearch> resume_;
    NiceMock<MockTakeDirtyLeaves> take_dirty_;
    test_unfolder_t unfolder_;
};

TEST_F(PudUnfolderTest, UnfoldLinksChildClearsParentAndYieldsUnit) {
    EXPECT_CALL(add_inference_, add_inference(&leaf_, 0, &leaf_, _)).WillOnce(Return(&child_));
    EXPECT_CALL(link_children_, link_children(&leaf_, ElementsAre(&child_)));
    EXPECT_CALL(clear_queries_, clear(&leaf_));
    EXPECT_CALL(invalidate_, invalidate_leaf(&leaf_));
    EXPECT_CALL(fork_child_, fork_child(&child_, _));

    const unfold_out out = drain(unfolder_.unfold(&leaf_, 0));
    ASSERT_EQ(out.yields.size(), 1u);
    ASSERT_TRUE(std::holds_alternative<pud_forced_unfold::unit>(out.yields[0].content));
    const auto unit = std::get<pud_forced_unfold::unit>(out.yields[0].content);
    EXPECT_EQ(unit.leaf, &child_);
    EXPECT_EQ(unit.body_goal_idx, 0u);
    EXPECT_THAT(out.children, ElementsAre(&child_));
}

TEST_F(PudUnfolderTest, UnfoldYieldsRefutedWhenABodyGoalHasZeroCandidates) {
    EXPECT_CALL(resume_, resume(_, _))
        .WillOnce(Return(
            pud_candidate_search_result{pud_candidate_search_result::self_witness{}}))
        .WillRepeatedly(Return(
            pud_candidate_search_result{pud_candidate_search_result::axiom_refuted{}}));

    const unfold_out out = drain(unfolder_.unfold(&leaf_, 0));
    ASSERT_EQ(out.yields.size(), 1u);
    ASSERT_TRUE(std::holds_alternative<pud_forced_unfold::refuted>(out.yields[0].content));
    EXPECT_EQ(std::get<pud_forced_unfold::refuted>(out.yields[0].content).leaf, &child_);
}

TEST_F(PudUnfolderTest, UnfoldDoesNotCallUnfoldOnAnyCollaborator) {
    EXPECT_CALL(add_inference_, add_inference(_, _, _, _)).WillOnce(Return(&child_));
    EXPECT_CALL(link_children_, link_children(_, _));
    EXPECT_CALL(invalidate_, invalidate_leaf(&leaf_));
    drain(unfolder_.unfold(&leaf_, 0));
}

TEST_F(PudUnfolderTest, LiveCursorIsTheCalleePassedToInference) {
    EXPECT_CALL(add_inference_, add_inference(&leaf_, 0, &leaf_, _)).WillOnce(Return(&child_));
    drain(unfolder_.unfold(&leaf_, 0));
}

TEST_F(PudUnfolderTest, UnfoldCreatesOneChildPerLiveCandidateAndSkipsRefuted) {
    parent_query_.axiom_contexts = {
        pud_candidate_search_context{&callee_a_, {}},
        pud_candidate_search_context{&callee_c_, {}},
        pud_candidate_search_context{&callee_b_, {}}};

    ON_CALL(resume_, resume(_, _))
        .WillByDefault(Invoke(this, &PudUnfolderTest::skip_refuted_callee));
    ON_CALL(take_dirty_, take_dirty_leaves()).WillByDefault(
        Return(std::vector<const pud_rule_id*>{&child_a_, &child_b_}));

    {
        InSequence seq;
        EXPECT_CALL(add_inference_, add_inference(&leaf_, 0, &callee_a_, _))
            .WillOnce(Return(&child_a_));
        EXPECT_CALL(add_inference_, add_inference(&leaf_, 0, &callee_b_, _))
            .WillOnce(Return(&child_b_));
        EXPECT_CALL(link_children_,
                    link_children(&leaf_, ElementsAre(&child_a_, &child_b_)));
    }
    EXPECT_CALL(fork_child_, fork_child(&child_a_, _));
    EXPECT_CALL(fork_child_, fork_child(&child_b_, _));

    const unfold_out out = drain(unfolder_.unfold(&leaf_, 0));
    EXPECT_THAT(out.children, ElementsAre(&child_a_, &child_b_));
}
