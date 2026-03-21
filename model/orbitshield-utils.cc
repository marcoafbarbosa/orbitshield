/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#include "orbitshield-utils.h"

#include <iomanip>
#include <sstream>

namespace ns3
{

std::string
JulianDateToString(const perturb::JulianDate& jd)
{
    auto dt = jd.to_datetime();
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(4) << dt.year << "-"
        << std::setw(2) << (int)dt.month << "-"
        << std::setw(2) << (int)dt.day << " "
        << std::setw(2) << (int)dt.hour << ":"
        << std::setw(2) << (int)dt.min << ":"
        << std::fixed << std::setprecision(4) << dt.sec;
    return oss.str();
}

} // namespace ns3