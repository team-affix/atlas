// pud_unify_head: unify_head path-replays ancestor unifications; unify_callee / normalize.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <optional>
#include <random>
#include <sstream>
#include <unordered_map>
#include <vector>
#include "infrastructure/pud_unify_head.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/om_interval.hpp"
#include "value_objects/pud_added_unification.hpp"
#include "value_objects/pud_query.hpp"
#include "value_objects/pud_rule_id.hpp"

using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::IsEmpty;
using ::testing::_;

struct MockAllocateChildInterval {
    MOCK_METHOD(om_interval, allocate_child_of, (const om_interval&), ());
};

struct MockFindParent {
    MOCK_METHOD(const pud_rule_id*, find_parent, (const pud_rule_id*), ());
};

struct MockGetAddedUnifications {
    MOCK_METHOD(const std::vector<pud_added_unification>&, get, (const pud_rule_id*), ());
};

struct MockRecordBinding {
    MOCK_METHOD(void, record, (om_interval, uint32_t, framed_expr), ());
};

struct MockQueryBinding {
    MOCK_METHOD(std::optional<framed_expr>, query, (om_label, uint32_t), ());
};

struct MockGlobalize {
    MOCK_METHOD(uint32_t, globalize, (uint32_t, uint32_t), ());
};

struct MockMakeVar {
    MOCK_METHOD(const expr*, make_var, (uint32_t), ());
};

struct MockMakeFunctor {
    MOCK_METHOD(const expr*, make_functor, (uint32_t, const std::vector<const expr*>&), ());
};

using test_unify_head_t = pud_unify_head<
    NiceMock<MockAllocateChildInterval>,
    NiceMock<MockFindParent>,
    NiceMock<MockGetAddedUnifications>,
    NiceMock<MockRecordBinding>,
    NiceMock<MockQueryBinding>,
    NiceMock<MockGlobalize>,
    NiceMock<MockMakeVar>,
    NiceMock<MockMakeFunctor>>;

struct PudUnifyHeadTest : public ::testing::Test {
    PudUnifyHeadTest()
        : open_(10)
        , close_(40)
        , nested_open_(15)
        , nested_close_(20)
        , interval_{om_label(&open_), om_label(&close_)}
        , nested_{om_label(&nested_open_), om_label(&nested_close_)}
        , pred_{expr::functor{7, {}}}
        , var0_{expr::var{0}}
        , axiom_{pud_rule_id::axiom{0}}
        , mid_{pud_rule_id::inference{&axiom_, 0, &axiom_}}
        , leaf_{pud_rule_id::inference{&mid_, 0, &axiom_}}
        , unifs_axiom_{{0, &pred_}}
        , unifs_mid_{{1, &pred_}}
        , unifs_leaf_{{2, &pred_}}
        , query_{interval_, &pred_, {pud_candidate_search_context{&axiom_, {}}}, 1}
        , unify_head_(allocate_, find_parent_, get_added_unifications_, record_,
                      query_binding_, globalize_, make_var_, make_functor_) {
        ON_CALL(find_parent_, find_parent(_)).WillByDefault(Return(nullptr));
        ON_CALL(get_added_unifications_, get(&axiom_)).WillByDefault(ReturnRef(unifs_axiom_));
        ON_CALL(get_added_unifications_, get(&mid_)).WillByDefault(ReturnRef(unifs_mid_));
        ON_CALL(get_added_unifications_, get(&leaf_)).WillByDefault(ReturnRef(unifs_leaf_));
        ON_CALL(allocate_, allocate_child_of(_)).WillByDefault(Return(nested_));
        ON_CALL(make_var_, make_var(0)).WillByDefault(Return(&var0_));
        ON_CALL(globalize_, globalize(_, _)).WillByDefault([](uint32_t frame, uint32_t idx) {
            return frame + idx;
        });
        ON_CALL(query_binding_, query(_, 0)).WillByDefault(Return(framed_expr{&pred_, 0}));
    }

    uint64_t open_;
    uint64_t close_;
    uint64_t nested_open_;
    uint64_t nested_close_;
    om_interval interval_;
    om_interval nested_;
    expr pred_;
    expr var0_;
    pud_rule_id axiom_;
    pud_rule_id mid_;
    pud_rule_id leaf_;
    std::vector<pud_added_unification> unifs_axiom_;
    std::vector<pud_added_unification> unifs_mid_;
    std::vector<pud_added_unification> unifs_leaf_;
    pud_query query_;
    NiceMock<MockAllocateChildInterval> allocate_;
    NiceMock<MockFindParent> find_parent_;
    NiceMock<MockGetAddedUnifications> get_added_unifications_;
    NiceMock<MockRecordBinding> record_;
    NiceMock<MockQueryBinding> query_binding_;
    NiceMock<MockGlobalize> globalize_;
    NiceMock<MockMakeVar> make_var_;
    NiceMock<MockMakeFunctor> make_functor_;
    test_unify_head_t unify_head_;
};

TEST_F(PudUnifyHeadTest, UnifyHeadRecordsPathAndSucceedsWhenHeadMatches) {
    EXPECT_CALL(allocate_, allocate_child_of(_)).WillOnce(Return(nested_));
    EXPECT_CALL(record_, record(_, 0, _)).Times(::testing::AtLeast(1));
    EXPECT_TRUE(unify_head_.unify_head(query_, &axiom_));
}

TEST_F(PudUnifyHeadTest, UnifyHeadFailsWhenRecordedHeadDiffersFromBody) {
    expr other{expr::functor{8, {}}};
    ON_CALL(query_binding_, query(_, 0)).WillByDefault(Return(framed_expr{&other, 0}));
    EXPECT_FALSE(unify_head_.unify_head(query_, &axiom_));
}

TEST_F(PudUnifyHeadTest, UnifyHeadRecordsEachNodeOnAThreeNodeChain) {
    query_.axiom_contexts = {pud_candidate_search_context{&leaf_, {}}};
    query_.frame_offset = 3;
    EXPECT_CALL(find_parent_, find_parent(&leaf_)).WillRepeatedly(Return(&mid_));
    EXPECT_CALL(find_parent_, find_parent(&mid_)).WillRepeatedly(Return(&axiom_));
    EXPECT_CALL(find_parent_, find_parent(&axiom_)).WillRepeatedly(Return(nullptr));
    EXPECT_CALL(record_, record(_, 0, _)).Times(::testing::AtLeast(1));
    EXPECT_CALL(record_, record(_, 1, _)).Times(::testing::AtLeast(1));
    EXPECT_CALL(record_, record(_, 2, _)).Times(::testing::AtLeast(1));
    unify_head_.unify_head(query_, &leaf_);
}

TEST_F(PudUnifyHeadTest, UnifyCalleeWritesEnvAndCollectsTouchedReps) {
    ON_CALL(query_binding_, query(_, _)).WillByDefault(Return(std::nullopt));
    om_interval env{om_label(&open_), om_label(&close_)};
    std::vector<uint32_t> touched_reps;
    EXPECT_CALL(allocate_, allocate_child_of(_)).WillOnce(Return(nested_));
    EXPECT_TRUE(unify_head_.unify_callee(query_, &axiom_, touched_reps, env));
    EXPECT_EQ(env.open.rank_ptr(), nested_.open.rank_ptr());
    EXPECT_EQ(env.close.rank_ptr(), nested_.close.rank_ptr());
    EXPECT_FALSE(touched_reps.empty());
}

TEST_F(PudUnifyHeadTest, UnifyCalleeFailsWhenHeadDiffers) {
    expr other{expr::functor{8, {}}};
    ON_CALL(query_binding_, query(_, 0)).WillByDefault(Return(framed_expr{&other, 0}));
    om_interval env{om_label(&open_), om_label(&close_)};
    std::vector<uint32_t> touched_reps;
    EXPECT_CALL(allocate_, allocate_child_of(_)).WillOnce(Return(nested_));
    EXPECT_FALSE(unify_head_.unify_callee(query_, &axiom_, touched_reps, env));
    EXPECT_EQ(env.open.rank_ptr(), nested_.open.rank_ptr());
}

TEST_F(PudUnifyHeadTest, NormalizeGroundFunctorRoundTrips) {
    EXPECT_CALL(make_functor_, make_functor(7, IsEmpty()))
        .WillOnce(Return(&pred_));
    std::unordered_map<uint32_t, uint32_t> translation;
    EXPECT_EQ(unify_head_.normalize(interval_, framed_expr{&pred_, 0}, 0, translation),
              &pred_);
    EXPECT_TRUE(translation.empty());
}

TEST_F(PudUnifyHeadTest, NormalizeGrowsTranslationForLiftedVars) {
    expr lifted{expr::var{0}};
    ON_CALL(query_binding_, query(_, _)).WillByDefault(Return(std::nullopt));
    ON_CALL(make_var_, make_var(1)).WillByDefault(Return(&lifted));
    std::unordered_map<uint32_t, uint32_t> translation;
    const expr* out = unify_head_.normalize(
        interval_, framed_expr{&var0_, 1}, 1, translation);
    EXPECT_EQ(out, &lifted);
    ASSERT_EQ(translation.size(), 1u);
    EXPECT_EQ(translation.at(1), 1u);
}

TEST_F(PudUnifyHeadTest, DropQueryAllowsFreshEnsure) {
    EXPECT_TRUE(unify_head_.unify_head(query_, &axiom_));
    unify_head_.drop_query(&query_);
    EXPECT_CALL(allocate_, allocate_child_of(_)).WillOnce(Return(nested_));
    EXPECT_CALL(record_, record(_, 0, _)).Times(::testing::AtLeast(1));
    EXPECT_TRUE(unify_head_.unify_head(query_, &axiom_));
}

TEST_F(PudUnifyHeadTest, UnifyHeadEnsuresAncestorChainFromLiveEdgeCurrent) {
    query_.axiom_contexts = {pud_candidate_search_context{
        &axiom_,
        {pud_witness_search_context{&leaf_, &leaf_}}}};
    query_.frame_offset = 3;
    EXPECT_CALL(find_parent_, find_parent(&leaf_)).WillRepeatedly(Return(&mid_));
    EXPECT_CALL(find_parent_, find_parent(&mid_)).WillRepeatedly(Return(&axiom_));
    EXPECT_CALL(find_parent_, find_parent(&axiom_)).WillRepeatedly(Return(nullptr));
    EXPECT_CALL(record_, record(_, 0, _)).Times(::testing::AtLeast(1));
    EXPECT_CALL(record_, record(_, 1, _)).Times(::testing::AtLeast(1));
    EXPECT_CALL(record_, record(_, 2, _)).Times(::testing::AtLeast(1));
    unify_head_.unify_head(query_, &leaf_);
}

TEST_F(PudUnifyHeadTest, RepeatedUnifyHeadReusesCachedChildInterval) {
    EXPECT_CALL(allocate_, allocate_child_of(_))
        .Times(1)
        .WillOnce(Return(nested_));
    for (int step = 0; step < 5; ++step)
        EXPECT_TRUE(unify_head_.unify_head(query_, &axiom_));
}

TEST_F(PudUnifyHeadTest, FuzzUnifyHeadAndDropQuery) {
    constexpr uint32_t k_seed = 42;
    std::mt19937 rng{k_seed};
    std::uniform_int_distribution<int> op_dist(0, 3);
    std::ostringstream log;
    query_.axiom_contexts = {pud_candidate_search_context{&axiom_, {}}};
    ON_CALL(make_functor_, make_functor(_, _)).WillByDefault(Return(&pred_));
    ON_CALL(make_var_, make_var(_)).WillByDefault(Return(&var0_));
    for (int step = 0; step < 64; ++step) {
        const int op = op_dist(rng);
        log << step << ':' << op << ' ';
        switch (op) {
        case 0:
            unify_head_.unify_head(query_, &axiom_);
            break;
        case 1: {
            om_interval env{interval_};
            std::vector<uint32_t> touched;
            unify_head_.unify_callee(query_, &axiom_, touched, env);
            break;
        }
        case 2: {
            std::unordered_map<uint32_t, uint32_t> translation;
            unify_head_.normalize(interval_, framed_expr{&pred_, 0}, 0, translation);
            break;
        }
        case 3:
            unify_head_.drop_query(&query_);
            break;
        }
    }
    unify_head_.drop_query(&query_);
    EXPECT_CALL(record_, record(_, 0, _)).Times(::testing::AtLeast(1));
    unify_head_.unify_head(query_, &axiom_);
    EXPECT_FALSE(log.str().empty()) << "seed " << k_seed;
}
