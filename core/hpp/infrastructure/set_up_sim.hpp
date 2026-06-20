#ifndef SET_UP_SIM_HPP
#define SET_UP_SIM_HPP

template<typename IPushTrailFrame>
struct set_up_sim {
    set_up_sim(IPushTrailFrame& trail);
    void set_up();
private:
    IPushTrailFrame& push_trail_frame_;
};

template<typename IPushTrailFrame>
set_up_sim<IPushTrailFrame>::set_up_sim(IPushTrailFrame& trail) : push_trail_frame_(trail) {}

template<typename IPushTrailFrame>
void set_up_sim<IPushTrailFrame>::set_up() { push_trail_frame_.push(); }

#endif
