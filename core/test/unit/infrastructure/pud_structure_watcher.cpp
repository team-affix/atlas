#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_structure_watcher.hpp"

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::Invoke;
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::UnorderedElementsAre;

namespace {

struct MockBindMap {
    MOCK_METHOD(ubs_class, find, (uint32_t));
};
struct MockMakeBindMap {
    MOCK_METHOD(MockBindMap&, make_bind_map, (const pud_ubs_state&));
};
struct MockGetNodeAddedSubstitutions {
    MOCK_METHOD(const std::vector<pud_substitution>&, get, (pud_node_id));
};

ubs_class singleton(uint32_t var) {
    return ubs_class{.root = var, .size = 1, .rep = var, .value = std::nullopt};
}

using test_watcher_t = pud_structure_watcher<NiceMock<MockMakeBindMap>, NiceMock<MockGetNodeAddedSubstitutions>>;

// Goals: 0 = f(X0, X1), 1 = g(X2), 2 = h(X3), with the child adding 3 = k(X4). The child's bindings
// are drawn on the bind map mock: every var is an unbound singleton unless a test says otherwise.
struct PudStructureWatcherTest : public ::testing::Test {
    NiceMock<MockBindMap> bind_map;
    NiceMock<MockMakeBindMap> make_bind_map;
    NiceMock<MockGetNodeAddedSubstitutions> substitutions;
    test_watcher_t watcher{make_bind_map, substitutions};
    expr x0{expr::var{0}}, x1{expr::var{1}}, x2{expr::var{2}}, x3{expr::var{3}}, x4{expr::var{4}};
    expr f{expr::functor{1, {&x0, &x1}}};
    expr g{expr::functor{2, {&x2}}};
    expr h{expr::functor{3, {&x3}}};
    expr k{expr::functor{4, {&x4}}};
    expr ground{expr::functor{5, {}}};
    pud_goal_list root_goals = pud_goal_list{.goals = {}, .next_id = 0}.adding({&f, &g, &h});
    std::vector<pud_substitution> child_substitutions;

    void SetUp() override {
        ON_CALL(make_bind_map, make_bind_map(_)).WillByDefault(ReturnRef(bind_map));
        ON_CALL(bind_map, find(_)).WillByDefault(Invoke(&singleton));
        ON_CALL(substitutions, get(9)).WillByDefault(ReturnRef(child_substitutions));
    }

    std::vector<pud_body_goal_id> changed_by(std::vector<pud_substitution> substituted) {
        child_substitutions = std::move(substituted);
        const pud_structure_watch root = watcher.watch_root(pud_ubs_state{}, root_goals);
        return watcher.descend(root, pud_ubs_state{}, 9, 2, root_goals.closing(2).adding({&k})).changed_goals;
    }
};

}

TEST_F(PudStructureWatcherTest, ARootWatchesEveryRepresentativeOfEveryGoal) {
    const pud_structure_watch watch = watcher.watch_root(pud_ubs_state{}, root_goals);
    EXPECT_EQ(watch.goal_to_reps[0], (immer::set<uint32_t>{}.insert(0).insert(1)));
    EXPECT_EQ(watch.goal_to_reps[1], (immer::set<uint32_t>{}.insert(2)));
}

TEST_F(PudStructureWatcherTest, NoSubstitutionChangesNoGoal) {
    EXPECT_THAT(changed_by({}), IsEmpty());
}

TEST_F(PudStructureWatcherTest, InstantiatingARepresentativeChangesTheGoalsThatMentionIt) {
    EXPECT_THAT(changed_by({pud_substitution{.var_idx = 2, .value = &ground}}), ElementsAre(1u));
}

TEST_F(PudStructureWatcherTest, RenamingToARepresentativeTheGoalDoesNotMentionChangesNothing) {
    EXPECT_THAT(changed_by({pud_substitution{.var_idx = 2, .value = &x1}}), IsEmpty());
}

TEST_F(PudStructureWatcherTest, MergingTwoRepresentativesOfOneGoalChangesIt) {
    EXPECT_THAT(changed_by({pud_substitution{.var_idx = 1, .value = &x0}}), ElementsAre(0u));
}

TEST_F(PudStructureWatcherTest, RenamingToABoundVariableInstantiates) {
    ON_CALL(bind_map, find(1)).WillByDefault(Return(ubs_class{.root = 1, .size = 2, .rep = 1, .value = framed_expr{&ground, 0}}));
    EXPECT_THAT(changed_by({pud_substitution{.var_idx = 2, .value = &x1}}), ElementsAre(1u));
}

TEST_F(PudStructureWatcherTest, AGoalChangedTwiceIsReportedOnce) {
    EXPECT_THAT(
        changed_by({pud_substitution{.var_idx = 0, .value = &ground}, pud_substitution{.var_idx = 1, .value = &ground}}),
        ElementsAre(0u));
}

TEST_F(PudStructureWatcherTest, TheClosedGoalIsNoLongerWatched) {
    EXPECT_THAT(changed_by({pud_substitution{.var_idx = 3, .value = &ground}}), IsEmpty());
}

TEST_F(PudStructureWatcherTest, AGoalTheChildAddsIsWatchedButNeverReportedChanged) {
    child_substitutions = {pud_substitution{.var_idx = 4, .value = &ground}};
    const pud_structure_watch root = watcher.watch_root(pud_ubs_state{}, root_goals);
    const pud_structure_watch_change change = watcher.descend(root, pud_ubs_state{}, 9, 2, root_goals.closing(2).adding({&k}));
    EXPECT_THAT(change.changed_goals, IsEmpty());
    EXPECT_EQ(change.watch.goal_to_reps[3], (immer::set<uint32_t>{}.insert(4)));
    EXPECT_EQ(change.watch.goal_to_reps.find(2), nullptr);
}

TEST_F(PudStructureWatcherTest, AnInstantiatedRepresentativeIsReplacedByTheValuesRepresentatives) {
    expr pair{expr::functor{6, {&x3, &x4}}};
    child_substitutions = {pud_substitution{.var_idx = 2, .value = &pair}};
    const pud_structure_watch root = watcher.watch_root(pud_ubs_state{}, root_goals);
    const pud_structure_watch_change change = watcher.descend(root, pud_ubs_state{}, 9, 2, root_goals.closing(2));
    EXPECT_EQ(change.watch.goal_to_reps[1], (immer::set<uint32_t>{}.insert(3).insert(4)));
}
