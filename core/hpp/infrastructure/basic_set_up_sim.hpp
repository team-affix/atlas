#ifndef BASIC_SET_UP_SIM_HPP
#define BASIC_SET_UP_SIM_HPP

template<typename ISetUpSim>
struct basic_set_up_sim {
    basic_set_up_sim(ISetUpSim&);
    void set_up();
private:
    ISetUpSim& set_up_;
};

template<typename ISUS>
basic_set_up_sim<ISUS>::basic_set_up_sim(ISUS& set_up)
    : set_up_(set_up) {}

template<typename ISUS>
void basic_set_up_sim<ISUS>::set_up() {
    set_up_.set_up();
}

#endif
