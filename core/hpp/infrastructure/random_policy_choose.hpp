#ifndef RANDOM_POLICY_CHOOSE_HPP
#define RANDOM_POLICY_CHOOSE_HPP

#include <cstddef>
#include "debug_assert.hpp"

template<typename IChoice, typename IRndGen>
struct random_policy_choose {
    random_policy_choose(IRndGen&);

    template<typename INodeHandle, typename IGetChoiceCount, typename IGetChoiceAt>
    IChoice policy_choose(const INodeHandle& node,
                          const IGetChoiceCount& get_choice_count,
                          const IGetChoiceAt& get_choice_at);

private:
    IRndGen& rnd_gen_;
};

template<typename IChoice, typename IRndGen>
random_policy_choose<IChoice, IRndGen>::random_policy_choose(IRndGen& rnd_gen)
    : rnd_gen_(rnd_gen) {}

template<typename IChoice, typename IRndGen>
template<typename INodeHandle, typename IGetChoiceCount, typename IGetChoiceAt>
IChoice random_policy_choose<IChoice, IRndGen>::policy_choose(
        const INodeHandle&,
        const IGetChoiceCount& get_choice_count,
        const IGetChoiceAt& get_choice_at) {
    DEBUG_ASSERT(get_choice_count.size() > 0);
    const size_t index = rnd_gen_.sample_index(get_choice_count.size());
    return get_choice_at.at(index);
}

#endif
