#ifndef PAUSE_POLLER_HPP
#define PAUSE_POLLER_HPP

#include <iostream>
#include <optional>

template<typename IPrintStats, typename IIsTty, typename ITryReadByte, typename IIdle>
struct pause_poller {
    pause_poller(IPrintStats&, IIsTty&, ITryReadByte&, IIdle&);
    void print();
private:
    void wait_for_resume();
    IPrintStats& print_stats_;
    IIsTty& is_tty_;
    ITryReadByte& try_read_byte_;
    IIdle& idle_;
};

template<typename IPS, typename IIT, typename ITRB, typename II>
pause_poller<IPS, IIT, ITRB, II>::pause_poller(
    IPS& print_stats, IIT& is_tty, ITRB& try_read_byte, II& idle)
    : print_stats_(print_stats)
    , is_tty_(is_tty)
    , try_read_byte_(try_read_byte)
    , idle_(idle)
{}

template<typename IPS, typename IIT, typename ITRB, typename II>
void pause_poller<IPS, IIT, ITRB, II>::print() {
    print_stats_.print();
    if (!is_tty_.is_tty()) return;
    const std::optional<char> byte = try_read_byte_.try_read_byte();
    const bool space_pending = byte.has_value() && *byte == ' ';
    if (!space_pending) return;
    wait_for_resume();
}

template<typename IPS, typename IIT, typename ITRB, typename II>
void pause_poller<IPS, IIT, ITRB, II>::wait_for_resume() {
    std::cout << " PAUSED" << std::flush;
    while (true) {
        idle_.idle();
        const std::optional<char> byte = try_read_byte_.try_read_byte();
        const bool space_pressed = byte.has_value() && *byte == ' ';
        if (space_pressed) return;
    }
}

#endif
