#ifndef HORIZON_SET_UP_SIM_HPP
#define HORIZON_SET_UP_SIM_HPP

template<typename ISetUpMcts, typename ISetUpSim>
struct horizon_set_up_sim {
    horizon_set_up_sim(ISetUpMcts&, ISetUpSim&);
    void set_up();
private:
    ISetUpMcts& set_up_mcts_;
    ISetUpSim& set_up_;
};

template<typename ISUM, typename ISUS>
horizon_set_up_sim<ISUM, ISUS>::horizon_set_up_sim(ISUM& set_up_mcts, ISUS& set_up)
    : set_up_mcts_(set_up_mcts)
    , set_up_(set_up) {}

template<typename ISUM, typename ISUS>
void horizon_set_up_sim<ISUM, ISUS>::set_up() {
    set_up_mcts_.set_up();
    set_up_.set_up();
}

#endif
