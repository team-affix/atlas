#ifndef UNIFORM_INDEX_SAMPLE_HPP
#define UNIFORM_INDEX_SAMPLE_HPP

#include <cstddef>
#include <random>
#include "debug_assert.hpp"

template<typename IEngine>
struct uniform_index_sample {
    uniform_index_sample(IEngine&);
    size_t sample_index(size_t count);
private:
    IEngine& engine_;
};

template<typename IEngine>
uniform_index_sample<IEngine>::uniform_index_sample(IEngine& engine)
    : engine_(engine) {}

template<typename IEngine>
size_t uniform_index_sample<IEngine>::sample_index(size_t count) {
    DEBUG_ASSERT(count > 0);
    std::uniform_int_distribution<size_t> dist(0, count - 1);
    return dist(engine_);
}

#endif
