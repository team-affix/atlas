#ifndef CANDIDATE_HPP
#define CANDIDATE_HPP

#include <memory>
#include "../interfaces/i_candidate.hpp"

struct candidate : i_candidate {
    candidate(std::unique_ptr<i_translation_map>, std::unique_ptr<i_unifier>);
    i_translation_map& tm() override;
    i_unifier& u() override;
private:
    std::unique_ptr<i_translation_map> tm_;
    std::unique_ptr<i_unifier> u_;
};

#endif
