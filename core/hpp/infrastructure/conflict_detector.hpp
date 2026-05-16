#ifndef CONFLICT_DETECTOR_HPP
#define CONFLICT_DETECTOR_HPP

#include "../interfaces/i_conflict_detector.hpp"

struct conflict_detector : i_conflict_detector {
    bool detect(const goal&) override;
};

#endif
