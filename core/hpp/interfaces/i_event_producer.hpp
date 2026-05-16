#ifndef I_EVENT_PRODUCER_HPP
#define I_EVENT_PRODUCER_HPP

template<typename Event>
struct i_event_producer {
    virtual ~i_event_producer() = default;
    virtual void produce(const Event&) = 0;
};

#endif
