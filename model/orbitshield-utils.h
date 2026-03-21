/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#ifndef ORBITSHIELD_UTILS_H
#define ORBITSHIELD_UTILS_H

#include <perturb/perturb.hpp>
#include <string>

namespace ns3
{

/**
 * \brief Convert a JulianDate to a formatted string.
 *
 * \param jd the JulianDate to convert
 * \return a string representation in "YYYY-MM-DD HH:MM:SS.ssss" format
 */
std::string JulianDateToString(const perturb::JulianDate& jd);

} // namespace ns3

#endif /* ORBITSHIELD_UTILS_H */