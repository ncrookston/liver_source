#ifndef JHMI_LIVER_CONSTANTS_HPP_NRC_20150903
#define JHMI_LIVER_CONSTANTS_HPP_NRC_20150903

#include "utility/units.hpp"
#include <boost/math/constants/constants.hpp>

namespace jhmi {
//  using gamma = boost::units::static_rational<27, 10>::type;
//  using inv_gamma = boost::units::static_rational<gamma::Denominator, gamma::Numerator>::type;

  static double const pi = boost::math::double_constants::pi;
  static Pa const input_pressure = 98_mmHg;//1999 Bezy-Wendling
#if 0
#if 0
  inline Pa_s blood_viscosity(m radius) {
    return std::min(4. * pascals * seconds, Pa_s::from_value(1.8 * .6913 * (220 * std::exp(-1.3 * 1e6 * 2. * radius.value())
      + 3.2 - 2.44 * std::exp(-.06 * std::pow(1e6 * 2. * radius.value(), .645)))));
  }
#else
  inline Pa_s blood_viscosity(m) {
    return 3.5e-3 * pascals * seconds;
  }
#endif
#endif
  static Pa_s const blood_viscosity = 3.5e-3 * pascals * seconds;//Fung
  //static Pa const cell_pressure = 25_mmHg;//volmar
  //static auto const proper_ha_flow = 400. * mL / minutes;//1999 Bezy-Wendling
}
#endif
