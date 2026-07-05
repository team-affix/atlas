// Integration: real sim orchestration slice (manifest wiring minus solver / random decisions).
// i_generate_decision is the only mocked collaborator — scripted resolutions keep runs deterministic.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <optional>
#include <string>
#include <vector>
#include "infrastructure/set_up_sim.hpp"
#include "infrastructure/tear_down_sim.hpp"
#include "infrastructure/run_sim.hpp"
#include "infrastructure/db.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "infrastructure/trail.hpp"
#include "infrastructure/bind_map.hpp"
#include "infrastructure/bind_map_factory.hpp"
#include "infrastructure/unifier_factory.hpp"
#include "infrastructure/lineage_pool.hpp"
#include "infrastructure/ra_active_goals.hpp"
#include "infrastructure/ra_rule_id_set_factory.hpp"
#include "infrastructure/goal_exprs.hpp"
#include "infrastructure/goal_candidate_rules.hpp"
#include "infrastructure/unit_goals.hpp"
#include "infrastructure/decision_memory.hpp"
#include "infrastructure/resolution_memory.hpp"
#include "infrastructure/candidate_frame_offsets.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/frame_bump_allocator.hpp"
#include "infrastructure/elimination_backlog.hpp"
#include "infrastructure/cdcl_elimination_generator.hpp"
#include "infrastructure/chosen_goal_candidates.hpp"
#include "infrastructure/mhu_elimination_generator.hpp"
#include "infrastructure/joint_elimination_generator.hpp"
#include "infrastructure/get_resolution_rule.hpp"
#include "infrastructure/conflict_detector.hpp"
#include "infrastructure/unit_goal_detector.hpp"
#include "infrastructure/solution_detector.hpp"
#include "infrastructure/goal_activator.hpp"
#include "infrastructure/goal_deactivator.hpp"
#include "infrastructure/candidate_activator.hpp"
#include "infrastructure/candidate_deactivator.hpp"
#include "infrastructure/elimination_router.hpp"
#include "infrastructure/get_unit_resolution.hpp"
#include "infrastructure/make_initial_goal_lineage.hpp"
#include "infrastructure/initial_goal_activator.hpp"
#include "infrastructure/goal_candidates_activator.hpp"
#include "infrastructure/goal_candidates_deactivator.hpp"
#include "infrastructure/subgoals_activator.hpp"
#include "infrastructure/initial_goals_activator.hpp"
#include "infrastructure/resolver.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/normalizer.hpp"
#include "value_objects/sim_termination.hpp"
#include "value_objects/lemma.hpp"
#include "functor_fixture.hpp"

using ::testing::IsEmpty;
using ::testing::Return;
using ::testing::UnorderedElementsAre;

struct MockGenerateDecision {
    MOCK_METHOD(const resolution_lineage*, generate, ());
};

namespace {

using unifier_factory_t            = unifier_factory<bind_map>;
using cdcl_t                      = cdcl_elimination_generator<chosen_goal_candidates>;
using mhu_t                       = mhu_elimination_generator<bind_map, bind_map_factory, unifier<bind_map>,
                                    unifier_factory_t, lineage_pool, expr_pool, goal_candidate_rules>;
using joint_t                     = joint_elimination_generator<cdcl_t, mhu_t>;
using get_resolution_rule_t         = get_resolution_rule<db>;
using conflict_detector_t          = conflict_detector<goal_candidate_rules>;
using unit_goal_detector_t          = unit_goal_detector<goal_candidate_rules>;
using solution_detector_t          = solution_detector<ra_active_goals>;
using goal_activator_t             = goal_activator<goal_exprs, goal_candidate_rules,
                                    ra_active_goals, candidate_frame_offsets, get_resolution_rule_t>;
using goal_deactivator_t           = goal_deactivator<goal_exprs, goal_candidate_rules, ra_active_goals>;
using candidate_deactivator_t      = candidate_deactivator<candidate_frame_offsets, goal_candidate_rules>;
using candidate_activator_t        = candidate_activator<frame_bump_allocator, candidate_frame_offsets,
                                    mhu_t, elimination_backlog, goal_exprs, db, goal_candidate_rules>;
using elimination_router_t         = elimination_router<goal_candidate_rules, ra_active_goals,
                                    elimination_backlog, candidate_deactivator_t>;
using get_unit_resolution_t         = get_unit_resolution<goal_candidate_rules, lineage_pool>;
using make_initial_goal_lineage_t    = make_initial_goal_lineage<lineage_pool>;
using initial_goal_activator_t      = initial_goal_activator<initial_goal_exprs,
                                    make_initial_goal_lineage_t, goal_exprs, goal_candidate_rules, ra_active_goals>;
using goal_candidates_deactivator_t = goal_candidates_deactivator<goal_candidate_rules,
                                    lineage_pool, candidate_deactivator_t>;
using goal_candidates_activator_t   = goal_candidates_activator<db, lineage_pool, candidate_activator_t,
                                    conflict_detector_t, unit_goal_detector_t, unit_goals>;
using subgoals_activator_t         = subgoals_activator<lineage_pool, goal_activator_t,
                                    db, goal_candidates_activator_t>;
using initial_goals_activator_t     = initial_goals_activator<initial_goal_exprs,
                                    initial_goal_activator_t, make_initial_goal_lineage_t, goal_candidates_activator_t>;
using resolver_t                  = resolver<goal_deactivator_t, subgoals_activator_t, goal_candidates_deactivator_t, chosen_goal_candidates>;
using set_up_sim_t  = set_up_sim<trail>;
using tear_down_sim_t  = tear_down_sim<trail, unit_goals, decision_memory, resolution_memory,
                    goal_candidate_rules, goal_exprs, ra_active_goals, candidate_frame_offsets,
                    mhu_t, bind_map, lineage_pool, frame_bump_allocator, cdcl_t, chosen_goal_candidates>;
using run_sim_t    = run_sim<initial_goals_activator_t, solution_detector_t, conflict_detector_t,
                    unit_goal_detector_t, unit_goals, unit_goals, MockGenerateDecision,
                    joint_t, elimination_router_t, resolver_t, get_unit_resolution_t,
                    decision_memory, resolution_memory, resolution_memory>;

struct sim_stack {
    db& database_;
    initial_goal_exprs& initial_goals_;

    globalizer globalizer_;
    trail trail_;
    bind_map bind_map_{globalizer_};
    bind_map_factory bind_map_factory_{globalizer_};
    unifier_factory_t unifier_factory_{globalizer_};
    lineage_pool lineage_pool_;
    ra_rule_id_set_factory ra_rule_id_set_factory_;
    ra_active_goals ra_active_goals_;
    goal_exprs goal_exprs_;
    goal_candidate_rules goal_candidate_rules_{ra_rule_id_set_factory_};
    unit_goals unit_goals_;
    decision_memory decision_memory_;
    resolution_memory resolution_memory_;
    candidate_frame_offsets candidate_frame_offsets_;
    chosen_goal_candidates chosen_goal_candidates_;

    expr_pool expr_pool_;
    frame_bump_allocator frame_allocator_{0};
    elimination_backlog elimination_backlog_{trail_};

    cdcl_t cdcl_{chosen_goal_candidates_};
    std::optional<mhu_t> mhu_;
    std::optional<joint_t> joint_;

    get_resolution_rule_t get_resolution_rule_{database_};
    conflict_detector_t conflict_detector_{goal_candidate_rules_};
    unit_goal_detector_t unit_goal_detector_{goal_candidate_rules_};
    solution_detector_t solution_detector_{ra_active_goals_};

    goal_activator_t goal_activator_{goal_exprs_, goal_candidate_rules_, ra_active_goals_,
                                   candidate_frame_offsets_, get_resolution_rule_};
    goal_deactivator_t goal_deactivator_{goal_exprs_, goal_candidate_rules_, ra_active_goals_};
    candidate_deactivator_t candidate_deactivator_{candidate_frame_offsets_, goal_candidate_rules_};
    std::optional<candidate_activator_t> candidate_activator_;

    elimination_router_t elimination_router_{goal_candidate_rules_, ra_active_goals_,
                                          elimination_backlog_, candidate_deactivator_};
    get_unit_resolution_t get_unit_resolution_{goal_candidate_rules_, lineage_pool_};
    make_initial_goal_lineage_t make_initial_goal_lineage_{lineage_pool_};
    std::optional<initial_goal_activator_t> initial_goal_activator_;

    std::optional<goal_candidates_deactivator_t> goal_candidates_deactivator_;
    std::optional<goal_candidates_activator_t> goal_candidates_activator_;
    std::optional<subgoals_activator_t> subgoals_activator_;
    std::optional<initial_goals_activator_t> initial_goals_activator_;
    std::optional<resolver_t> resolver_;

    testing::NiceMock<MockGenerateDecision> decision_generator;

    sim_stack(db& database_in, initial_goal_exprs& initial_goals_in)
        : database_(database_in), initial_goals_(initial_goals_in) {
        mhu_.emplace(bind_map_, lineage_pool_, expr_pool_, bind_map_factory_,
                     unifier_factory_, goal_candidate_rules_);
        joint_.emplace(cdcl_, *mhu_);
        candidate_activator_.emplace(frame_allocator_, candidate_frame_offsets_, *mhu_,
                                     elimination_backlog_, goal_exprs_, database_,
                                     goal_candidate_rules_);
        initial_goal_activator_.emplace(initial_goals_, make_initial_goal_lineage_,
                                        goal_exprs_, goal_candidate_rules_, ra_active_goals_);
        goal_candidates_deactivator_.emplace(goal_candidate_rules_, lineage_pool_,
                                             candidate_deactivator_);
        goal_candidates_activator_.emplace(database_, lineage_pool_, *candidate_activator_,
                                           conflict_detector_, unit_goal_detector_, unit_goals_);
        subgoals_activator_.emplace(lineage_pool_, goal_activator_, database_,
                                    *goal_candidates_activator_);
        initial_goals_activator_.emplace(initial_goals_, *initial_goal_activator_,
                                         make_initial_goal_lineage_, *goal_candidates_activator_);
        resolver_.emplace(goal_deactivator_, *subgoals_activator_, *goal_candidates_deactivator_,
                          chosen_goal_candidates_);
    }
};

const expr* make_suc_n(test_functors& functors, expr_pool& make_functor, const expr* zero, int n) {
    const expr* cur = zero;
    for (int i = 0; i < n; ++i)
        cur = make_functor.make_functor(functors.id("suc"), {cur});
    return cur;
}

void chain_clause_db(test_functors& functors, db& database, std::vector<expr>& storage, const char* prefix,
    size_t depth, const expr* ground_atom) {
    storage.reserve(depth * 3);
    for (size_t i = 0; i + 1 < depth; ++i) {
        storage.emplace_back(expr::var{0});
        const size_t x_idx = storage.size() - 1;
        storage.emplace_back(
            expr::functor{functors.id(std::string(prefix) + std::to_string(i)), {&storage[x_idx]}});
        storage.emplace_back(
            expr::functor{functors.id(std::string(prefix) + std::to_string(i + 1)), {&storage[x_idx]}});
        database.push(rule{&storage[storage.size() - 2], {&storage[storage.size() - 1]}});
    }
    storage.emplace_back(
        expr::functor{functors.id(std::string(prefix) + std::to_string(depth - 1)), {ground_atom}});
    database.push(rule{&storage.back(), {}});
}

}  // namespace

struct simulation {
    set_up_sim_t set_up_sim_;
    std::optional<tear_down_sim_t> tear_down_sim_;
    std::optional<run_sim_t> run_sim_;

    simulation(sim_stack& s, size_t max_resolutions)
        : set_up_sim_(s.trail_) {
        tear_down_sim_.emplace(s.trail_, s.unit_goals_, s.decision_memory_, s.resolution_memory_,
            s.goal_candidate_rules_, s.goal_exprs_, s.ra_active_goals_, s.candidate_frame_offsets_,
            *s.mhu_, s.bind_map_, s.lineage_pool_, s.frame_allocator_, s.cdcl_,
            s.chosen_goal_candidates_);
        run_sim_.emplace(*s.initial_goals_activator_, s.solution_detector_, s.conflict_detector_,
            s.unit_goal_detector_, s.unit_goals_, s.unit_goals_, s.decision_generator,
            *s.joint_, s.elimination_router_, *s.resolver_, s.get_unit_resolution_,
            s.decision_memory_, s.resolution_memory_, s.resolution_memory_, max_resolutions);
    }

    void set_up() { set_up_sim_.set_up(); }
    sim_termination run() { return run_sim_->run(); }
    void tear_down() { tear_down_sim_->tear_down(); }
};

struct SimIntegrationTest : public ::testing::Test {
    test_functors functors;
    static constexpr size_t kDefaultMaxResolutions = 8;

    db database;
    initial_goal_exprs initial_goals;
    expr_pool saved_expr_pool_;
    sim_stack stack{database, initial_goals};
};

TEST_F(SimIntegrationTest, RunWithNoInitialGoalsAndEmptyDbNeverGeneratesDecision) {
    /*
     * initial goals: (none)
     * database: (empty)
     */
    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    simulation simulation{stack, kDefaultMaxResolutions};
    simulation.set_up();
    EXPECT_EQ(simulation.run(), sim_termination::solved);
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsConflictedWhenInitialGoalHasNoDbCandidates) {
    /*
     * initial goals:
     *   f.
     * database: (empty)
     */
    expr goal{expr::functor{functors.id("f"), {}}};
    initial_goals.push(&goal);

    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    simulation simulation{stack, kDefaultMaxResolutions};
    simulation.set_up();
    EXPECT_EQ(simulation.run(), sim_termination::conflicted);
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedWhenUnitFactAppliesToInitialGoal) {
    /*
     * initial goals:
     *   f.
     * database:
     *   0: f.
     */
    expr goal{expr::functor{functors.id("f"), {}}};
    expr head{expr::functor{functors.id("f"), {}}};
    initial_goals.push(&goal);
    database.push(rule{&head, {}});

    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    simulation simulation{stack, kDefaultMaxResolutions};
    simulation.set_up();
    EXPECT_EQ(simulation.run(), sim_termination::solved);
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsConflictedWhenDbRuleHeadFailsToUnifyWithGoal) {
    /*
     * initial goals:
     *   f.
     * database:
     *   0: g.
     */
    expr goal{expr::functor{functors.id("f"), {}}};
    expr head{expr::functor{functors.id("g"), {}}};
    initial_goals.push(&goal);
    database.push(rule{&head, {}});

    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    simulation simulation{stack, kDefaultMaxResolutions};
    simulation.set_up();
    EXPECT_EQ(simulation.run(), sim_termination::conflicted);
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedWhenOnlyOneOfTwoDbRulesUnifiesWithGoal) {
    /*
     * initial goals:
     *   f.
     * database:
     *   0: f.
     *   1: g.
     */
    expr goal{expr::functor{functors.id("f"), {}}};
    expr matching_head{expr::functor{functors.id("f"), {}}};
    expr mismatching_head{expr::functor{functors.id("g"), {}}};
    initial_goals.push(&goal);
    database.push(rule{&matching_head, {}});
    database.push(rule{&mismatching_head, {}});

    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    simulation simulation{stack, kDefaultMaxResolutions};
    simulation.set_up();
    EXPECT_EQ(simulation.run(), sim_termination::solved);
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedAfterSingleDecisionWithTwoMatchingFacts) {
    /*
     * initial goals:
     *   f.
     * database:
     *   0: f.
     *   1: f.
     * setup: decision picks rule 1
     */
    expr goal{expr::functor{functors.id("f"), {}}};
    expr head0{expr::functor{functors.id("f"), {}}};
    expr head1{expr::functor{functors.id("f"), {}}};
    initial_goals.push(&goal);
    database.push(rule{&head0, {}});
    database.push(rule{&head1, {}});

    const goal_lineage* gl = stack.make_initial_goal_lineage_.make(0);
    const resolution_lineage* chosen =
        stack.lineage_pool_.make_resolution_lineage(gl, rule_id{1});
    EXPECT_CALL(stack.decision_generator, generate()).WillOnce(Return(chosen));

    simulation simulation{stack, kDefaultMaxResolutions};
    simulation.set_up();
    EXPECT_EQ(simulation.run(), sim_termination::solved);
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedViaClauseThenBodyFactWithoutDecisions) {
    /*
     * initial goals:
     *   f.
     * database:
     *   0: f :- g.
     *   1: g.
     */
    expr goal{expr::functor{functors.id("f"), {}}};
    expr f_head{expr::functor{functors.id("f"), {}}};
    expr g_body{expr::functor{functors.id("g"), {}}};
    expr g_head{expr::functor{functors.id("g"), {}}};
    initial_goals.push(&goal);
    database.push(rule{&f_head, {&g_body}});
    database.push(rule{&g_head, {}});

    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    simulation simulation{stack, kDefaultMaxResolutions};
    simulation.set_up();
    EXPECT_EQ(simulation.run(), sim_termination::solved);
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsConflictedWhenSecondInitialGoalHasNoCandidates) {
    /*
     * initial goals:
     *   f.
     *   g.
     * database:
     *   0: f.
     */
    expr goal_f{expr::functor{functors.id("f"), {}}};
    expr goal_g{expr::functor{functors.id("g"), {}}};
    expr f_head{expr::functor{functors.id("f"), {}}};
    initial_goals.push(&goal_f);
    initial_goals.push(&goal_g);
    database.push(rule{&f_head, {}});

    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    simulation simulation{stack, kDefaultMaxResolutions};
    simulation.set_up();
    EXPECT_EQ(simulation.run(), sim_termination::conflicted);
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedWhenTwoInitialGoalsEachHaveMatchingFacts) {
    /*
     * initial goals:
     *   f.
     *   g.
     * database:
     *   0: f.
     *   1: g.
     */
    expr goal_f{expr::functor{functors.id("f"), {}}};
    expr goal_g{expr::functor{functors.id("g"), {}}};
    expr f_head{expr::functor{functors.id("f"), {}}};
    expr g_head{expr::functor{functors.id("g"), {}}};
    initial_goals.push(&goal_f);
    initial_goals.push(&goal_g);
    database.push(rule{&f_head, {}});
    database.push(rule{&g_head, {}});

    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    simulation simulation{stack, kDefaultMaxResolutions};
    simulation.set_up();
    EXPECT_EQ(simulation.run(), sim_termination::solved);
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedAfterOneDecisionWhenInitialGoalFHasTwoMatchingFacts) {
    /*
     * initial goals:
     *   f.
     *   g.
     * database:
     *   0: f.
     *   1: f.
     *   2: g.
     * setup: decision picks rule 0 for goal f
     */
    expr goal_f{expr::functor{functors.id("f"), {}}};
    expr goal_g{expr::functor{functors.id("g"), {}}};
    expr f_head0{expr::functor{functors.id("f"), {}}};
    expr f_head1{expr::functor{functors.id("f"), {}}};
    expr g_head{expr::functor{functors.id("g"), {}}};
    initial_goals.push(&goal_f);
    initial_goals.push(&goal_g);
    database.push(rule{&f_head0, {}});
    database.push(rule{&f_head1, {}});
    database.push(rule{&g_head, {}});

    const goal_lineage* gl_f = stack.make_initial_goal_lineage_.make(0);
    const resolution_lineage* chosen =
        stack.lineage_pool_.make_resolution_lineage(gl_f, rule_id{0});
    EXPECT_CALL(stack.decision_generator, generate()).WillOnce(Return(chosen));

    simulation simulation{stack, kDefaultMaxResolutions};
    simulation.set_up();
    EXPECT_EQ(simulation.run(), sim_termination::solved);
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsConflictedWhenSecondInitialGoalLacksCandidatesDespiteTwoFFacts) {
    /*
     * initial goals:
     *   f.
     *   g.
     * database:
     *   0: f.
     *   1: f.
     */
    expr goal_f{expr::functor{functors.id("f"), {}}};
    expr goal_g{expr::functor{functors.id("g"), {}}};
    expr f_head0{expr::functor{functors.id("f"), {}}};
    expr f_head1{expr::functor{functors.id("f"), {}}};
    initial_goals.push(&goal_f);
    initial_goals.push(&goal_g);
    database.push(rule{&f_head0, {}});
    database.push(rule{&f_head1, {}});

    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    simulation simulation{stack, kDefaultMaxResolutions};
    simulation.set_up();
    EXPECT_EQ(simulation.run(), sim_termination::conflicted);
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedWhenRuleZeroIsBackloggedBeforeRun) {
    /*
     * initial goals:
     *   f.
     * database:
     *   0: f.
     *   1: f.
     * setup: rule 0 backlogged before run; expects resolution via rule 1 only
     */
    expr goal{expr::functor{functors.id("f"), {}}};
    expr f_head0{expr::functor{functors.id("f"), {}}};
    expr f_head1{expr::functor{functors.id("f"), {}}};
    initial_goals.push(&goal);
    database.push(rule{&f_head0, {}});
    database.push(rule{&f_head1, {}});

    auto& make_initial_goal_lineage =
        stack.make_initial_goal_lineage_;
    auto& make_resolution_lineage =
        stack.lineage_pool_;
    auto& insert_backlogged_elimination =
        stack.elimination_backlog_;
    auto& derive_resolution_lemma =
        stack.resolution_memory_;

    const goal_lineage* gl = make_initial_goal_lineage.make(0);
    const resolution_lineage* rl0 =
        make_resolution_lineage.make_resolution_lineage(gl, rule_id{0});
    const resolution_lineage* rl1 =
        make_resolution_lineage.make_resolution_lineage(gl, rule_id{1});

    insert_backlogged_elimination.insert_backlogged_elimination(rl0);

    simulation simulation{stack, kDefaultMaxResolutions};
    simulation.set_up();

    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    EXPECT_EQ(simulation.run(), sim_termination::solved);
    EXPECT_THAT(derive_resolution_lemma.derive_resolution_lemma().get_resolutions(),
        UnorderedElementsAre(rl1));
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedAfterCdclAvoidanceForcesG1RuleThree) {
    /*
     * initial goals:
     *   f.
     *   g.
     * database:
     *   0: f.
     *   1: f.
     *   2: g.
     *   3: g.
     * setup: CDCL avoidance {{goal f, rule 0}, {goal g, rule 2}}; decision picks f rule 0
     */
    expr goal_f{expr::functor{functors.id("f"), {}}};
    expr goal_g{expr::functor{functors.id("g"), {}}};
    expr f_head0{expr::functor{functors.id("f"), {}}};
    expr f_head1{expr::functor{functors.id("f"), {}}};
    expr g_head2{expr::functor{functors.id("g"), {}}};
    expr g_head3{expr::functor{functors.id("g"), {}}};
    initial_goals.push(&goal_f);
    initial_goals.push(&goal_g);
    database.push(rule{&f_head0, {}});
    database.push(rule{&f_head1, {}});
    database.push(rule{&g_head2, {}});
    database.push(rule{&g_head3, {}});

    auto& make_initial_goal_lineage =
        stack.make_initial_goal_lineage_;
    auto& make_resolution_lineage =
        stack.lineage_pool_;
    auto& learn_avoidance = stack.cdcl_;
    auto& derive_resolution_lemma =
        stack.resolution_memory_;

    const goal_lineage* gl0 = make_initial_goal_lineage.make(0);
    const goal_lineage* gl1 = make_initial_goal_lineage.make(1);
    const resolution_lineage* rl_g0_0 =
        make_resolution_lineage.make_resolution_lineage(gl0, rule_id{0});
    const resolution_lineage* rl_g1_2 =
        make_resolution_lineage.make_resolution_lineage(gl1, rule_id{2});
    const resolution_lineage* rl_g1_3 =
        make_resolution_lineage.make_resolution_lineage(gl1, rule_id{3});

    learn_avoidance.learn(lemma{{rl_g0_0, rl_g1_2}});

    simulation simulation{stack, kDefaultMaxResolutions};
    simulation.set_up();

    EXPECT_CALL(stack.decision_generator, generate()).WillOnce(Return(rl_g0_0));

    EXPECT_EQ(simulation.run(), sim_termination::solved);
    EXPECT_THAT(derive_resolution_lemma.derive_resolution_lemma().get_resolutions(),
        UnorderedElementsAre(rl_g0_0, rl_g1_3));
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedOnRecursiveClauseTreeWithoutDecisions) {
    /*
     * initial goals:
     *   f.
     * database:
     *   0: f :- g, h.
     *   1: g :- i, j.
     *   2: h :- i, j.
     *   3: i.
     *   4: j.
     */
    expr goal_f{expr::functor{functors.id("f"), {}}};
    expr f_head{expr::functor{functors.id("f"), {}}};
    expr g_body{expr::functor{functors.id("g"), {}}};
    expr h_body{expr::functor{functors.id("h"), {}}};
    expr g_head{expr::functor{functors.id("g"), {}}};
    expr h_head{expr::functor{functors.id("h"), {}}};
    expr i_body{expr::functor{functors.id("i"), {}}};
    expr j_body{expr::functor{functors.id("j"), {}}};
    expr i_head{expr::functor{functors.id("i"), {}}};
    expr j_head{expr::functor{functors.id("j"), {}}};
    initial_goals.push(&goal_f);
    database.push(rule{&f_head, {&g_body, &h_body}});
    database.push(rule{&g_head, {&i_body, &j_body}});
    database.push(rule{&h_head, {&i_body, &j_body}});
    database.push(rule{&i_head, {}});
    database.push(rule{&j_head, {}});

    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    simulation simulation{stack, kDefaultMaxResolutions};
    simulation.set_up();
    EXPECT_EQ(simulation.run(), sim_termination::solved);
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedAfterOneDecisionOnInitialGoalKWithTwoFacts) {
    /*
     * initial goals:
     *   f.
     *   g.
     *   k.
     * database:
     *   0: f :- g, h.
     *   1: g :- i, j.
     *   2: h :- i, j.
     *   3: i.
     *   4: j.
     *   5: k.
     *   6: k.
     * setup: decision picks rule 5 for goal k
     */
    expr goal_f{expr::functor{functors.id("f"), {}}};
    expr goal_g{expr::functor{functors.id("g"), {}}};
    expr goal_k{expr::functor{functors.id("k"), {}}};
    expr f_head{expr::functor{functors.id("f"), {}}};
    expr g_body{expr::functor{functors.id("g"), {}}};
    expr h_body{expr::functor{functors.id("h"), {}}};
    expr g_head{expr::functor{functors.id("g"), {}}};
    expr h_head{expr::functor{functors.id("h"), {}}};
    expr i_body{expr::functor{functors.id("i"), {}}};
    expr j_body{expr::functor{functors.id("j"), {}}};
    expr k_head0{expr::functor{functors.id("k"), {}}};
    expr k_head1{expr::functor{functors.id("k"), {}}};
    expr i_head{expr::functor{functors.id("i"), {}}};
    expr j_head{expr::functor{functors.id("j"), {}}};
    initial_goals.push(&goal_f);
    initial_goals.push(&goal_g);
    initial_goals.push(&goal_k);
    database.push(rule{&f_head, {&g_body, &h_body}});
    database.push(rule{&g_head, {&i_body, &j_body}});
    database.push(rule{&h_head, {&i_body, &j_body}});
    database.push(rule{&i_head, {}});
    database.push(rule{&j_head, {}});
    database.push(rule{&k_head0, {}});
    database.push(rule{&k_head1, {}});

    auto& make_initial_goal_lineage =
        stack.make_initial_goal_lineage_;
    auto& make_resolution_lineage =
        stack.lineage_pool_;

    const goal_lineage* gl_k = make_initial_goal_lineage.make(2);
    const resolution_lineage* rl_k_first =
        make_resolution_lineage.make_resolution_lineage(gl_k, rule_id{5});

    EXPECT_CALL(stack.decision_generator, generate()).WillOnce(Return(rl_k_first));

    static constexpr size_t kMaxResolutions = 32;
    simulation simulation{stack, kMaxResolutions};
    simulation.set_up();
    EXPECT_EQ(simulation.run(), sim_termination::solved);
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedAndBindsVarsViaMhuWhenFactUnifiesGoal) {
    /*
     * initial goals:
     *   f(A, B).
     * database:
     *   0: f(abc, 123).
     */
    expr abc{expr::functor{functors.id("abc"), {}}};
    expr _123{expr::functor{functors.id("123"), {}}};
    expr head{expr::functor{functors.id("f"), {&abc, &_123}}};
    database.push(rule{&head, {}});

    auto& bind_map = stack.bind_map_;
    auto& frame_alloc = stack.frame_allocator_;
    auto& make_var = stack.expr_pool_;
    auto& make_functor = stack.expr_pool_;

    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    simulation simulation{stack, kDefaultMaxResolutions};
    simulation.set_up();
    const uint32_t idx_a = frame_alloc.bump(1);
    const uint32_t idx_b = frame_alloc.bump(1);
    const expr* var_a = saved_expr_pool_.make_var(idx_a);
    const expr* var_b = saved_expr_pool_.make_var(idx_b);
    initial_goals.push(saved_expr_pool_.make_functor(functors.id("f"), {var_a, var_b}));

    EXPECT_EQ(simulation.run(), sim_termination::solved);
    const expr::functor& whnf_a = std::get<expr::functor>(bind_map.whnf({var_a, 0}).skeleton->content);
    const expr::functor& whnf_b = std::get<expr::functor>(bind_map.whnf({var_b, 0}).skeleton->content);
    EXPECT_EQ(whnf_a.id, functors.id("abc"));
    EXPECT_TRUE(whnf_a.args.empty());
    EXPECT_EQ(whnf_b.id, functors.id("123"));
    EXPECT_TRUE(whnf_b.args.empty());
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedViaClauseBodyFactsBindingVarsWithoutDecisions) {
    /*
     * initial goals:
     *   f(A, B).
     * database:
     *   0: f(C, D) :- g(C), h(D).
     *   1: g(abc).
     *   2: h(123).
     */
    expr rule_var_a{expr::var{0}};
    expr rule_var_b{expr::var{1}};
    expr abc{expr::functor{functors.id("abc"), {}}};
    expr _123{expr::functor{functors.id("123"), {}}};
    expr f_head{expr::functor{functors.id("f"), {&rule_var_a, &rule_var_b}}};
    expr g_body{expr::functor{functors.id("g"), {&rule_var_a}}};
    expr h_body{expr::functor{functors.id("h"), {&rule_var_b}}};
    expr g_head{expr::functor{functors.id("g"), {&abc}}};
    expr h_head{expr::functor{functors.id("h"), {&_123}}};
    database.push(rule{&f_head, {&g_body, &h_body}});
    database.push(rule{&g_head, {}});
    database.push(rule{&h_head, {}});

    auto& bind_map = stack.bind_map_;
    auto& frame_alloc = stack.frame_allocator_;
    auto& make_var = stack.expr_pool_;
    auto& make_functor = stack.expr_pool_;

    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    simulation simulation{stack, kDefaultMaxResolutions};
    simulation.set_up();
    const uint32_t idx_a = frame_alloc.bump(1);
    const uint32_t idx_b = frame_alloc.bump(1);
    const expr* var_a = saved_expr_pool_.make_var(idx_a);
    const expr* var_b = saved_expr_pool_.make_var(idx_b);
    initial_goals.push(saved_expr_pool_.make_functor(functors.id("f"), {var_a, var_b}));

    EXPECT_EQ(simulation.run(), sim_termination::solved);
    const expr::functor& whnf_a = std::get<expr::functor>(bind_map.whnf({var_a, 0}).skeleton->content);
    const expr::functor& whnf_b = std::get<expr::functor>(bind_map.whnf({var_b, 0}).skeleton->content);
    EXPECT_EQ(whnf_a.id, functors.id("abc"));
    EXPECT_TRUE(whnf_a.args.empty());
    EXPECT_EQ(whnf_b.id, functors.id("123"));
    EXPECT_TRUE(whnf_b.args.empty());
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedAfterDecisionBindingVarsFromChosenFact) {
    /*
     * initial goals:
     *   f(A, B).
     * database:
     *   0: f(abc, 123).
     *   1: f(xyz, 456).
     * setup: decision picks rule 1
     */
    expr abc{expr::functor{functors.id("abc"), {}}};
    expr xyz{expr::functor{functors.id("xyz"), {}}};
    expr _123{expr::functor{functors.id("123"), {}}};
    expr _456{expr::functor{functors.id("456"), {}}};
    expr head0{expr::functor{functors.id("f"), {&abc, &_123}}};
    expr head1{expr::functor{functors.id("f"), {&xyz, &_456}}};
    database.push(rule{&head0, {}});
    database.push(rule{&head1, {}});

    const goal_lineage* gl = stack.make_initial_goal_lineage_.make(0);
    const resolution_lineage* chosen =
        stack.lineage_pool_.make_resolution_lineage(gl, rule_id{1});

    auto& bind_map = stack.bind_map_;
    auto& frame_alloc = stack.frame_allocator_;
    auto& make_var = stack.expr_pool_;
    auto& make_functor = stack.expr_pool_;

    EXPECT_CALL(stack.decision_generator, generate()).WillOnce(Return(chosen));

    simulation simulation{stack, kDefaultMaxResolutions};
    simulation.set_up();
    const uint32_t idx_a = frame_alloc.bump(1);
    const uint32_t idx_b = frame_alloc.bump(1);
    const expr* var_a = saved_expr_pool_.make_var(idx_a);
    const expr* var_b = saved_expr_pool_.make_var(idx_b);
    initial_goals.push(saved_expr_pool_.make_functor(functors.id("f"), {var_a, var_b}));

    EXPECT_EQ(simulation.run(), sim_termination::solved);
    const expr::functor& whnf_a = std::get<expr::functor>(bind_map.whnf({var_a, 0}).skeleton->content);
    const expr::functor& whnf_b = std::get<expr::functor>(bind_map.whnf({var_b, 0}).skeleton->content);
    EXPECT_EQ(whnf_a.id, functors.id("xyz"));
    EXPECT_TRUE(whnf_a.args.empty());
    EXPECT_EQ(whnf_b.id, functors.id("456"));
    EXPECT_TRUE(whnf_b.args.empty());
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedWhenMhuRejectsInconsistentRuleWithoutDecision) {
    /*
     * initial goals:
     *   f(A, A).
     * database:
     *   0: f(abc, abc).
     *   1: f(abc, 123).
     */
    expr abc{expr::functor{functors.id("abc"), {}}};
    expr _123{expr::functor{functors.id("123"), {}}};
    expr head0{expr::functor{functors.id("f"), {&abc, &abc}}};
    expr head1{expr::functor{functors.id("f"), {&abc, &_123}}};
    database.push(rule{&head0, {}});
    database.push(rule{&head1, {}});

    auto& bind_map = stack.bind_map_;
    auto& frame_alloc = stack.frame_allocator_;
    auto& make_var = stack.expr_pool_;
    auto& make_functor = stack.expr_pool_;
    auto& make_initial_goal_lineage =
        stack.make_initial_goal_lineage_;
    auto& make_resolution_lineage =
        stack.lineage_pool_;
    auto& derive_resolution_lemma =
        stack.resolution_memory_;

    const goal_lineage* gl = make_initial_goal_lineage.make(0);
    const resolution_lineage* rl0 =
        make_resolution_lineage.make_resolution_lineage(gl, rule_id{0});

    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    simulation simulation{stack, kDefaultMaxResolutions};
    simulation.set_up();
    const uint32_t idx_a = frame_alloc.bump(1);
    const expr* var_a = saved_expr_pool_.make_var(idx_a);
    initial_goals.push(saved_expr_pool_.make_functor(functors.id("f"), {var_a, var_a}));

    EXPECT_EQ(simulation.run(), sim_termination::solved);
    const expr::functor& whnf_a = std::get<expr::functor>(bind_map.whnf({var_a, 0}).skeleton->content);
    EXPECT_EQ(whnf_a.id, functors.id("abc"));
    EXPECT_TRUE(whnf_a.args.empty());
    EXPECT_THAT(derive_resolution_lemma.derive_resolution_lemma().get_resolutions(),
        UnorderedElementsAre(rl0));
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunDeactivatesRuleOneWhenDecisionResolvesRuleZeroOnMhuSharedGoal) {
    /*
     * initial goals:
     *   f(A, B).
     * database:
     *   0: f(abc, 123).
     *   1: f(def, 123).
     * setup: decision picks rule 0; MHU constrain on rule 0 should purge rule 1 head
     */
    expr abc{expr::functor{functors.id("abc"), {}}};
    expr def{expr::functor{functors.id("def"), {}}};
    expr _123{expr::functor{functors.id("123"), {}}};
    expr head0{expr::functor{functors.id("f"), {&abc, &_123}}};
    expr head1{expr::functor{functors.id("f"), {&def, &_123}}};
    database.push(rule{&head0, {}});
    database.push(rule{&head1, {}});

    auto& make_initial_goal_lineage =
        stack.make_initial_goal_lineage_;
    auto& make_resolution_lineage =
        stack.lineage_pool_;
    auto& derive_resolution_lemma =
        stack.resolution_memory_;
    auto& bind_map = stack.bind_map_;
    auto& frame_alloc = stack.frame_allocator_;
    auto& make_var = stack.expr_pool_;
    auto& make_functor = stack.expr_pool_;

    const goal_lineage* gl = make_initial_goal_lineage.make(0);
    const resolution_lineage* rl0 =
        make_resolution_lineage.make_resolution_lineage(gl, rule_id{0});
    const resolution_lineage* rl1 =
        make_resolution_lineage.make_resolution_lineage(gl, rule_id{1});

    EXPECT_CALL(stack.decision_generator, generate()).WillOnce(Return(rl0));

    simulation simulation{stack, kDefaultMaxResolutions};
    simulation.set_up();
    const expr* var_a = saved_expr_pool_.make_var(frame_alloc.bump(1));
    const expr* var_b = saved_expr_pool_.make_var(frame_alloc.bump(1));
    initial_goals.push(saved_expr_pool_.make_functor(functors.id("f"), {var_a, var_b}));

    EXPECT_EQ(simulation.run(), sim_termination::solved);
    EXPECT_THAT(derive_resolution_lemma.derive_resolution_lemma().get_resolutions(),
        UnorderedElementsAre(rl0));
    const expr::functor& whnf_a = std::get<expr::functor>(bind_map.whnf({var_a, 0}).skeleton->content);
    const expr::functor& whnf_b = std::get<expr::functor>(bind_map.whnf({var_b, 0}).skeleton->content);
    EXPECT_EQ(whnf_a.id, functors.id("abc"));
    EXPECT_EQ(whnf_b.id, functors.id("123"));
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunDeactivatesCrossGoalCandidateOnMhuIncompatibleHead) {
    /*
     * initial goals (two subgoals, shared var A):
     *   f(A).
     *   g(A).
     * database:
     *   0: f(abc).
     *   1: f(xyz).
     *   2: g(def).
     *   3: g(abc).
     * setup: both goals non-unit; decision picks goal 0 rule 0; MHU eliminates goal 1 rule 2
     */
    expr abc{expr::functor{functors.id("abc"), {}}};
    expr xyz{expr::functor{functors.id("xyz"), {}}};
    expr def{expr::functor{functors.id("def"), {}}};
    expr head_f0{expr::functor{functors.id("f"), {&abc}}};
    expr head_f1{expr::functor{functors.id("f"), {&xyz}}};
    expr head_g2{expr::functor{functors.id("g"), {&def}}};
    expr head_g3{expr::functor{functors.id("g"), {&abc}}};
    database.push(rule{&head_f0, {}});
    database.push(rule{&head_f1, {}});
    database.push(rule{&head_g2, {}});
    database.push(rule{&head_g3, {}});

    auto& make_initial_goal_lineage =
        stack.make_initial_goal_lineage_;
    auto& make_resolution_lineage =
        stack.lineage_pool_;
    auto& bind_map = stack.bind_map_;
    auto& frame_alloc = stack.frame_allocator_;
    auto& make_var = stack.expr_pool_;
    auto& make_functor = stack.expr_pool_;

    const goal_lineage* gl0 = make_initial_goal_lineage.make(0);
    const resolution_lineage* rl_f0 =
        make_resolution_lineage.make_resolution_lineage(gl0, rule_id{0});

    EXPECT_CALL(stack.decision_generator, generate()).WillOnce(Return(rl_f0));

    simulation simulation{stack, kDefaultMaxResolutions};
    simulation.set_up();
    const expr* var_a = saved_expr_pool_.make_var(frame_alloc.bump(1));
    initial_goals.push(saved_expr_pool_.make_functor(functors.id("f"), {var_a}));
    initial_goals.push(saved_expr_pool_.make_functor(functors.id("g"), {var_a}));

    EXPECT_EQ(simulation.run(), sim_termination::solved);
    const expr::functor& whnf_a = std::get<expr::functor>(bind_map.whnf({var_a, 0}).skeleton->content);
    EXPECT_EQ(whnf_a.id, functors.id("abc"));
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedBindingVarInNestedFunctorArgWithoutDecisions) {
    /*
     * initial goals:
     *   f(g(A)).
     * database:
     *   0: f(g(abc)).
     */
    expr abc{expr::functor{functors.id("abc"), {}}};
    expr g_abc{expr::functor{functors.id("g"), {&abc}}};
    expr head{expr::functor{functors.id("f"), {&g_abc}}};
    database.push(rule{&head, {}});

    auto& bind_map = stack.bind_map_;
    auto& frame_alloc = stack.frame_allocator_;
    auto& make_var = stack.expr_pool_;
    auto& make_functor = stack.expr_pool_;

    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    simulation simulation{stack, kDefaultMaxResolutions};
    simulation.set_up();
    const uint32_t idx_a = frame_alloc.bump(1);
    const expr* var_a = saved_expr_pool_.make_var(idx_a);
    const expr* g_a = saved_expr_pool_.make_functor(functors.id("g"), {var_a});
    initial_goals.push(saved_expr_pool_.make_functor(functors.id("f"), {g_a}));

    EXPECT_EQ(simulation.run(), sim_termination::solved);
    const expr::functor& whnf_a = std::get<expr::functor>(bind_map.whnf({var_a, 0}).skeleton->content);
    EXPECT_EQ(whnf_a.id, functors.id("abc"));
    EXPECT_TRUE(whnf_a.args.empty());
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedBindingRemainingVarWhenGoalIsPartiallyGround) {
    /*
     * initial goals:
     *   f(abc, B).
     * database:
     *   0: f(abc, 123).
     */
    expr abc{expr::functor{functors.id("abc"), {}}};
    expr _123{expr::functor{functors.id("123"), {}}};
    expr head{expr::functor{functors.id("f"), {&abc, &_123}}};
    database.push(rule{&head, {}});

    auto& bind_map = stack.bind_map_;
    auto& frame_alloc = stack.frame_allocator_;
    auto& make_var = stack.expr_pool_;
    auto& make_functor = stack.expr_pool_;

    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    simulation simulation{stack, kDefaultMaxResolutions};
    simulation.set_up();
    const expr* abc_pool = saved_expr_pool_.make_functor(functors.id("abc"), {});
    const uint32_t idx_b = frame_alloc.bump(1);
    const expr* var_b = saved_expr_pool_.make_var(idx_b);
    initial_goals.push(saved_expr_pool_.make_functor(functors.id("f"), {abc_pool, var_b}));

    EXPECT_EQ(simulation.run(), sim_termination::solved);
    const expr::functor& whnf_b = std::get<expr::functor>(bind_map.whnf({var_b, 0}).skeleton->content);
    EXPECT_EQ(whnf_b.id, functors.id("123"));
    EXPECT_TRUE(whnf_b.args.empty());
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsConflictedWhenClauseBodyGoalHasNoUnifyingFact) {
    /*
     * initial goals:
     *   f(A, B).
     * database:
     *   0: f(C, D) :- g(C), h(D).
     *   1: g(abc).
     */
    expr rule_var_a{expr::var{0}};
    expr rule_var_b{expr::var{1}};
    expr abc{expr::functor{functors.id("abc"), {}}};
    expr f_head{expr::functor{functors.id("f"), {&rule_var_a, &rule_var_b}}};
    expr g_body{expr::functor{functors.id("g"), {&rule_var_a}}};
    expr h_body{expr::functor{functors.id("h"), {&rule_var_b}}};
    expr g_head{expr::functor{functors.id("g"), {&abc}}};
    database.push(rule{&f_head, {&g_body, &h_body}});
    database.push(rule{&g_head, {}});

    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    auto& frame_alloc = stack.frame_allocator_;
    auto& make_var = stack.expr_pool_;
    auto& make_functor = stack.expr_pool_;

    simulation simulation{stack, kDefaultMaxResolutions};
    simulation.set_up();
    const expr* var_a = saved_expr_pool_.make_var(frame_alloc.bump(1));
    const expr* var_b = saved_expr_pool_.make_var(frame_alloc.bump(1));
    initial_goals.push(saved_expr_pool_.make_functor(functors.id("f"), {var_a, var_b}));

    EXPECT_EQ(simulation.run(), sim_termination::conflicted);
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsConflictedWhenClauseBodyFactMismatchesGroundGoal) {
    /*
     * initial goals:
     *   f(abc, 123).
     * database:
     *   0: f(A, B) :- g(A), h(B).
     *   1: g(abc).
     *   2: h(234).
     */
    expr rule_var_a{expr::var{0}};
    expr rule_var_b{expr::var{1}};
    expr abc{expr::functor{functors.id("abc"), {}}};
    expr two_three_four{expr::functor{functors.id("234"), {}}};
    expr f_head{expr::functor{functors.id("f"), {&rule_var_a, &rule_var_b}}};
    expr g_body{expr::functor{functors.id("g"), {&rule_var_a}}};
    expr h_body{expr::functor{functors.id("h"), {&rule_var_b}}};
    expr g_head{expr::functor{functors.id("g"), {&abc}}};
    expr h_head{expr::functor{functors.id("h"), {&two_three_four}}};
    database.push(rule{&f_head, {&g_body, &h_body}});
    database.push(rule{&g_head, {}});
    database.push(rule{&h_head, {}});

    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    auto& make_functor = stack.expr_pool_;

    simulation simulation{stack, kDefaultMaxResolutions};
    simulation.set_up();
    initial_goals.push(saved_expr_pool_.make_functor(functors.id("f"),
        {saved_expr_pool_.make_functor(functors.id("abc"), {}), saved_expr_pool_.make_functor(functors.id("123"), {})}));

    EXPECT_EQ(simulation.run(), sim_termination::conflicted);
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedAfterDecisionOnBodyGoalWithTwoFacts) {
    /*
     * initial goals:
     *   f(A, B).
     * database:
     *   0: f(C, D) :- g(C), h(D).
     *   1: g(abc).
     *   2: g(xyz).
     *   3: h(123).
     * setup: decision picks rule 2 for subgoal g
     */
    expr rule_var_a{expr::var{0}};
    expr rule_var_b{expr::var{1}};
    expr abc{expr::functor{functors.id("abc"), {}}};
    expr xyz{expr::functor{functors.id("xyz"), {}}};
    expr _123{expr::functor{functors.id("123"), {}}};
    expr f_head{expr::functor{functors.id("f"), {&rule_var_a, &rule_var_b}}};
    expr g_body{expr::functor{functors.id("g"), {&rule_var_a}}};
    expr h_body{expr::functor{functors.id("h"), {&rule_var_b}}};
    expr g_head0{expr::functor{functors.id("g"), {&abc}}};
    expr g_head1{expr::functor{functors.id("g"), {&xyz}}};
    expr h_head{expr::functor{functors.id("h"), {&_123}}};
    database.push(rule{&f_head, {&g_body, &h_body}});
    database.push(rule{&g_head0, {}});
    database.push(rule{&g_head1, {}});
    database.push(rule{&h_head, {}});

    auto& make_initial_goal_lineage =
        stack.make_initial_goal_lineage_;
    auto& make_resolution_lineage =
        stack.lineage_pool_;
    auto& make_goal_lineage = stack.lineage_pool_;
    auto& bind_map = stack.bind_map_;
    auto& frame_alloc = stack.frame_allocator_;
    auto& make_var = stack.expr_pool_;
    auto& make_functor = stack.expr_pool_;

    const goal_lineage* gl_f = make_initial_goal_lineage.make(0);
    const resolution_lineage* rl_f =
        make_resolution_lineage.make_resolution_lineage(gl_f, rule_id{0});
    const goal_lineage* gl_g = make_goal_lineage.make_goal_lineage(rl_f, subgoal_id{0});
    const resolution_lineage* chosen_g =
        make_resolution_lineage.make_resolution_lineage(gl_g, rule_id{2});

    EXPECT_CALL(stack.decision_generator, generate()).WillOnce(Return(chosen_g));

    simulation simulation{stack, kDefaultMaxResolutions};
    simulation.set_up();
    const expr* var_a = saved_expr_pool_.make_var(frame_alloc.bump(1));
    const expr* var_b = saved_expr_pool_.make_var(frame_alloc.bump(1));
    initial_goals.push(saved_expr_pool_.make_functor(functors.id("f"), {var_a, var_b}));

    EXPECT_EQ(simulation.run(), sim_termination::solved);
    const expr::functor& whnf_a = std::get<expr::functor>(bind_map.whnf({var_a, 0}).skeleton->content);
    const expr::functor& whnf_b = std::get<expr::functor>(bind_map.whnf({var_b, 0}).skeleton->content);
    EXPECT_EQ(whnf_a.id, functors.id("xyz"));
    EXPECT_TRUE(whnf_a.args.empty());
    EXPECT_EQ(whnf_b.id, functors.id("123"));
    EXPECT_TRUE(whnf_b.args.empty());
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedWhenSharedVarLinksTwoGoalsWithoutDecisions) {
    /*
     * initial goals:
     *   f(A, B).
     *   g(B, C).
     * database:
     *   0: f(abc, 123).
     *   1: g(456, 789).
     *   2: g(123, xyz).
     */
    expr abc{expr::functor{functors.id("abc"), {}}};
    expr _123{expr::functor{functors.id("123"), {}}};
    expr _456{expr::functor{functors.id("456"), {}}};
    expr _789{expr::functor{functors.id("789"), {}}};
    expr xyz{expr::functor{functors.id("xyz"), {}}};
    expr f_head{expr::functor{functors.id("f"), {&abc, &_123}}};
    expr g_head0{expr::functor{functors.id("g"), {&_456, &_789}}};
    expr g_head1{expr::functor{functors.id("g"), {&_123, &xyz}}};
    database.push(rule{&f_head, {}});
    database.push(rule{&g_head0, {}});
    database.push(rule{&g_head1, {}});

    auto& bind_map = stack.bind_map_;
    auto& frame_alloc = stack.frame_allocator_;
    auto& make_var = stack.expr_pool_;
    auto& make_functor = stack.expr_pool_;

    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    simulation simulation{stack, kDefaultMaxResolutions};
    simulation.set_up();
    const expr* var_a = saved_expr_pool_.make_var(frame_alloc.bump(1));
    const expr* var_b = saved_expr_pool_.make_var(frame_alloc.bump(1));
    const expr* var_c = saved_expr_pool_.make_var(frame_alloc.bump(1));
    initial_goals.push(saved_expr_pool_.make_functor(functors.id("f"), {var_a, var_b}));
    initial_goals.push(saved_expr_pool_.make_functor(functors.id("g"), {var_b, var_c}));

    EXPECT_EQ(simulation.run(), sim_termination::solved);
    const expr::functor& whnf_a = std::get<expr::functor>(bind_map.whnf({var_a, 0}).skeleton->content);
    const expr::functor& whnf_b = std::get<expr::functor>(bind_map.whnf({var_b, 0}).skeleton->content);
    const expr::functor& whnf_c = std::get<expr::functor>(bind_map.whnf({var_c, 0}).skeleton->content);
    EXPECT_EQ(whnf_a.id, functors.id("abc"));
    EXPECT_TRUE(whnf_a.args.empty());
    EXPECT_EQ(whnf_b.id, functors.id("123"));
    EXPECT_TRUE(whnf_b.args.empty());
    EXPECT_EQ(whnf_c.id, functors.id("xyz"));
    EXPECT_TRUE(whnf_c.args.empty());
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedWhenCdclAndMhuReduceGoalGCandidatesWithoutDecisions) {
    /*
     * initial goals:
     *   f.
     *   g(A, xyz).
     * database:
     *   0: f.
     *   1: g(abc, xyz).
     *   2: g(def, xyz).
     *   3: g(ghi, jkl).
     * setup: CDCL avoidance {{goal f, rule 0}, {goal g, rule 1}}
     */
    expr abc{expr::functor{functors.id("abc"), {}}};
    expr def{expr::functor{functors.id("def"), {}}};
    expr ghi{expr::functor{functors.id("ghi"), {}}};
    expr _xyz{expr::functor{functors.id("xyz"), {}}};
    expr _jkl{expr::functor{functors.id("jkl"), {}}};
    expr f_head{expr::functor{functors.id("f"), {}}};
    expr g_head1{expr::functor{functors.id("g"), {&abc, &_xyz}}};
    expr g_head2{expr::functor{functors.id("g"), {&def, &_xyz}}};
    expr g_head3{expr::functor{functors.id("g"), {&ghi, &_jkl}}};
    database.push(rule{&f_head, {}});
    database.push(rule{&g_head1, {}});
    database.push(rule{&g_head2, {}});
    database.push(rule{&g_head3, {}});

    auto& make_initial_goal_lineage =
        stack.make_initial_goal_lineage_;
    auto& make_resolution_lineage =
        stack.lineage_pool_;
    auto& learn_avoidance = stack.cdcl_;
    auto& derive_resolution_lemma =
        stack.resolution_memory_;
    auto& bind_map = stack.bind_map_;
    auto& frame_alloc = stack.frame_allocator_;
    auto& make_var = stack.expr_pool_;
    auto& make_functor = stack.expr_pool_;

    const goal_lineage* gl_f = make_initial_goal_lineage.make(0);
    const goal_lineage* gl_g = make_initial_goal_lineage.make(1);
    const resolution_lineage* rl_f_0 =
        make_resolution_lineage.make_resolution_lineage(gl_f, rule_id{0});
    const resolution_lineage* rl_g_1 =
        make_resolution_lineage.make_resolution_lineage(gl_g, rule_id{1});
    const resolution_lineage* rl_g_2 =
        make_resolution_lineage.make_resolution_lineage(gl_g, rule_id{2});

    learn_avoidance.learn(lemma{{rl_f_0, rl_g_1}});

    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    simulation simulation{stack, kDefaultMaxResolutions};
    simulation.set_up();
    const expr* var_a = saved_expr_pool_.make_var(frame_alloc.bump(1));
    const expr* xyz = saved_expr_pool_.make_functor(functors.id("xyz"), {});
    initial_goals.push(saved_expr_pool_.make_functor(functors.id("f"), {}));
    initial_goals.push(saved_expr_pool_.make_functor(functors.id("g"), {var_a, xyz}));

    EXPECT_EQ(simulation.run(), sim_termination::solved);
    EXPECT_THAT(derive_resolution_lemma.derive_resolution_lemma().get_resolutions(),
        UnorderedElementsAre(rl_f_0, rl_g_2));
    const expr::functor& whnf_a = std::get<expr::functor>(bind_map.whnf({var_a, 0}).skeleton->content);
    EXPECT_EQ(whnf_a.id, functors.id("def"));
    EXPECT_TRUE(whnf_a.args.empty());
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedBuildingListOfFiveAbcWithoutDecisions) {
    /*
     * initial goals:
     *   make_list(suc(suc(suc(suc(suc(zero))))), abc, R).
     * database:
     *   0: make_list(zero, _, nil).
     *   1: make_list(suc(L), A, cons(A, T)) :- make_list(L, A, T).
     */
    expr rule_ignored{expr::var{0}};
    expr rule_l{expr::var{0}};
    expr rule_a{expr::var{1}};
    expr rule_t{expr::var{2}};
    expr zero{expr::functor{functors.id("zero"), {}}};
    expr nil{expr::functor{functors.id("nil"), {}}};
    expr head0{expr::functor{functors.id("make_list"), {&zero, &rule_ignored, &nil}}};
    expr suc_l{expr::functor{functors.id("suc"), {&rule_l}}};
    expr cons_at{expr::functor{functors.id("cons"), {&rule_a, &rule_t}}};
    expr head1{expr::functor{functors.id("make_list"), {&suc_l, &rule_a, &cons_at}}};
    expr body1{expr::functor{functors.id("make_list"), {&rule_l, &rule_a, &rule_t}}};
    database.push(rule{&head0, {}});
    database.push(rule{&head1, {&body1}});

    auto& bind_map = stack.bind_map_;
    auto& frame_alloc = stack.frame_allocator_;
    auto& make_var = stack.expr_pool_;
    auto& make_functor = stack.expr_pool_;

    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    static constexpr size_t kMaxResolutions = 32;
    static constexpr int kListLength = 5;
    simulation simulation{stack, kMaxResolutions};
    simulation.set_up();
    const expr* zero_pool = saved_expr_pool_.make_functor(functors.id("zero"), {});
    const expr* len = zero_pool;
    for (int i = 0; i < kListLength; ++i)
        len = saved_expr_pool_.make_functor(functors.id("suc"), {len});
    const expr* abc = saved_expr_pool_.make_functor(functors.id("abc"), {});
    const expr* var_r = saved_expr_pool_.make_var(frame_alloc.bump(1));
    initial_goals.push(saved_expr_pool_.make_functor(functors.id("make_list"), {len, abc, var_r}));

    EXPECT_EQ(simulation.run(), sim_termination::solved);

    normalizer<globalizer, expr_pool, expr_pool, decltype(stack.bind_map_)> norm{stack.globalizer_, stack.expr_pool_, stack.expr_pool_, stack.bind_map_};
    const expr* tail = norm.normalize({var_r, 0});
    for (int i = 0; i < kListLength; ++i) {
        const expr::functor& cell = std::get<expr::functor>(tail->content);
        ASSERT_EQ(cell.id, functors.id("cons"));
        ASSERT_EQ(cell.args.size(), 2u);
        const expr::functor& head =
            std::get<expr::functor>(norm.normalize({cell.args[0], 0})->content);
        EXPECT_EQ(head.id, functors.id("abc"));
        EXPECT_TRUE(head.args.empty());
        tail = norm.normalize({cell.args[1], 0});
    }
    const expr::functor& nil_tail = std::get<expr::functor>(norm.normalize({tail, 0})->content);
    EXPECT_EQ(nil_tail.id, functors.id("nil"));
    EXPECT_TRUE(nil_tail.args.empty());

    simulation.tear_down();
}

// ---------------------------------------------------------------------------
// round 2 gaps — depth, multi-decision, CDCL mid-loop conflict, partial progress
// ---------------------------------------------------------------------------

TEST_F(SimIntegrationTest, RunReturnsConflictedWhenCdclEliminationExhaustsSiblingGoal) {
    /*
     * initial goals:
     *   f.
     *   g.
     * database:
     *   0: f.
     *   1: f.
     *   2: g.
     *   3: g.
     * setup: two avoidances sharing f/0 eliminate both g candidates on constrain(f/0);
     *        conflict mirror of RunReturnsSolvedAfterCdclAvoidanceForcesG1RuleThree
     */
    expr goal_f{expr::functor{functors.id("f"), {}}};
    expr goal_g{expr::functor{functors.id("g"), {}}};
    expr f_head0{expr::functor{functors.id("f"), {}}};
    expr f_head1{expr::functor{functors.id("f"), {}}};
    expr g_head0{expr::functor{functors.id("g"), {}}};
    expr g_head1{expr::functor{functors.id("g"), {}}};
    initial_goals.push(&goal_f);
    initial_goals.push(&goal_g);
    database.push(rule{&f_head0, {}});
    database.push(rule{&f_head1, {}});
    database.push(rule{&g_head0, {}});
    database.push(rule{&g_head1, {}});

    auto& make_initial_goal_lineage =
        stack.make_initial_goal_lineage_;
    auto& make_resolution_lineage =
        stack.lineage_pool_;
    auto& learn_avoidance = stack.cdcl_;
    auto& derive_resolution_lemma =
        stack.resolution_memory_;
    auto& get_resolution_count =
        stack.resolution_memory_;

    const goal_lineage* gl_f = make_initial_goal_lineage.make(0);
    const goal_lineage* gl_g = make_initial_goal_lineage.make(1);
    const resolution_lineage* rl_f_0 =
        make_resolution_lineage.make_resolution_lineage(gl_f, rule_id{0});
    const resolution_lineage* rl_g_0 =
        make_resolution_lineage.make_resolution_lineage(gl_g, rule_id{2});
    const resolution_lineage* rl_g_1 =
        make_resolution_lineage.make_resolution_lineage(gl_g, rule_id{3});

    learn_avoidance.learn(lemma{{rl_f_0, rl_g_0}});
    learn_avoidance.learn(lemma{{rl_f_0, rl_g_1}});

    simulation simulation{stack, kDefaultMaxResolutions};
    simulation.set_up();

    EXPECT_CALL(stack.decision_generator, generate()).WillOnce(Return(rl_f_0));

    EXPECT_EQ(simulation.run(), sim_termination::conflicted);
    EXPECT_EQ(get_resolution_count.get_resolution_count(), 1u);
    EXPECT_THAT(derive_resolution_lemma.derive_resolution_lemma().get_resolutions(),
        UnorderedElementsAre(rl_f_0));
    simulation.tear_down();
}

// Integration counterpart of unit RunReturnsDepthExceededWhenNoSolutionWithinLimit.
TEST_F(SimIntegrationTest, RunReturnsDepthExceededOnSelfRecursiveClause) {
    /*
     * initial goals:
     *   f.
     * database:
     *   0: f :- f.
     */
    expr goal{expr::functor{functors.id("f"), {}}};
    expr f_head{expr::functor{functors.id("f"), {}}};
    expr f_body{expr::functor{functors.id("f"), {}}};
    initial_goals.push(&goal);
    database.push(rule{&f_head, {&f_body}});

    static constexpr size_t kMaxResolutions = 4;
    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    simulation simulation{stack, kMaxResolutions};
    simulation.set_up();
    EXPECT_EQ(simulation.run(), sim_termination::depth_exceeded);
    EXPECT_EQ(stack.resolution_memory_.get_resolution_count(), kMaxResolutions);
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedAfterTwoSequentialDecisions) {
    /*
     * initial goals:
     *   f.
     *   g.
     * database:
     *   0: f.
     *   1: f.
     *   2: g.
     *   3: g.
     * setup: second decision on g after f branch committed
     */
    expr goal_f{expr::functor{functors.id("f"), {}}};
    expr goal_g{expr::functor{functors.id("g"), {}}};
    expr f_head0{expr::functor{functors.id("f"), {}}};
    expr f_head1{expr::functor{functors.id("f"), {}}};
    expr g_head0{expr::functor{functors.id("g"), {}}};
    expr g_head1{expr::functor{functors.id("g"), {}}};
    initial_goals.push(&goal_f);
    initial_goals.push(&goal_g);
    database.push(rule{&f_head0, {}});
    database.push(rule{&f_head1, {}});
    database.push(rule{&g_head0, {}});
    database.push(rule{&g_head1, {}});

    auto& make_initial_goal_lineage =
        stack.make_initial_goal_lineage_;
    auto& make_resolution_lineage =
        stack.lineage_pool_;
    auto& derive_resolution_lemma =
        stack.resolution_memory_;
    auto& get_decision_count =
        stack.decision_memory_;

    const goal_lineage* gl_f = make_initial_goal_lineage.make(0);
    const goal_lineage* gl_g = make_initial_goal_lineage.make(1);
    const resolution_lineage* rl_f_0 =
        make_resolution_lineage.make_resolution_lineage(gl_f, rule_id{0});
    const resolution_lineage* rl_g_1 =
        make_resolution_lineage.make_resolution_lineage(gl_g, rule_id{3});

    // Script decision choices only; behavioral contract is termination + recorded decisions/resolutions.
    EXPECT_CALL(stack.decision_generator, generate())
        .WillOnce(Return(rl_f_0))
        .WillOnce(Return(rl_g_1));

    simulation simulation{stack, kDefaultMaxResolutions};
    simulation.set_up();
    EXPECT_EQ(simulation.run(), sim_termination::solved);
    EXPECT_EQ(get_decision_count.count(), 2u);
    EXPECT_THAT(derive_resolution_lemma.derive_resolution_lemma().get_resolutions(),
        UnorderedElementsAre(rl_f_0, rl_g_1));
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedWhenCdclUnitElimForcesRemainingCandidate) {
    /*
     * initial goals:
     *   f.
     *   g.
     * database:
     *   0: f.
     *   1: f.
     *   2: g.
     *   3: g.
     * setup: CDCL unit elim g/2 on constrain(f/0) leaves g unit on g/3; no second decision
     */
    expr goal_f{expr::functor{functors.id("f"), {}}};
    expr goal_g{expr::functor{functors.id("g"), {}}};
    expr f_head0{expr::functor{functors.id("f"), {}}};
    expr f_head1{expr::functor{functors.id("f"), {}}};
    expr g_head0{expr::functor{functors.id("g"), {}}};
    expr g_head1{expr::functor{functors.id("g"), {}}};
    initial_goals.push(&goal_f);
    initial_goals.push(&goal_g);
    database.push(rule{&f_head0, {}});
    database.push(rule{&f_head1, {}});
    database.push(rule{&g_head0, {}});
    database.push(rule{&g_head1, {}});

    auto& make_initial_goal_lineage =
        stack.make_initial_goal_lineage_;
    auto& make_resolution_lineage =
        stack.lineage_pool_;
    auto& learn_avoidance = stack.cdcl_;
    auto& derive_resolution_lemma =
        stack.resolution_memory_;

    const goal_lineage* gl_f = make_initial_goal_lineage.make(0);
    const goal_lineage* gl_g = make_initial_goal_lineage.make(1);
    const resolution_lineage* rl_f_0 =
        make_resolution_lineage.make_resolution_lineage(gl_f, rule_id{0});
    const resolution_lineage* rl_g_2 =
        make_resolution_lineage.make_resolution_lineage(gl_g, rule_id{2});
    const resolution_lineage* rl_g_3 =
        make_resolution_lineage.make_resolution_lineage(gl_g, rule_id{3});

    learn_avoidance.learn(lemma{{rl_f_0, rl_g_2}});

    simulation simulation{stack, kDefaultMaxResolutions};
    simulation.set_up();

    EXPECT_CALL(stack.decision_generator, generate()).WillOnce(Return(rl_f_0));

    EXPECT_EQ(simulation.run(), sim_termination::solved);
    EXPECT_THAT(derive_resolution_lemma.derive_resolution_lemma().get_resolutions(),
        UnorderedElementsAre(rl_f_0, rl_g_3));
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsConflictedAfterPartialProgressWhenDerivedGoalFails) {
    /*
     * initial goals:
     *   f(A).
     *   g(A).
     * database:
     *   0: f(A) :- h(A).
     *   1: h(abc).
     *   2: g(xyz).
     * setup: shared A; f/h and g(xyz) cannot both succeed — conflict after unit resolution(s)
     */
    expr rule_var_a{expr::var{0}};
    expr abc{expr::functor{functors.id("abc"), {}}};
    expr xyz{expr::functor{functors.id("xyz"), {}}};
    expr f_head{expr::functor{functors.id("f"), {&rule_var_a}}};
    expr h_body{expr::functor{functors.id("h"), {&rule_var_a}}};
    expr h_head{expr::functor{functors.id("h"), {&abc}}};
    expr g_head{expr::functor{functors.id("g"), {&xyz}}};
    database.push(rule{&f_head, {&h_body}});
    database.push(rule{&h_head, {}});
    database.push(rule{&g_head, {}});

    auto& frame_alloc = stack.frame_allocator_;
    auto& make_var = stack.expr_pool_;
    auto& make_functor = stack.expr_pool_;

    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    simulation simulation{stack, kDefaultMaxResolutions};
    simulation.set_up();
    const expr* var_a = saved_expr_pool_.make_var(frame_alloc.bump(1));
    initial_goals.push(saved_expr_pool_.make_functor(functors.id("f"), {var_a}));
    initial_goals.push(saved_expr_pool_.make_functor(functors.id("g"), {var_a}));

    EXPECT_EQ(simulation.run(), sim_termination::conflicted);
    simulation.tear_down();
}

// ---------------------------------------------------------------------------
// sim lifecycle (trail + store observables)
// ---------------------------------------------------------------------------

TEST_F(SimIntegrationTest, SetUpLifecyclePushesOneTrailFrameWithoutRunning) {
  // Capture baseline immediately before set_up(); wiring must not intern or push frames.
  const size_t depth_before = stack.trail_.depth();
  const size_t expr_before = stack.expr_pool_.size();

  simulation simulation{stack, kDefaultMaxResolutions};
  simulation.set_up();

  EXPECT_EQ(stack.trail_.depth(), depth_before + 1);
  EXPECT_EQ(stack.expr_pool_.size(), expr_before);
  EXPECT_TRUE(stack.ra_active_goals_.empty());
  simulation.tear_down();
}

TEST_F(SimIntegrationTest, TearDownLifecycleRestoresTrailDepthAfterEmptyRun) {
  const size_t depth_before = stack.trail_.depth();

  simulation simulation{stack, kDefaultMaxResolutions};
  simulation.set_up();
  EXPECT_EQ(simulation.run(), sim_termination::solved);
  simulation.tear_down();

  EXPECT_EQ(stack.trail_.depth(), depth_before);
}

TEST_F(SimIntegrationTest, TearDownLifecycleRestoresTrailDepthAfterConflictedRun) {
  expr goal{expr::functor{functors.id("f"), {}}};
  initial_goals.push(&goal);

  const size_t depth_before = stack.trail_.depth();

  EXPECT_CALL(stack.decision_generator, generate()).Times(0);

  simulation simulation{stack, kDefaultMaxResolutions};
  simulation.set_up();
  EXPECT_EQ(simulation.run(), sim_termination::conflicted);
  simulation.tear_down();

  EXPECT_EQ(stack.trail_.depth(), depth_before);
}

TEST_F(SimIntegrationTest, TearDownLifecycleRestoresTrailDepthAfterDepthExceededRun) {
  expr goal{expr::functor{functors.id("f"), {}}};
  expr f_head{expr::functor{functors.id("f"), {}}};
  expr f_body{expr::functor{functors.id("f"), {}}};
  initial_goals.push(&goal);
  database.push(rule{&f_head, {&f_body}});

  static constexpr size_t kMaxResolutions = 4;
  const size_t depth_before = stack.trail_.depth();

  EXPECT_CALL(stack.decision_generator, generate()).Times(0);

  simulation simulation{stack, kMaxResolutions};
  simulation.set_up();
  EXPECT_EQ(simulation.run(), sim_termination::depth_exceeded);
  simulation.tear_down();

  EXPECT_EQ(stack.trail_.depth(), depth_before);
}

TEST_F(SimIntegrationTest, TearDownLifecycleClearsEphemeralStoresAfterSolvedRun) {
  expr abc{expr::functor{functors.id("abc"), {}}};
  expr _123{expr::functor{functors.id("123"), {}}};
  expr head{expr::functor{functors.id("f"), {&abc, &_123}}};
  database.push(rule{&head, {}});

  auto& bind_map = stack.bind_map_;
  auto& frame_alloc = stack.frame_allocator_;
  auto& make_var = stack.expr_pool_;
  auto& make_functor = stack.expr_pool_;

  const uint32_t idx_test = frame_alloc.bump(1);
  const expr* test_var = saved_expr_pool_.make_var(idx_test);
  initial_goals.push(saved_expr_pool_.make_functor(functors.id("f"), {test_var, saved_expr_pool_.make_var(frame_alloc.bump(1))}));

  EXPECT_CALL(stack.decision_generator, generate()).Times(0);

  simulation simulation{stack, kDefaultMaxResolutions};
  simulation.set_up();
  EXPECT_EQ(simulation.run(), sim_termination::solved);
  simulation.tear_down();

  EXPECT_TRUE(stack.ra_active_goals_.empty());
  EXPECT_EQ(stack.decision_memory_.count(), 0u);
  EXPECT_EQ(stack.resolution_memory_.get_resolution_count(), 0u);
  EXPECT_THAT(stack.resolution_memory_.derive_resolution_lemma().get_resolutions(),
      IsEmpty());
  const expr* whnf = bind_map.whnf({test_var, 0}).skeleton;
  ASSERT_TRUE(std::holds_alternative<expr::var>(whnf->content));
  EXPECT_EQ(std::get<expr::var>(whnf->content).index, idx_test);
}

TEST_F(SimIntegrationTest, TearDownLifecycleRetainsInFrameExprPoolGrowth) {
  expr rule_ignored{expr::var{0}};
  expr rule_l{expr::var{0}};
  expr rule_a{expr::var{1}};
  expr rule_t{expr::var{2}};
  expr zero{expr::functor{functors.id("zero"), {}}};
  expr nil{expr::functor{functors.id("nil"), {}}};
  expr head0{expr::functor{functors.id("make_list"), {&zero, &rule_ignored, &nil}}};
  expr suc_l{expr::functor{functors.id("suc"), {&rule_l}}};
  expr cons_at{expr::functor{functors.id("cons"), {&rule_a, &rule_t}}};
  expr head1{expr::functor{functors.id("make_list"), {&suc_l, &rule_a, &cons_at}}};
  expr body1{expr::functor{functors.id("make_list"), {&rule_l, &rule_a, &rule_t}}};
  database.push(rule{&head0, {}});
  database.push(rule{&head1, {&body1}});

  auto& frame_alloc = stack.frame_allocator_;
  auto& make_var = stack.expr_pool_;
  auto& make_functor = stack.expr_pool_;

  const size_t expr_before = stack.expr_pool_.size();

  static constexpr size_t kMaxResolutions = 32;
  static constexpr int kListLength = 5;
  EXPECT_CALL(stack.decision_generator, generate()).Times(0);

  simulation simulation{stack, kMaxResolutions};
  simulation.set_up();
  const expr* zero_pool = saved_expr_pool_.make_functor(functors.id("zero"), {});
  const expr* len = zero_pool;
  for (int i = 0; i < kListLength; ++i)
    len = saved_expr_pool_.make_functor(functors.id("suc"), {len});
  const expr* abc = saved_expr_pool_.make_functor(functors.id("abc"), {});
  const expr* var_r = saved_expr_pool_.make_var(frame_alloc.bump(1));
  initial_goals.push(saved_expr_pool_.make_functor(functors.id("make_list"), {len, abc, var_r}));

  EXPECT_EQ(simulation.run(), sim_termination::solved);
  EXPECT_GT(stack.expr_pool_.size(), expr_before);

  simulation.tear_down();
  EXPECT_GT(stack.expr_pool_.size(), expr_before);
}

TEST_F(SimIntegrationTest, TearDownLifecycleResetsVarSequencerWhenIncrementedInFrame) {
  expr rule_var{expr::var{0}};
  expr abc{expr::functor{functors.id("abc"), {}}};
  expr f_head{expr::functor{functors.id("f"), {&rule_var}}};
  expr g_body{expr::functor{functors.id("g"), {&rule_var}}};
  expr g_head{expr::functor{functors.id("g"), {&abc}}};
  database.push(rule{&f_head, {&g_body}});
  database.push(rule{&g_head, {}});

  auto& frame_alloc = stack.frame_allocator_;
  auto& make_var = stack.expr_pool_;
  auto& make_functor = stack.expr_pool_;

  // In the real system, initial_frame_offset is baked into the frame_bump_allocator at
  // construction; here the wiring starts at 0. We verify that sim-frame bumps are undone.
  static constexpr uint32_t kExpectedAfterTeardown = 0;

  EXPECT_CALL(stack.decision_generator, generate()).Times(0);

  simulation simulation{stack, kDefaultMaxResolutions};
  simulation.set_up();
  const expr* var_in_frame = saved_expr_pool_.make_var(frame_alloc.bump(1));
  initial_goals.push(saved_expr_pool_.make_functor(functors.id("f"), {var_in_frame}));

  EXPECT_EQ(simulation.run(), sim_termination::solved);
  simulation.tear_down();

  EXPECT_EQ(frame_alloc.bump(1), kExpectedAfterTeardown);
}

TEST_F(SimIntegrationTest, BaseFrameCdclLearnSurvivesLifecycleTearDown) {
  expr goal_f{expr::functor{functors.id("f"), {}}};
  expr goal_g{expr::functor{functors.id("g"), {}}};
  expr f_head0{expr::functor{functors.id("f"), {}}};
  expr f_head1{expr::functor{functors.id("f"), {}}};
  expr g_head2{expr::functor{functors.id("g"), {}}};
  expr g_head3{expr::functor{functors.id("g"), {}}};
  initial_goals.push(&goal_f);
  initial_goals.push(&goal_g);
  database.push(rule{&f_head0, {}});
  database.push(rule{&f_head1, {}});
  database.push(rule{&g_head2, {}});
  database.push(rule{&g_head3, {}});

  auto& make_initial_goal_lineage =
      stack.make_initial_goal_lineage_;
  auto& make_resolution_lineage =
      stack.lineage_pool_;
  auto& learn_avoidance = stack.cdcl_;
  auto& pin_resolution_lineage =
      stack.lineage_pool_;
  auto& derive_resolution_lemma =
      stack.resolution_memory_;

  const goal_lineage* gl0 = make_initial_goal_lineage.make(0);
  const goal_lineage* gl1 = make_initial_goal_lineage.make(1);
  const resolution_lineage* rl_g0_0 =
      make_resolution_lineage.make_resolution_lineage(gl0, rule_id{0});
  const resolution_lineage* rl_g1_2 =
      make_resolution_lineage.make_resolution_lineage(gl1, rule_id{2});
  const resolution_lineage* rl_g1_3 =
      make_resolution_lineage.make_resolution_lineage(gl1, rule_id{3});

  learn_avoidance.learn(lemma{{rl_g0_0, rl_g1_2}});
  pin_resolution_lineage.pin(rl_g0_0);
  pin_resolution_lineage.pin(rl_g1_2);
  pin_resolution_lineage.pin(rl_g1_3);

  simulation simulation{stack, kDefaultMaxResolutions};
  simulation.set_up();

  EXPECT_CALL(stack.decision_generator, generate()).WillOnce(Return(rl_g0_0));
  EXPECT_EQ(simulation.run(), sim_termination::solved);
  EXPECT_THAT(derive_resolution_lemma.derive_resolution_lemma().get_resolutions(),
      UnorderedElementsAre(rl_g0_0, rl_g1_3));
  simulation.tear_down();

  simulation.set_up();
  EXPECT_CALL(stack.decision_generator, generate()).WillOnce(Return(rl_g0_0));
  EXPECT_EQ(simulation.run(), sim_termination::solved);
  EXPECT_THAT(derive_resolution_lemma.derive_resolution_lemma().get_resolutions(),
      UnorderedElementsAre(rl_g0_0, rl_g1_3));
  simulation.tear_down();
}

TEST_F(SimIntegrationTest, IdenticalSimCycleLifecycleRunsCleanAfterTearDown) {
  expr abc{expr::functor{functors.id("abc"), {}}};
  expr _123{expr::functor{functors.id("123"), {}}};
  expr head{expr::functor{functors.id("f"), {&abc, &_123}}};
  database.push(rule{&head, {}});

  auto& bind_map = stack.bind_map_;
  auto& frame_alloc = stack.frame_allocator_;
  auto& make_var = stack.expr_pool_;
  auto& make_functor = stack.expr_pool_;
  auto& get_resolution_count =
      stack.resolution_memory_;
  auto& get_decision_count = stack.decision_memory_;

  const uint32_t idx_a = frame_alloc.bump(1);
  const uint32_t idx_b = frame_alloc.bump(1);
  const expr* var_a = saved_expr_pool_.make_var(idx_a);
  const expr* var_b = saved_expr_pool_.make_var(idx_b);
  const expr* goal = saved_expr_pool_.make_functor(functors.id("f"), {var_a, var_b});
  initial_goals.push(goal);

  EXPECT_CALL(stack.decision_generator, generate()).Times(0);

  simulation simulation{stack, kDefaultMaxResolutions};

  simulation.set_up();
  EXPECT_EQ(simulation.run(), sim_termination::solved);
  const size_t res_count_1 = get_resolution_count.get_resolution_count();
  const size_t dec_count_1 = get_decision_count.count();
  const uint32_t whnf_a_1 =
      std::get<expr::functor>(bind_map.whnf({var_a, 0}).skeleton->content).id;
  const uint32_t whnf_b_1 =
      std::get<expr::functor>(bind_map.whnf({var_b, 0}).skeleton->content).id;
  simulation.tear_down();

  simulation.set_up();
  EXPECT_EQ(simulation.run(), sim_termination::solved);
  EXPECT_EQ(get_resolution_count.get_resolution_count(), res_count_1);
  EXPECT_EQ(get_decision_count.count(), dec_count_1);
  const uint32_t whnf_a_2 =
      std::get<expr::functor>(bind_map.whnf({var_a, 0}).skeleton->content).id;
  const uint32_t whnf_b_2 =
      std::get<expr::functor>(bind_map.whnf({var_b, 0}).skeleton->content).id;
  EXPECT_EQ(whnf_a_2, whnf_a_1);
  EXPECT_EQ(whnf_b_2, whnf_b_1);
  simulation.tear_down();
}

// ---------------------------------------------------------------------------
// sim stress
// ---------------------------------------------------------------------------

TEST_F(SimIntegrationTest, RunReturnsSolvedStressListOfTwentyAbcWithoutDecisions) {
  expr rule_ignored{expr::var{0}};
  expr rule_l{expr::var{0}};
  expr rule_a{expr::var{1}};
  expr rule_t{expr::var{2}};
  expr zero{expr::functor{functors.id("zero"), {}}};
  expr nil{expr::functor{functors.id("nil"), {}}};
  expr head0{expr::functor{functors.id("make_list"), {&zero, &rule_ignored, &nil}}};
  expr suc_l{expr::functor{functors.id("suc"), {&rule_l}}};
  expr cons_at{expr::functor{functors.id("cons"), {&rule_a, &rule_t}}};
  expr head1{expr::functor{functors.id("make_list"), {&suc_l, &rule_a, &cons_at}}};
  expr body1{expr::functor{functors.id("make_list"), {&rule_l, &rule_a, &rule_t}}};
  database.push(rule{&head0, {}});
  database.push(rule{&head1, {&body1}});

  auto& bind_map = stack.bind_map_;
  auto& frame_alloc = stack.frame_allocator_;
  auto& make_var = stack.expr_pool_;
  auto& make_functor = stack.expr_pool_;

  static constexpr size_t kMaxResolutions = 64;
  static constexpr int kListLength = 20;
  EXPECT_CALL(stack.decision_generator, generate()).Times(0);

  simulation simulation{stack, kMaxResolutions};
  simulation.set_up();
  const expr* zero_pool = saved_expr_pool_.make_functor(functors.id("zero"), {});
  const expr* len = make_suc_n(functors, make_functor, zero_pool, kListLength);
  const expr* abc = saved_expr_pool_.make_functor(functors.id("abc"), {});
  const expr* var_r = saved_expr_pool_.make_var(frame_alloc.bump(1));
  initial_goals.push(saved_expr_pool_.make_functor(functors.id("make_list"), {len, abc, var_r}));

  EXPECT_EQ(simulation.run(), sim_termination::solved);

  normalizer<globalizer, expr_pool, expr_pool, decltype(stack.bind_map_)> norm{stack.globalizer_, stack.expr_pool_, stack.expr_pool_, stack.bind_map_};
  const expr* tail = norm.normalize({var_r, 0});
  for (int i = 0; i < kListLength; ++i) {
    const expr::functor& cell = std::get<expr::functor>(tail->content);
    ASSERT_EQ(cell.id, k_cons_functor_id);
    ASSERT_EQ(cell.args.size(), 2u);
    const expr::functor& head_cell =
        std::get<expr::functor>(norm.normalize({cell.args[0], 0})->content);
    EXPECT_EQ(head_cell.id, functors.id("abc"));
    EXPECT_TRUE(head_cell.args.empty());
    tail = norm.normalize({cell.args[1], 0});
  }
  const expr::functor& nil_tail = std::get<expr::functor>(norm.normalize({tail, 0})->content);
  EXPECT_EQ(nil_tail.id, functors.id("nil"));
  EXPECT_TRUE(nil_tail.args.empty());
  simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedStressLinearChainClauseDepthTwenty) {
  static constexpr size_t kChainDepth = 20;
  static constexpr size_t kMaxResolutions = 64;

  expr ground{expr::functor{functors.id("abc"), {}}};
  std::vector<expr> storage;
  chain_clause_db(functors, database, storage, "chain", kChainDepth, &ground);

  auto& get_resolution_count =
      stack.resolution_memory_;

  EXPECT_CALL(stack.decision_generator, generate()).Times(0);

  expr goal{expr::functor{functors.id("chain0"), {&ground}}};
  initial_goals.push(&goal);

  simulation simulation{stack, kMaxResolutions};
  simulation.set_up();
  EXPECT_EQ(simulation.run(), sim_termination::solved);
  EXPECT_GE(get_resolution_count.get_resolution_count(), kChainDepth);
  EXPECT_LE(get_resolution_count.get_resolution_count(), kMaxResolutions);
  simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedStressEvenOddPeanoGoal) {
  expr rule_x{expr::var{0}};
  expr zero{expr::functor{functors.id("zero"), {}}};
  expr suc_x{expr::functor{functors.id("suc"), {&rule_x}}};
  expr even_head0{expr::functor{functors.id("even"), {&zero}}};
  expr odd_head{expr::functor{functors.id("odd"), {&suc_x}}};
  expr even_head1{expr::functor{functors.id("even"), {&suc_x}}};
  expr odd_body{expr::functor{functors.id("odd"), {&rule_x}}};
  expr even_body{expr::functor{functors.id("even"), {&rule_x}}};
  database.push(rule{&even_head0, {}});
  database.push(rule{&odd_head, {&even_body}});
  database.push(rule{&even_head1, {&odd_body}});

  auto& make_functor = stack.expr_pool_;

  static constexpr size_t kMaxResolutions = 128;
  static constexpr int kSucDepth = 12;
  EXPECT_CALL(stack.decision_generator, generate()).Times(0);

  simulation simulation{stack, kMaxResolutions};
  simulation.set_up();
  const expr* zero_pool = saved_expr_pool_.make_functor(functors.id("zero"), {});
  const expr* goal_even = saved_expr_pool_.make_functor(functors.id("even"), {make_suc_n(functors, make_functor, zero_pool, kSucDepth)});
  initial_goals.push(goal_even);

  EXPECT_EQ(simulation.run(), sim_termination::solved);
  simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedStressDeepNestedFunctorTower) {
  static constexpr int kTowerDepth = 8;
  static constexpr size_t kMaxResolutions = 64;
  expr zero{expr::functor{functors.id("zero"), {}}};
  expr rule_x{expr::var{0}};
  expr suc_wrap{expr::functor{functors.id("wrap"), {&rule_x}}};
  expr unwrap_head{expr::functor{functors.id("unwrap"), {&suc_wrap}}};
  expr unwrap_body{expr::functor{functors.id("unwrap"), {&rule_x}}};
  expr unwrap_zero{expr::functor{functors.id("unwrap"), {&zero}}};
  database.push(rule{&unwrap_head, {&unwrap_body}});
  database.push(rule{&unwrap_zero, {}});

  auto& make_functor = stack.expr_pool_;
  auto& get_resolution_count =
      stack.resolution_memory_;

  EXPECT_CALL(stack.decision_generator, generate()).Times(0);

  simulation simulation{stack, kMaxResolutions};
  simulation.set_up();
  const expr* inner = &zero;
  for (int i = 0; i < kTowerDepth; ++i)
    inner = saved_expr_pool_.make_functor(functors.id("wrap"), {inner});
  const expr* goal_expr = saved_expr_pool_.make_functor(functors.id("unwrap"), {inner});
  initial_goals.push(goal_expr);

  EXPECT_EQ(simulation.run(), sim_termination::solved);
  EXPECT_GE(get_resolution_count.get_resolution_count(), static_cast<size_t>(kTowerDepth));
  simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedStressLongSharedVarChainWithoutDecisions) {
  static constexpr int kChainGoals = 6;
  expr t0{expr::functor{functors.id("t0"), {}}};
  expr t1{expr::functor{functors.id("t1"), {}}};
  expr t2{expr::functor{functors.id("t2"), {}}};
  expr t3{expr::functor{functors.id("t3"), {}}};
  expr t4{expr::functor{functors.id("t4"), {}}};
  expr t5{expr::functor{functors.id("t5"), {}}};
  expr t6{expr::functor{functors.id("t6"), {}}};
  expr g0_head{expr::functor{functors.id("g0"), {&t0, &t1}}};
  expr g1_head{expr::functor{functors.id("g1"), {&t1, &t2}}};
  expr g2_head{expr::functor{functors.id("g2"), {&t2, &t3}}};
  expr g3_head{expr::functor{functors.id("g3"), {&t3, &t4}}};
  expr g4_head{expr::functor{functors.id("g4"), {&t4, &t5}}};
  expr g5_head{expr::functor{functors.id("g5"), {&t5, &t6}}};
  database.push(rule{&g0_head, {}});
  database.push(rule{&g1_head, {}});
  database.push(rule{&g2_head, {}});
  database.push(rule{&g3_head, {}});
  database.push(rule{&g4_head, {}});
  database.push(rule{&g5_head, {}});

  auto& bind_map = stack.bind_map_;
  auto& frame_alloc = stack.frame_allocator_;
  auto& make_var = stack.expr_pool_;
  auto& make_functor = stack.expr_pool_;

  EXPECT_CALL(stack.decision_generator, generate()).Times(0);

  simulation simulation{stack, kDefaultMaxResolutions};
  simulation.set_up();
  std::vector<const expr*> vars;
  for (int i = 0; i <= kChainGoals; ++i)
    vars.push_back(saved_expr_pool_.make_var(frame_alloc.bump(1)));
  for (int i = 0; i < kChainGoals; ++i)
    initial_goals.push(saved_expr_pool_.make_functor(
        functors.id(("g" + std::to_string(i)).c_str()), {vars[i], vars[i + 1]}));

  EXPECT_EQ(simulation.run(), sim_termination::solved);
  const expr::functor& whnf_end =
      std::get<expr::functor>(bind_map.whnf({vars[kChainGoals], 0}).skeleton->content);
  EXPECT_EQ(whnf_end.id, functors.id("t6"));
  EXPECT_TRUE(whnf_end.args.empty());
  simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedStressDiamondSharedVarWithoutDecisions) {
  expr abc{expr::functor{functors.id("abc"), {}}};
  expr _123{expr::functor{functors.id("123"), {}}};
  expr xyz{expr::functor{functors.id("xyz"), {}}};
  expr f_head{expr::functor{functors.id("f"), {&abc, &xyz}}};
  expr g_head{expr::functor{functors.id("g"), {&abc, &_123}}};
  expr h_head{expr::functor{functors.id("h"), {&_123, &xyz}}};
  database.push(rule{&f_head, {}});
  database.push(rule{&g_head, {}});
  database.push(rule{&h_head, {}});

  auto& bind_map = stack.bind_map_;
  auto& frame_alloc = stack.frame_allocator_;
  auto& make_var = stack.expr_pool_;
  auto& make_functor = stack.expr_pool_;

  EXPECT_CALL(stack.decision_generator, generate()).Times(0);

  simulation simulation{stack, kDefaultMaxResolutions};
  simulation.set_up();
  const expr* var_a = saved_expr_pool_.make_var(frame_alloc.bump(1));
  const expr* var_b = saved_expr_pool_.make_var(frame_alloc.bump(1));
  const expr* var_c = saved_expr_pool_.make_var(frame_alloc.bump(1));
  initial_goals.push(saved_expr_pool_.make_functor(functors.id("f"), {var_a, var_c}));
  initial_goals.push(saved_expr_pool_.make_functor(functors.id("g"), {var_a, var_b}));
  initial_goals.push(saved_expr_pool_.make_functor(functors.id("h"), {var_b, var_c}));

  EXPECT_EQ(simulation.run(), sim_termination::solved);
  const expr::functor& whnf_a = std::get<expr::functor>(bind_map.whnf({var_a, 0}).skeleton->content);
  const expr::functor& whnf_c = std::get<expr::functor>(bind_map.whnf({var_c, 0}).skeleton->content);
  EXPECT_EQ(whnf_a.id, functors.id("abc"));
  EXPECT_EQ(whnf_c.id, functors.id("xyz"));
  simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedStressWideClauseTreeWithoutDecisions) {
  // Base tree: f:-g,h; g:-i,j; h:-i,j; facts i,j. Wide: extra clause k:-i,j,l; l. fact.
  static constexpr size_t kMinResolutions = 7;

  expr goal_f{expr::functor{functors.id("f"), {}}};
  expr f_head{expr::functor{functors.id("f"), {}}};
  expr g_body{expr::functor{functors.id("g"), {}}};
  expr h_body{expr::functor{functors.id("h"), {}}};
  expr g_head{expr::functor{functors.id("g"), {}}};
  expr h_head{expr::functor{functors.id("h"), {}}};
  expr k_head{expr::functor{functors.id("k"), {}}};
  expr l_body{expr::functor{functors.id("l"), {}}};
  expr i_body{expr::functor{functors.id("i"), {}}};
  expr j_body{expr::functor{functors.id("j"), {}}};
  expr i_head{expr::functor{functors.id("i"), {}}};
  expr j_head{expr::functor{functors.id("j"), {}}};
  expr l_head{expr::functor{functors.id("l"), {}}};
  initial_goals.push(&goal_f);
  database.push(rule{&f_head, {&g_body, &h_body}});
  database.push(rule{&g_head, {&i_body, &j_body}});
  database.push(rule{&h_head, {&i_body, &j_body}});
  database.push(rule{&k_head, {&i_body, &j_body, &l_body}});
  database.push(rule{&i_head, {}});
  database.push(rule{&j_head, {}});
  database.push(rule{&l_head, {}});

  auto& get_resolution_count =
      stack.resolution_memory_;

  EXPECT_CALL(stack.decision_generator, generate()).Times(0);

  simulation simulation{stack, kDefaultMaxResolutions};
  simulation.set_up();
  EXPECT_EQ(simulation.run(), sim_termination::solved);
  EXPECT_GE(get_resolution_count.get_resolution_count(), kMinResolutions);
  simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedStressMultipleAvoidancesSeveralDecisions) {
  // Four duplicate-rule goals; four scripted decisions. Outcome contract only — not call order.
  expr goal_f{expr::functor{functors.id("f"), {}}};
  expr goal_g{expr::functor{functors.id("g"), {}}};
  expr goal_h{expr::functor{functors.id("h"), {}}};
  expr goal_k{expr::functor{functors.id("k"), {}}};
  expr f0{expr::functor{functors.id("f"), {}}};
  expr f1{expr::functor{functors.id("f"), {}}};
  expr g0{expr::functor{functors.id("g"), {}}};
  expr g1{expr::functor{functors.id("g"), {}}};
  expr h0{expr::functor{functors.id("h"), {}}};
  expr h1{expr::functor{functors.id("h"), {}}};
  expr k0{expr::functor{functors.id("k"), {}}};
  expr k1{expr::functor{functors.id("k"), {}}};
  initial_goals.push(&goal_f);
  initial_goals.push(&goal_g);
  initial_goals.push(&goal_h);
  initial_goals.push(&goal_k);
  database.push(rule{&f0, {}});
  database.push(rule{&f1, {}});
  database.push(rule{&g0, {}});
  database.push(rule{&g1, {}});
  database.push(rule{&h0, {}});
  database.push(rule{&h1, {}});
  database.push(rule{&k0, {}});
  database.push(rule{&k1, {}});

  auto& make_initial_goal_lineage =
      stack.make_initial_goal_lineage_;
  auto& make_resolution_lineage =
      stack.lineage_pool_;
  auto& get_decision_count = stack.decision_memory_;

  const goal_lineage* gl_f = make_initial_goal_lineage.make(0);
  const goal_lineage* gl_g = make_initial_goal_lineage.make(1);
  const goal_lineage* gl_h = make_initial_goal_lineage.make(2);
  const goal_lineage* gl_k = make_initial_goal_lineage.make(3);
  const resolution_lineage* rl_f_0 =
      make_resolution_lineage.make_resolution_lineage(gl_f, rule_id{0});
  const resolution_lineage* rl_g_1 =
      make_resolution_lineage.make_resolution_lineage(gl_g, rule_id{3});
  const resolution_lineage* rl_h_0 =
      make_resolution_lineage.make_resolution_lineage(gl_h, rule_id{4});
  const resolution_lineage* rl_k_1 =
      make_resolution_lineage.make_resolution_lineage(gl_k, rule_id{7});

  static constexpr size_t kMaxResolutions = 32;
  static constexpr size_t kExpectedDecisions = 4;

  EXPECT_CALL(stack.decision_generator, generate())
      .WillOnce(Return(rl_f_0))
      .WillOnce(Return(rl_g_1))
      .WillOnce(Return(rl_h_0))
      .WillOnce(Return(rl_k_1));

  simulation simulation{stack, kMaxResolutions};
  simulation.set_up();
  EXPECT_EQ(simulation.run(), sim_termination::solved);
  EXPECT_EQ(get_decision_count.count(), kExpectedDecisions);
  simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedStressCdclMhuManyGroundHeadsOnSharedVar) {
  expr abc{expr::functor{functors.id("abc"), {}}};
  expr def{expr::functor{functors.id("def"), {}}};
  expr ghi{expr::functor{functors.id("ghi"), {}}};
  expr jkl{expr::functor{functors.id("jkl"), {}}};
  expr mno{expr::functor{functors.id("mno"), {}}};
  expr pqr{expr::functor{functors.id("pqr"), {}}};
  expr stu{expr::functor{functors.id("stu"), {}}};
  expr _xyz{expr::functor{functors.id("xyz"), {}}};
  expr f_head0{expr::functor{functors.id("f"), {}}};
  expr f_head1{expr::functor{functors.id("f"), {}}};
  expr g1{expr::functor{functors.id("g"), {&abc, &_xyz, &pqr}}};
  expr g2{expr::functor{functors.id("g"), {&def, &_xyz, &pqr}}};
  expr g3{expr::functor{functors.id("g"), {&ghi, &_xyz, &pqr}}};
  expr g4{expr::functor{functors.id("g"), {&jkl, &_xyz, &pqr}}};
  expr g5{expr::functor{functors.id("g"), {&mno, &_xyz, &pqr}}};
  expr g_bad{expr::functor{functors.id("g"), {&ghi, &jkl, &stu}}};
  database.push(rule{&f_head0, {}});
  database.push(rule{&f_head1, {}});
  database.push(rule{&g1, {}});
  database.push(rule{&g2, {}});
  database.push(rule{&g3, {}});
  database.push(rule{&g4, {}});
  database.push(rule{&g5, {}});
  database.push(rule{&g_bad, {}});

  auto& make_initial_goal_lineage =
      stack.make_initial_goal_lineage_;
  auto& make_resolution_lineage =
      stack.lineage_pool_;
  auto& learn_avoidance = stack.cdcl_;
  auto& bind_map = stack.bind_map_;
  auto& frame_alloc = stack.frame_allocator_;
  auto& make_var = stack.expr_pool_;
  auto& make_functor = stack.expr_pool_;

  const goal_lineage* gl_f = make_initial_goal_lineage.make(0);
  const goal_lineage* gl_g = make_initial_goal_lineage.make(1);
  const resolution_lineage* rl_f_0 =
      make_resolution_lineage.make_resolution_lineage(gl_f, rule_id{0});
  const resolution_lineage* rl_g_abc =
      make_resolution_lineage.make_resolution_lineage(gl_g, rule_id{2});
  const resolution_lineage* rl_g_def =
      make_resolution_lineage.make_resolution_lineage(gl_g, rule_id{3});

  learn_avoidance.learn(lemma{{rl_f_0, rl_g_abc}});

  EXPECT_CALL(stack.decision_generator, generate())
      .WillOnce(Return(rl_f_0))
      .WillOnce(Return(rl_g_def));

  simulation simulation{stack, kDefaultMaxResolutions};
  simulation.set_up();
  const expr* var_a = saved_expr_pool_.make_var(frame_alloc.bump(1));
  const expr* var_b = saved_expr_pool_.make_var(frame_alloc.bump(1));
  const expr* var_c = saved_expr_pool_.make_var(frame_alloc.bump(1));
  const expr* xyz = saved_expr_pool_.make_functor(functors.id("xyz"), {});
  initial_goals.push(saved_expr_pool_.make_functor(functors.id("f"), {}));
  initial_goals.push(saved_expr_pool_.make_functor(functors.id("g"), {var_a, var_b, var_c}));

  EXPECT_EQ(simulation.run(), sim_termination::solved);
  const expr::functor& whnf_a = std::get<expr::functor>(bind_map.whnf({var_a, 0}).skeleton->content);
  const expr::functor& whnf_b = std::get<expr::functor>(bind_map.whnf({var_b, 0}).skeleton->content);
  const expr::functor& whnf_c = std::get<expr::functor>(bind_map.whnf({var_c, 0}).skeleton->content);
  EXPECT_EQ(whnf_b.id, functors.id("xyz"));
  EXPECT_EQ(whnf_c.id, functors.id("pqr"));
  EXPECT_EQ(whnf_a.id, functors.id("def"));
  (void)xyz;
  simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsDepthExceededStressOnDeepLinearChainWithinBudget) {
  static constexpr size_t kMaxResolutions = 8;
  static constexpr size_t kChainDepth = 20;

  expr ground{expr::functor{functors.id("abc"), {}}};
  std::vector<expr> storage;
  chain_clause_db(functors, database, storage, "chain", kChainDepth, &ground);

  expr goal{expr::functor{functors.id("chain0"), {&ground}}};
  initial_goals.push(&goal);

  EXPECT_CALL(stack.decision_generator, generate()).Times(0);

  simulation simulation{stack, kMaxResolutions};
  simulation.set_up();
  EXPECT_EQ(simulation.run(), sim_termination::depth_exceeded);
  EXPECT_EQ(stack.resolution_memory_.get_resolution_count(), kMaxResolutions);
  simulation.tear_down();
}
