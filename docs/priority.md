# Event Priority

Highest priority at the top, lowest at the bottom.
Higher number = higher priority.
Events sharing a number are emitted together and have the same priority tier.

| #    | Event                                | Constraint                                                        | Extra |
| ---  | ------------------------------------ | ----------------------------------------------------------------- | ----- |
| --   | --                                   | --                                                                | SIM SETUP ZONE |
| 120  | `initial_goal_activating_event`      | BEFORE `initial_goals_activated_event`                            |       |
| 119  | `initial_goals_activated_event`      | AFTER `initial_goal_activating_event`, BEFORE `sim_started_event` |       |
| --   | --                                   | --                                                                | END SIM SETUP |
| --   | --                                   | --                                                                | SIM ZONE |
| 118  | `sim_started_event`                  | AFTER `initial_goals_activated_event`                             |       |
| --   | --                                   | --                                                                | EARLY TERMINATION ZONE |
| 117  | `sim_termination_condition_reached_event`   | AFTER `sim_started_event`, BEFORE all sim events                  |       |
| 117  | `solved_event`                       | AFTER `sim_started_event`, BEFORE all sim events                  |       |
| 117  | `conflicted_event`                   | AFTER `sim_started_event`, BEFORE all sim events                  |       |
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
| 109  | `goal_resolving_event`               | —                                                                 |       |
| 108  | `goal_activating_event`              | —                                                                 |       |
| 108  | `goal_deactivating_event`            | —                                                                 |       |
| 107  | `goal_activated_event`               | —                                                                 |       |
| 107  | `goal_deactivated_event`             | —                                                                 |       |
| 106  | `goal_resolved_event`                | —                                                                 |       |
| --   | --                                   | --                                                                | END RESOLUTION |
| 105  | `goal_unit_event`                    | —                                                                 |       |
| 104  | `fixpoint_reached_event`             | AFTER `goal_unit_event`                                           |       |
| --   | --                                   | --                                                                | END SIM |
| 103  | `refuted_event`                      | AFTER `conflicted_event`                                          |       |
| 102  | `goal_stores_clearing_event`         | AFTER `conflicted_event` / `solved_event`                         |       |
| 102  | `goal_stores_cleared_event`          | AFTER `conflicted_event` / `solved_event`                         |       |
| 101  | `sim_stopped_event`                  | AFTER `goal_stores_cleared_event`                                 |       |
| —    | `never_event`                        | never emitted                                                     |       |
