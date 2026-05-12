#include "../../../hpp/domain/entities/goal.hpp"

goal::goal(const expr* e) : e_(e) {}

const expr* goal::e() const { return e_; }

i_set<uint32_t>& goal::e_reps() { return e_reps_; }

i_map<size_t, std::unique_ptr<i_candidate>>& goal::candidates() { return candidates_; }
