#ifndef SET_UP_SIM_HPP
#define SET_UP_SIM_HPP

template<typename IPushFrame>
struct set_up_sim {
    set_up_sim(IPushFrame& frames);
    void set_up();
private:
    IPushFrame& push_frame_;
};

template<typename IPushFrame>
set_up_sim<IPushFrame>::set_up_sim(IPushFrame& frames) : push_frame_(frames) {}

template<typename IPushFrame>
void set_up_sim<IPushFrame>::set_up() { push_frame_.push_frame(); }

#endif
