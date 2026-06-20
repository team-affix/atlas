#ifndef SET_UP_SIM_HPP
#define SET_UP_SIM_HPP

template<typename ITrail>
struct set_up_sim {
    set_up_sim(ITrail& trail);
    void set_up();
private:
    ITrail& push_trail_frame_;
};

template<typename ITrail>
set_up_sim<ITrail>::set_up_sim(ITrail& trail) : push_trail_frame_(trail) {}

template<typename ITrail>
void set_up_sim<ITrail>::set_up() { push_trail_frame_.push(); }

#endif
