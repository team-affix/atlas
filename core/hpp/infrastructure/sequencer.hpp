#ifndef SEQUENCER_HPP
#define SEQUENCER_HPP

#include "infrastructure/tracked.hpp"
#include "infrastructure/backtrackable_increment.hpp"

template<typename IndexType>
struct sequencer {
    sequencer(i_log_to_current_trail_frame& t);
    IndexType next();
private:
    tracked<IndexType> index;
};

template<typename IndexType>
sequencer<IndexType>::sequencer(i_log_to_current_trail_frame& t) :
    index(t, 0) {
}

template<typename IndexType>
IndexType sequencer<IndexType>::next() {
    IndexType result = index.get();
    index.mutate(std::make_unique<backtrackable_increment<IndexType>>());
    return result;
}


#endif
