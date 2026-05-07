#ifndef SEQUENCER_HPP
#define SEQUENCER_HPP

#include "../utility/tracked.hpp"
#include "../utility/backtrackable_increment.hpp"

template<typename IndexType>
struct sequencer {
    sequencer(i_trail& t);
    IndexType next();
private:
    tracked<IndexType> index;
};

template<typename IndexType>
sequencer<IndexType>::sequencer(i_trail& t) :
    index(t, 0) {
}

template<typename IndexType>
IndexType sequencer<IndexType>::next() {
    IndexType result = index.get();
    index.mutate(std::make_unique<backtrackable_increment<IndexType>>());
    return result;
}


#endif
