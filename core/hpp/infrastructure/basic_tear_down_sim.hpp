#ifndef BASIC_TEAR_DOWN_SIM_HPP
#define BASIC_TEAR_DOWN_SIM_HPP

template<typename ITearDownSim>
struct basic_tear_down_sim {
    basic_tear_down_sim(ITearDownSim&);
    void tear_down();
private:
    ITearDownSim& tear_down_;
};

template<typename ITDS>
basic_tear_down_sim<ITDS>::basic_tear_down_sim(ITDS& tear_down)
    : tear_down_(tear_down) {}

template<typename ITDS>
void basic_tear_down_sim<ITDS>::tear_down() {
    tear_down_.tear_down();
}

#endif
