#include "../../../hpp/domain/entities/candidate.hpp"

candidate::candidate(std::unique_ptr<i_translation_map> tm, std::unique_ptr<i_unifier> u)
    : tm_(std::move(tm)), u_(std::move(u)) {}

i_translation_map& candidate::tm() { return *tm_; }
i_unifier& candidate::u() { return *u_; }
