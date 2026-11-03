#ifndef PUD_LOAD_READOUT_HPP
#define PUD_LOAD_READOUT_HPP

#include <chrono>
#include <iostream>
#include <string>

// Announces a slow setup step and, when it is done, how long it took: "Loading module ...", then
// "Module loaded in 600ms". begin() hands back the start time so steps can nest. Quiet prints nothing.
template<typename INow>
struct pud_load_readout {
    using time_point = typename INow::time_point;
    pud_load_readout(INow& now, bool quiet);
    time_point begin(const std::string& activity);
    void end(const std::string& outcome, time_point started);
private:
    INow& now_;
    bool quiet_;
};

template<typename INow>
pud_load_readout<INow>::pud_load_readout(INow& now, bool quiet)
    : now_(now)
    , quiet_(quiet) {}

template<typename INow>
typename pud_load_readout<INow>::time_point pud_load_readout<INow>::begin(const std::string& activity) {
    if (!quiet_)
        std::cout << activity << "...\n" << std::flush;
    return now_.now();
}

template<typename INow>
void pud_load_readout<INow>::end(const std::string& outcome, time_point started) {
    if (quiet_)
        return;
    const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now_.now() - started).count();
    std::cout << outcome << " in " << milliseconds << "ms\n" << std::flush;
}

#endif
