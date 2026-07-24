/*
 * Quell coverage gap audit
 *
 * Context: docs/testing.md is absent; rules from .cursor/rules/cpp-testing*.mdc
 * and the quench/horizon behavior-contract specificity rules apply.
 *
 * Existing related coverage (do not duplicate):
 * - goal_weights / dbuct_goal_weights / CGW / horizon_* hooks: already unit+integ tested
 * - ridge_reward / tear_down / terminate: already covered
 * - mcts_sim / dbuct_sim / run_sim / manifests for ridge/horizon/genius: integ smoke
 *
 * Gaps for new quell surfaces (write these tests):
 *
 * 1. goal_work_function::get(depth)
 *    Why: wrong f(l)=1+e^{-K(l-J)} silently corrupts every reward.
 *    Need: new unit file goal_work_function.cpp (K=0.2,J=10: f(J)=2, monotone, asymptote→1).
 *
 * 2. goal_depths / goal_work_values get/set/erase/clear
 *    Why: map contract bugs → assert fails or stale work after deactivate.
 *    Need: new unit files mirroring goal_weights.cpp.
 *
 * 3. remaining_work add/subtract/get/clear
 *    Why: reward is -get(); drift = wrong MCTS backup.
 *    Need: new unit file remaining_work.cpp.
 *
 * 4. dbuct_goal_depths / dbuct_goal_work_values / dbuct_remaining_work push/pop undo
 *    Why: camping backtrack leaves wrong remaining work → reward gaming / wrong backup.
 *    Need: new unit files mirroring dbuct_goal_weights / dbuct_cgw.
 *
 * 5. quell_initial_goal_activator::activate_initial_goal
 *    Why: must set depth 0, work f(0), add remaining; miss → under-counted work.
 *    Need: new unit TEST_F; exact Times on set/add; AtLeast on decay get.
 *
 * 6. quell_goal_activator::activate
 *    Why: child depth/work maps must match resolver’s remaining_work math.
 *    Need: new unit TEST_F; Times(1) on set depth/work; AtLeast on parent get/decay.
 *
 * 7. quell_goal_deactivator::deactivate
 *    Why: must erase depth+work before base deactivate; leak → clear/assert issues.
 *    Need: new unit TEST_F; Times(1) each erase.
 *
 * 8. quell_resolver::resolve
 *    Why: on success subtract parent + add |body|*f(parent+1); on failure leave remaining.
 *    Need: new unit tests for fact (subtract only), clause (subtract+add), propagate false
 *    without mutating remaining_work.
 *
 * 9. quell_reward::compute_mcts_reward
 *    Why: must be -remaining_work; sign flip inverts search objective.
 *    Need: new unit file; AtLeast(1) on get.
 *
 * 10. quell_set_up_sim / quell_tear_down_sim
 *     Why: tear_down must snapshot reward then clear depths/work/remaining.
 *     Need: new unit files; Times(1) on set_value/clears; AtLeast on compute reward get.
 *
 * 11. dbuct_quell_frame_hub push/pop order
 *     Why: reverse pop must undo remaining then work then depth then base.
 *     Need: new unit file with StrictMock sequence.
 *
 * 12. dbuct_quell_terminate_sim::terminate
 *     Why: must set value delta from reward before dbuct terminate.
 *     Need: new unit file.
 *
 * 13. Work conservation (Σ goal_work_values == remaining_work) across activate/resolve
 *     Why: incremental math bug only shows when stores diverge.
 *     Need: new integration quell_work_conservation.cpp.
 *
 * 14. quell_manifest / quell_fc_manifest / dbuct_quell_* wiring + sim smoke
 *     Why: composition-root wiring regressions (wrong collaborator slot).
 *     Need: integration *_manifest.cpp per variant (after each solver lands).
 *
 * 15. CLI quell handlers / --work-decay-k|--work-decay-j
 *     Why: wrong defaults or missing flags → unusable solver.
 *     Need: extend cli command_handler parameterized tests when CLI is wired.
 *
 * Adjacent (out of quell scope; noted only):
 * - run_sim / set_up_sim / tear_down_sim base types still lack dedicated unit tests
 *   (covered via wrappers); no action for quell.
 */

# Quell test audit

See the comment block above for the authoritative gap list.
