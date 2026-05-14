#ifndef CANDIDATE_FACTORY_HPP
#define CANDIDATE_FACTORY_HPP

#include <memory>
#include "../domain/interfaces/i_factory.hpp"
#include "../domain/interfaces/i_database.hpp"
#include "../domain/interfaces/i_copier.hpp"
#include "../domain/value_objects/candidate.hpp"

struct candidate_factory : i_factory<candidate, size_t> {
    candidate_factory();
    std::unique_ptr<candidate> make(size_t) const override;
private:
    const i_database& db_;
    i_copier& copier_;
};

#endif
