#ifndef SEQUENCER_HPP
#define SEQUENCER_HPP

#include "infrastructure/tracked.hpp"
#include "infrastructure/backtrackable_increment.hpp"

template<typename IndexType, typename ILog>
struct sequencer {
    sequencer(ILog& t, IndexType initial);
    IndexType next();
private:
    tracked<IndexType, ILog> index;
};

template<typename IndexType, typename ILog>
sequencer<IndexType, ILog>::sequencer(ILog& t, IndexType initial) : index(t, initial) {}

template<typename IndexType, typename ILog>
IndexType sequencer<IndexType, ILog>::next() {
    IndexType result = index.get();
    index.mutate(std::make_unique<backtrackable_increment<IndexType>>());
    return result;
}

#endif
