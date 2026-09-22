// pud_witness_search: accept-first resume; intern-allocate-store-unify on descend.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <deque>
#include <optional>
#include <random>
#include <set>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "infrastructure/pud_witness_search.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/om_interval.hpp"
#include "value_objects/pud_added_unification.hpp"
#include "value_objects/pud_rule_id.hpp"
#include "value_objects/pud_witness_search_context.hpp"

using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::SaveArg;
using ::testing::_;

using children_set_t = std::set<const pud_rule_id*>;
using children_opt_t = std::optional<children_set_t>;

struct MockGetChildren {
    MOCK_METHOD(children_opt_t, get, (const pud_rule_id*), ());
};

struct MockGetParent {
    MOCK_METHOD(const pud_rule_id*, get, (const pud_rule_id*), ());
};

struct MockMakeInference {
    MOCK_METHOD(const pud_rule_id*, make_inference,
                (const pud_rule_id*, size_t, const pud_rule_id*), ());
};

struct MockContainsInterval {
    MOCK_METHOD(bool, contains, (const pud_rule_id*), ());
};

struct MockGetInterval {
    MOCK_METHOD(const om_interval&, get, (const pud_rule_id*), ());
};

struct MockStoreInterval {
    MOCK_METHOD(void, store, (const pud_rule_id*, om_interval), ());
};

struct MockAllocateChildInterval {
    MOCK_METHOD(om_interval, allocate_child_of, (const om_interval&), ());
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

struct MockStoreAddedCallerReps {
    MOCK_METHOD(void, store, (const pud_rule_id*, (std::vector<uint32_t>)), ());
};

using test_search_t = pud_witness_search<
    NiceMock<MockGetChildren>,
    NiceMock<MockGetParent>,
    NiceMock<MockMakeInference>,
    NiceMock<MockContainsInterval>,
    NiceMock<MockGetInterval>,
    NiceMock<MockStoreInterval>,
    NiceMock<MockAllocateChildInterval>,
    NiceMock<MockGetAddedUnifications>,
    NiceMock<MockRecordBinding>,
    NiceMock<MockQueryBinding>,
    NiceMock<MockGlobalize>,
    NiceMock<MockMakeVar>,
    NiceMock<MockStoreAddedCallerReps>>;

struct PudWitnessSearchTest : public ::testing::Test {
    PudWitnessSearchTest()
        : body_{expr::functor{1, {}}}
        , mismatch_{expr::functor{2, {}}}
        , var0_{expr::var{0}}
        , var1_{expr::var{1}}
        , const_a_{expr::functor{3, {}}}
        , query_leaf_{pud_rule_id::axiom{99}}
        , a0_{pud_rule_id::axiom{0}}
        , c0_{pud_rule_id::inference{&a0_, 0, &a0_}}
        , c1_{pud_rule_id::inference{&a0_, 1, &a0_}}
        , g0_{pud_rule_id::inference{&c0_, 0, &a0_}}
        , k_query_{pud_rule_id::inference{&query_leaf_, 0, nullptr}}
        , k_a0_{pud_rule_id::inference{&query_leaf_, 0, &a0_}}
        , k_c0_{pud_rule_id::inference{&query_leaf_, 0, &c0_}}
        , k_c1_{pud_rule_id::inference{&query_leaf_, 0, &c1_}}
        , k_g0_{pud_rule_id::inference{&query_leaf_, 0, &g0_}}
        , match_unifs_{{0, &body_}}
        , mismatch_unifs_{{0, &mismatch_}}
        , empty_unifs_{}
        , extra_vars_{}
        , query_open_(1)
        , query_close_(2)
        , search_(children_, get_parent_, make_inference_,
                  contains_, get_interval_, store_interval_, allocate_,
                  get_unifs_, record_, query_binding_, globalize_, make_var_,
                  store_added_caller_reps_) {
        intervals_.insert_or_assign(
            &k_query_, om_interval{om_label(&query_open_), om_label(&query_close_)});
        ON_CALL(make_inference_, make_inference(_, _, _))
            .WillByDefault([this](const pud_rule_id*, size_t, const pud_rule_id* node) {
                if (node == nullptr)
                    return static_cast<const pud_rule_id*>(&k_query_);
                return key_for(node);
            });
        ON_CALL(contains_, contains(_))
            .WillByDefault([this](const pud_rule_id* id) {
                return stored_.contains(id);
            });
        ON_CALL(get_interval_, get(_))
            .WillByDefault([this](const pud_rule_id* id) -> const om_interval& {
                auto it = intervals_.find(id);
                if (it != intervals_.end())
                    return it->second;
                return intervals_.at(&k_query_);
            });
        ON_CALL(store_interval_, store(_, _))
            .WillByDefault([this](const pud_rule_id* id, om_interval interval) {
                stored_.insert(id);
                intervals_.insert_or_assign(id, interval);
            });
        ON_CALL(allocate_, allocate_child_of(_))
            .WillByDefault([this](const om_interval&) {
                ranks_.push_back(ranks_.size());
                ranks_.push_back(ranks_.size());
                return om_interval{
                    om_label(&ranks_[ranks_.size() - 2]),
                    om_label(&ranks_[ranks_.size() - 1])};
            });
        ON_CALL(get_unifs_, get(_))
            .WillByDefault([this](const pud_rule_id* node)
                    -> const std::vector<pud_added_unification>& {
                if (fail_nodes_.contains(node))
                    return mismatch_unifs_;
                return match_unifs_;
            });
        ON_CALL(record_, record(_, _, _))
            .WillByDefault([this](om_interval interval, uint32_t var_id, framed_expr value) {
                recorded_[interval.open.rank_ptr()][var_id] = value;
            });
        ON_CALL(query_binding_, query(_, _))
            .WillByDefault([this](om_label open, uint32_t var_id)
                    -> std::optional<framed_expr> {
                auto interval_it = recorded_.find(open.rank_ptr());
                if (interval_it != recorded_.end()) {
                    auto var_it = interval_it->second.find(var_id);
                    if (var_it != interval_it->second.end())
                        return var_it->second;
                }
                auto query_it = recorded_.find(&query_open_);
                if (query_it == recorded_.end())
                    return std::nullopt;
                auto var_it = query_it->second.find(var_id);
                if (var_it == query_it->second.end())
                    return std::nullopt;
                return var_it->second;
            });
        ON_CALL(globalize_, globalize(_, _))
            .WillByDefault([](uint32_t offset, uint32_t idx) { return offset + idx; });
        ON_CALL(make_var_, make_var(_))
            .WillByDefault([this](uint32_t idx) -> const expr* {
                if (idx == 0)
                    return &var0_;
                if (idx == 1)
                    return &var1_;
                extra_vars_.push_back(expr{expr::var{idx}});
                return &extra_vars_.back();
            });
        ON_CALL(get_parent_, get(&a0_)).WillByDefault(Return(nullptr));
        ON_CALL(get_parent_, get(&c0_)).WillByDefault(Return(&a0_));
        ON_CALL(get_parent_, get(&c1_)).WillByDefault(Return(&a0_));
        ON_CALL(get_parent_, get(&g0_)).WillByDefault(Return(&c0_));
        bind_hole(&body_, 1);
    }

    void bind_hole(const expr* body, uint32_t frame_offset) {
        recorded_[&query_open_].clear();
        recorded_[&query_open_][frame_offset] = framed_expr{body, 0};
    }

    const pud_rule_id* key_for(const pud_rule_id* node) const {
        if (node == &a0_)
            return &k_a0_;
        if (node == &c0_)
            return &k_c0_;
        if (node == &c1_)
            return &k_c1_;
        if (node == &g0_)
            return &k_g0_;
        return node;
    }

    void pre_store(const pud_rule_id* node) {
        const pud_rule_id* key = key_for(node);
        ranks_.push_back(ranks_.size());
        ranks_.push_back(ranks_.size());
        intervals_.insert_or_assign(key, om_interval{
            om_label(&ranks_[ranks_.size() - 2]),
            om_label(&ranks_[ranks_.size() - 1])});
        stored_.insert(key);
    }

    pud_witness_search_context make_edge(const pud_rule_id* search_root,
                                         const pud_rule_id* current) {
        return pud_witness_search_context{&query_leaf_, 0, 1, search_root, current};
    }

    expr body_;
    expr mismatch_;
    expr var0_;
    expr var1_;
    expr const_a_;
    pud_rule_id query_leaf_;
    pud_rule_id a0_;
    pud_rule_id c0_;
    pud_rule_id c1_;
    pud_rule_id g0_;
    pud_rule_id k_query_;
    pud_rule_id k_a0_;
    pud_rule_id k_c0_;
    pud_rule_id k_c1_;
    pud_rule_id k_g0_;
    std::vector<pud_added_unification> match_unifs_;
    std::vector<pud_added_unification> mismatch_unifs_;
    std::vector<pud_added_unification> empty_unifs_;
    std::deque<expr> extra_vars_;
    std::unordered_set<const pud_rule_id*> fail_nodes_;
    std::unordered_set<const pud_rule_id*> stored_;
    std::unordered_map<const pud_rule_id*, om_interval> intervals_;
    std::unordered_map<const uint64_t*, std::unordered_map<uint32_t, framed_expr>> recorded_;
    std::deque<uint64_t> ranks_;
    uint64_t query_open_;
    uint64_t query_close_;
    NiceMock<MockGetChildren> children_;
    NiceMock<MockGetParent> get_parent_;
    NiceMock<MockMakeInference> make_inference_;
    NiceMock<MockContainsInterval> contains_;
    NiceMock<MockGetInterval> get_interval_;
    NiceMock<MockStoreInterval> store_interval_;
    NiceMock<MockAllocateChildInterval> allocate_;
    NiceMock<MockGetAddedUnifications> get_unifs_;
    NiceMock<MockRecordBinding> record_;
    NiceMock<MockQueryBinding> query_binding_;
    NiceMock<MockGlobalize> globalize_;
    NiceMock<MockMakeVar> make_var_;
    NiceMock<MockStoreAddedCallerReps> store_added_caller_reps_;
    test_search_t search_;
};

TEST_F(PudWitnessSearchTest, AcceptsCurrentIfItIsUnifyingLeaf) {
    pud_witness_search_context ctx = make_edge(&a0_, &a0_);
    EXPECT_CALL(children_, get(&a0_)).WillRepeatedly(Return(std::nullopt));
    search_.resume(ctx);
    EXPECT_EQ(ctx.current, &a0_);
}

TEST_F(PudWitnessSearchTest, DescendsIntoChildrenWhenCurrentIsNoLongerALeaf) {
    pud_witness_search_context ctx = make_edge(&a0_, &a0_);
    EXPECT_CALL(children_, get(&a0_)).WillRepeatedly(Return(children_set_t{&c0_}));
    EXPECT_CALL(children_, get(&c0_)).WillRepeatedly(Return(std::nullopt));
    search_.resume(ctx);
    EXPECT_EQ(ctx.current, &c0_);
}

TEST_F(PudWitnessSearchTest, PrunesSubtreeWhenUnifyFails) {
    fail_nodes_.insert(&a0_);
    pud_witness_search_context ctx = make_edge(&a0_, &a0_);
    EXPECT_CALL(children_, get(&a0_)).WillRepeatedly(Return(children_set_t{&c0_}));
    search_.resume(ctx);
    EXPECT_EQ(ctx.current, nullptr);
    EXPECT_EQ(ctx.search_root, &a0_);
}

TEST_F(PudWitnessSearchTest, TriesNextSiblingInIdOrderAfterFailedChild) {
    fail_nodes_.insert(&c0_);
    pud_witness_search_context ctx = make_edge(&a0_, &a0_);
    EXPECT_CALL(children_, get(&c0_)).WillRepeatedly(Return(std::nullopt));
    EXPECT_CALL(children_, get(&c1_)).WillRepeatedly(Return(std::nullopt));
    EXPECT_CALL(children_, get(&a0_)).WillRepeatedly(Return(children_set_t{&c0_, &c1_}));
    search_.resume(ctx);
    EXPECT_EQ(ctx.current, &c1_);
}

TEST_F(PudWitnessSearchTest, StopsAtSearchRootAndFailsWhenNoSiblingWorks) {
    fail_nodes_.insert(&g0_);
    pud_witness_search_context ctx = make_edge(&c0_, &c0_);
    EXPECT_CALL(children_, get(&g0_)).WillRepeatedly(Return(std::nullopt));
    EXPECT_CALL(children_, get(&c0_)).WillRepeatedly(Return(children_set_t{&g0_}));
    search_.resume(ctx);
    EXPECT_EQ(ctx.current, nullptr);
    EXPECT_EQ(ctx.search_root, &c0_);
}

TEST_F(PudWitnessSearchTest, NullCurrentIsANoOp) {
    pud_witness_search_context ctx = make_edge(&a0_, nullptr);
    EXPECT_CALL(children_, get(_)).Times(0);
    EXPECT_CALL(allocate_, allocate_child_of(_)).Times(0);
    search_.resume(ctx);
    EXPECT_EQ(ctx.current, nullptr);
    EXPECT_EQ(ctx.search_root, &a0_);
}

TEST_F(PudWitnessSearchTest, SelfNodeMayBeAWitness) {
    pud_witness_search_context ctx = make_edge(&a0_, &a0_);
    EXPECT_CALL(children_, get(&a0_)).WillRepeatedly(Return(std::nullopt));
    search_.resume(ctx);
    EXPECT_EQ(ctx.current, &a0_);
}

TEST_F(PudWitnessSearchTest, DescendsTwoLevelsToGrandchild) {
    pud_witness_search_context ctx = make_edge(&a0_, &a0_);
    EXPECT_CALL(children_, get(&a0_)).WillRepeatedly(Return(children_set_t{&c0_}));
    EXPECT_CALL(children_, get(&c0_)).WillRepeatedly(Return(children_set_t{&g0_}));
    EXPECT_CALL(children_, get(&g0_)).WillRepeatedly(Return(std::nullopt));
    search_.resume(ctx);
    EXPECT_EQ(ctx.current, &g0_);
}

TEST_F(PudWitnessSearchTest, InternalUnifyWithNoUnifyingChildFailsThenTriesSibling) {
    fail_nodes_.insert(&g0_);
    pud_witness_search_context ctx = make_edge(&a0_, &a0_);
    EXPECT_CALL(children_, get(&c0_)).WillRepeatedly(Return(children_set_t{&g0_}));
    EXPECT_CALL(children_, get(&g0_)).WillRepeatedly(Return(std::nullopt));
    EXPECT_CALL(children_, get(&c1_)).WillRepeatedly(Return(std::nullopt));
    EXPECT_CALL(children_, get(&a0_)).WillRepeatedly(Return(children_set_t{&c0_, &c1_}));
    search_.resume(ctx);
    EXPECT_EQ(ctx.current, &c1_);
}

TEST_F(PudWitnessSearchTest, ClimbsTwoParentsToReachUncle) {
    fail_nodes_.insert(&g0_);
    pre_store(&a0_);
    pre_store(&c0_);
    pud_witness_search_context ctx = make_edge(&a0_, &g0_);
    EXPECT_CALL(children_, get(&g0_)).WillRepeatedly(Return(std::nullopt));
    EXPECT_CALL(children_, get(&c1_)).WillRepeatedly(Return(std::nullopt));
    EXPECT_CALL(children_, get(&c0_)).WillRepeatedly(Return(children_set_t{&g0_}));
    EXPECT_CALL(children_, get(&a0_)).WillRepeatedly(Return(children_set_t{&c0_, &c1_}));
    search_.resume(ctx);
    EXPECT_EQ(ctx.current, &c1_);
}

TEST_F(PudWitnessSearchTest, ResumeFoundKeepsAcceptableCurrent) {
    pud_witness_search_context ctx = make_edge(&a0_, &a0_);
    EXPECT_CALL(children_, get(&a0_)).WillRepeatedly(Return(std::nullopt));
    EXPECT_CALL(allocate_, allocate_child_of(_)).Times(1);
    for (int step = 0; step < 8; ++step) {
        search_.resume(ctx);
        EXPECT_EQ(ctx.current, &a0_);
    }
}

TEST_F(PudWitnessSearchTest, StressWideSiblingScan) {
    std::vector<pud_rule_id> siblings;
    siblings.reserve(32);
    for (int idx = 0; idx < 32; ++idx)
        siblings.push_back(pud_rule_id{pud_rule_id::inference{&a0_, static_cast<size_t>(idx), &a0_}});
    children_set_t sibling_set;
    for (const pud_rule_id& sibling : siblings)
        sibling_set.insert(&sibling);

    EXPECT_CALL(children_, get(_))
        .WillRepeatedly([this, sibling_set](const pud_rule_id* node) -> children_opt_t {
            if (node == &a0_)
                return sibling_set;
            return std::nullopt;
        });
    ON_CALL(get_parent_, get(_))
        .WillByDefault([this, &siblings](const pud_rule_id* node) -> const pud_rule_id* {
            if (node == &a0_)
                return nullptr;
            for (const pud_rule_id& sibling : siblings) {
                if (node == &sibling)
                    return &a0_;
            }
            return nullptr;
        });
    for (size_t idx = 0; idx + 1 < siblings.size(); ++idx)
        fail_nodes_.insert(&siblings[idx]);
    pud_witness_search_context ctx = make_edge(&a0_, &a0_);
    search_.resume(ctx);
    EXPECT_EQ(ctx.current, &siblings.back());
}

TEST_F(PudWitnessSearchTest, FuzzResumeOnFixedMockTree) {
    fail_nodes_.insert(&a0_);
    fail_nodes_.insert(&c0_);
    ON_CALL(children_, get(_)).WillByDefault([&](const pud_rule_id* node) -> children_opt_t {
        if (node == &a0_)
            return children_set_t{&c0_, &c1_};
        if (node == &c0_)
            return children_set_t{&g0_};
        return std::nullopt;
    });

    const std::vector<std::pair<const pud_rule_id*, const pud_rule_id*>> legal = {
        {&a0_, &a0_}, {&a0_, &c0_}, {&a0_, &c1_}, {&a0_, &g0_},
        {&c0_, &c0_}, {&c0_, &g0_}, {&c1_, &c1_}, {&g0_, &g0_},
    };
    constexpr uint32_t k_seed = 42;
    std::mt19937 rng{k_seed};
    std::uniform_int_distribution<size_t> pick(0, legal.size() - 1);
    std::ostringstream log;
    for (int step = 0; step < 80; ++step) {
        const auto [search_root, current] = legal[pick(rng)];
        log << step << ':' << search_root << ',' << current << ' ';
        pud_witness_search_context ctx = make_edge(search_root, current);
        search_.resume(ctx);
        if (ctx.current != nullptr) {
            EXPECT_TRUE(ctx.current == &g0_ || ctx.current == &c1_)
                << "seed " << k_seed << " log " << log.str();
        }
        EXPECT_EQ(ctx.search_root, search_root) << "seed " << k_seed << " log " << log.str();
    }
}

TEST_F(PudWitnessSearchTest, StoresCallerRepsBelowLvc) {
    expr caller_var{expr::var{1}};
    bind_hole(&caller_var, 2);
    pud_witness_search_context ctx{
        &query_leaf_, 0, 2, &a0_, &a0_};
    EXPECT_CALL(children_, get(&a0_)).WillRepeatedly(Return(std::nullopt));
    std::vector<uint32_t> stored;
    EXPECT_CALL(store_added_caller_reps_, store(&k_a0_, _))
        .WillOnce(SaveArg<1>(&stored));
    search_.resume(ctx);
    EXPECT_EQ(ctx.current, &a0_);
    EXPECT_EQ(stored, (std::vector<uint32_t>{1}));
}

TEST_F(PudWitnessSearchTest, DropsCalleeYieldsAtOrAboveLvc) {
    pud_witness_search_context ctx = make_edge(&a0_, &a0_);
    EXPECT_CALL(children_, get(&a0_)).WillRepeatedly(Return(std::nullopt));
    std::vector<uint32_t> stored;
    EXPECT_CALL(store_added_caller_reps_, store(&k_a0_, _))
        .WillOnce(SaveArg<1>(&stored));
    search_.resume(ctx);
    EXPECT_EQ(ctx.current, &a0_);
    EXPECT_TRUE(stored.empty());
}

TEST_F(PudWitnessSearchTest, ReplaysSnapshotLiftedToLvcNotCallerKey) {
    std::vector<pud_added_unification> unifs{{1, &const_a_}};
    ON_CALL(get_unifs_, get(&a0_)).WillByDefault(ReturnRef(unifs));
    pud_witness_search_context ctx{
        &query_leaf_, 0, 4, &a0_, &a0_};
    EXPECT_CALL(children_, get(&a0_)).WillRepeatedly(Return(std::nullopt));
    search_.resume(ctx);
    EXPECT_EQ(ctx.current, &a0_);
    const om_interval stored = intervals_.at(&k_a0_);
    const auto& by_var = recorded_[stored.open.rank_ptr()];
    EXPECT_FALSE(by_var.contains(1u));
    ASSERT_TRUE(by_var.contains(5u));
    EXPECT_EQ(by_var.at(5u).skeleton, &const_a_);
    EXPECT_EQ(by_var.at(5u).frame_offset, 4u);
}

TEST_F(PudWitnessSearchTest, QueryVsHeadWithRuleVarOneSucceeds) {
    expr q_x{expr::functor{1, {&var1_}}};
    expr q_a{expr::functor{1, {&const_a_}}};
    std::vector<pud_added_unification> unifs{{0, &q_x}};
    ON_CALL(get_unifs_, get(&a0_)).WillByDefault(ReturnRef(unifs));
    bind_hole(&q_a, 4);
    pud_witness_search_context ctx{
        &query_leaf_, 0, 4, &a0_, &a0_};
    EXPECT_CALL(children_, get(&a0_)).WillRepeatedly(Return(std::nullopt));
    search_.resume(ctx);
    EXPECT_EQ(ctx.current, &a0_);
}

TEST_F(PudWitnessSearchTest, InferenceMismatchPrunesLikeAxiom) {
    fail_nodes_.insert(&c0_);
    pud_witness_search_context ctx = make_edge(&a0_, &a0_);
    EXPECT_CALL(children_, get(&c0_)).WillRepeatedly(Return(std::nullopt));
    EXPECT_CALL(children_, get(&c1_)).WillRepeatedly(Return(std::nullopt));
    EXPECT_CALL(children_, get(&a0_)).WillRepeatedly(Return(children_set_t{&c0_, &c1_}));
    search_.resume(ctx);
    EXPECT_EQ(ctx.current, &c1_);
}

TEST_F(PudWitnessSearchTest, ParentUnifyFailureDoesNotEnterChild) {
    fail_nodes_.insert(&a0_);
    pud_witness_search_context ctx = make_edge(&a0_, &a0_);
    EXPECT_CALL(children_, get(&c0_)).WillRepeatedly(Return(std::nullopt));
    EXPECT_CALL(children_, get(&a0_)).WillRepeatedly(Return(children_set_t{&c0_}));
    search_.resume(ctx);
    EXPECT_EQ(ctx.current, nullptr);
    EXPECT_FALSE(stored_.contains(&k_c0_));
}

TEST_F(PudWitnessSearchTest, RecordsCalleeHeadAtLvcNotCallerZero) {
    pud_witness_search_context ctx = make_edge(&a0_, &a0_);
    EXPECT_CALL(children_, get(&a0_)).WillRepeatedly(Return(std::nullopt));
    search_.resume(ctx);
    const om_interval stored = intervals_.at(&k_a0_);
    const auto& by_var = recorded_[stored.open.rank_ptr()];
    EXPECT_FALSE(by_var.contains(0u));
    ASSERT_TRUE(by_var.contains(1u));
    EXPECT_EQ(by_var.at(1u).skeleton, &body_);
}
