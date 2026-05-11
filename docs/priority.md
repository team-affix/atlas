# Event Priority

Highest priority at the top, lowest at the bottom.
Higher number = higher priority.
Events sharing a number are emitted together and have the same priority tier.

| #    | Event                                | Constraint                                                        | Extra |
| ---  | ------------------------------------ | ----------------------------------------------------------------- | ----- |
| --   | --                                   | --                                                                | SIM SETUP ZONE |
| 122  | `initial_goal_activating_event`      | BEFORE `initial_goals_activated_event`                            |       |
| 121  | `initial_goals_activated_event`      | AFTER `initial_goal_activating_event`, BEFORE `sim_started_event` |       |
| --   | --                                   | --                                                                | END SIM SETUP |
| --   | --                                   | --                                                                | SIM ZONE |
| 120  | `sim_started_event`                  | AFTER `initial_goals_activated_event`                             |       |
| --   | --                                   | --                                                                | EARLY TERMINATION ZONE |
| 119  | `sim_termination_condition_reached_event`   | BEFORE `conflicted_event`                                  |       |
| 118  | `refuted_event`                      | AFTER `conflicted_event`                                          |       |
| 117  | `conflicted_event`                   | AFTER `sim_started_event`, BEFORE all sim events                  |       |
| 117  | `solved_event`                       | AFTER `sim_started_event`, BEFORE all sim events                  |       |
| --   | --                                   | --                                                                | END EARLY TERMINATION |
| --   | --                                   | --                                                                | CONFLICT PRODUCTION ZONE |
| 116  | `avoidance_empty_event`              | —                                                                 |       |
| 116  | `goal_candidates_empty_event`        | BEFORE `goal_unit_event`                                          |       |
| --   | --                                   | --                                                                | END CONFLICT PRODUCTION |
| 115  | `goal_candidates_changed_event`      | —                                                                 |       |
| 114  | `candidate_eliminated_event`         | —                                                                 |       |
| --   | --                                   | --                                                                | ELIMINATION PRODUCTION ZONE |
| 113  | `backlogged_elimination_freed_event` | AFTER `candidate_eliminated_event`                                | Always produces active elim |
| 113  | `candidate_not_applicable_event`     | —                                                                 | Always produces active elim |
| 112  | `avoidance_unit_event`               | —                                                                 | Can either produce active elim or backlogged elim |
| --   | --                                   | --                                                                | END ELIMINATION PRODUCTION      |
| 111  | `goal_expr_changed_event`            | —                                                                 |       |
| 110  | `representative_changed_event`       | —                                                                 |       |
| --   | --                                   | --                                                                | RESOLUTION ZONE |
| 109  | `resolving_event`               | —                                                                 |       |
| 108  | `goal_activating_event`              | —                                                                 |       |
| 107  | `goal_deactivating_event`            | —                                                                 |       |
| 106  | `goal_activated_event`               | —                                                                 |       |
| 105  | `goal_deactivated_event`             | —                                                                 |       |
| 104  | `resolved_event`                | —                                                                 |       |
| --   | --                                   | --                                                                | END RESOLUTION |
| 103  | `goal_unit_event`                    | —                                                                 |       |
| 102  | `fixpoint_reached_event`             | AFTER `goal_unit_event`                                           |       |
| --   | --                                   | --                                                                | END SIM |
| --   | --                                   | --                                                                | CLEANUP ZONE |
| 101  | `goal_stores_clearing_event`         | AFTER `conflicted_event` / `solved_event`                         |       |
| 101  | `goal_stores_cleared_event`          | AFTER `conflicted_event` / `solved_event`                         |       |
| 100  | `sim_stopped_event`                  | AFTER `goal_stores_cleared_event`                                 |       |
| --   | --                                   | --                                                                | END CLEANUP |
| —    | `never_event`                        | never emitted                                                     |       |

NOTE FOR FUTURE: we will need to somehow allow cdcl to have avoidance_unit_events route to the elimination_backlog despite being in cancelled state at end of sim. we may need a second event handler that has an inverted cancellation state and routes only to elim backlog?
