#ifndef JHMI_UNITS_HPP_NRC_20141204
#define JHMI_UNITS_HPP_NRC_20141204

#include "pt3.hpp"
#include <boost/units/base_units/angle/degree.hpp>
#include <boost/units/base_units/metric/hour.hpp>
#include <boost/units/base_units/metric/liter.hpp>
#include <boost/units/base_units/metric/minute.hpp>
#include <boost/units/base_units/metric/mmHg.hpp>
#include <boost/units/cmath.hpp>
#include <boost/units/io.hpp>
#include <boost/units/systems/cgs/mass.hpp>
#include <boost/units/systems/si.hpp>
#include <boost/units/systems/si/prefixes.hpp>
#include <boost/units/systems/si/io.hpp>

namespace jhmi {
  constexpr std::size_t operator "" _z ( unsigned long long n ) { return n; }
  //This is complicated since milli * meters can only be converted explicitly
  // to an si::length.  I think var * millimeters isn't surprising if stored
  // as a meter type, so I want to support it, but without hacking boost units.
  // Hence the following:
#define JHMI_PREFIX_TYPE(prefix_,constant_,unit_)                                  \
  struct prefix_##constant_##_type {};                                             \
  static constexpr prefix_##constant_##_type const prefix_##constant_{};           \
  constexpr inline unit_ operator*(double t, prefix_##constant_##_type)            \
  { return unit_{t * boost::units::si::prefix_ * boost::units::si::constant_}; }   \
  constexpr inline unit_ operator*(prefix_##constant_##_type, double t)            \
  { return unit_{t * boost::units::si::prefix_ * boost::units::si::constant_}; }

  //Activity
  using Bq = boost::units::quantity<boost::units::si::activity, double>;
  constexpr inline Bq operator "" _Bq(unsigned long long n)
  { return Bq{static_cast<double>(n) * boost::units::si::becquerels}; }
  constexpr inline Bq operator "" _Bq(long double n)
  { return Bq{static_cast<double>(n) * boost::units::si::becquerels}; }
  constexpr inline Bq operator "" _kBq(unsigned long long n)
  { return Bq{static_cast<double>(n) * boost::units::si::kilo * boost::units::si::becquerels}; }
  constexpr inline Bq operator "" _kBq(long double n)
  { return Bq{static_cast<double>(n) * boost::units::si::kilo * boost::units::si::becquerels}; }
  BOOST_UNITS_STATIC_CONSTANT(becquerels,boost::units::si::activity);
  JHMI_PREFIX_TYPE(kilo,becquerels,Bq)
  static constexpr auto const kBq = kilobecquerels;
  JHMI_PREFIX_TYPE(giga,becquerels,Bq)
  static constexpr auto const GBq = gigabecquerels;

  //Angle
  using rad = boost::units::quantity<boost::units::si::plane_angle, double>;
  BOOST_UNITS_STATIC_CONSTANT(radians, boost::units::si::plane_angle);
  BOOST_UNITS_STATIC_CONSTANT(degrees, boost::units::angle::degree_base_unit::unit_type);
  constexpr inline rad operator "" _radians(unsigned long long n)
  { return static_cast<double>(n) * radians; }
  constexpr inline rad operator "" _radians(long double n)
  { return static_cast<double>(n) * radians; }
  constexpr inline rad operator "" _degrees(unsigned long long n)
  { return rad{static_cast<double>(n) * degrees}; }
  constexpr inline rad operator "" _degrees(long double n)
  { return rad{static_cast<double>(n) * degrees}; }

  //Time
  using s = boost::units::quantity<boost::units::si::time, double>;
  BOOST_UNITS_STATIC_CONSTANT(seconds, boost::units::si::time);
  BOOST_UNITS_STATIC_CONSTANT(minutes, boost::units::metric::minute_base_unit::unit_type);
  BOOST_UNITS_STATIC_CONSTANT(hours, boost::units::metric::hour_base_unit::unit_type);

  //Distance
  using m = boost::units::quantity<boost::units::si::length, double>;
  using m3 = pt3<m>;
  constexpr inline m operator""_mm(unsigned long long n)
  { return m{static_cast<double>(n) * boost::units::si::milli * boost::units::si::meters}; }
  constexpr inline m operator""_mm(long double n)
  { return m{static_cast<double>(n) * boost::units::si::milli * boost::units::si::meters}; }
  constexpr inline m operator""_um(unsigned long long n)
  { return m{static_cast<double>(n) * boost::units::si::micro * boost::units::si::meters}; }
  constexpr inline m operator""_um(long double n)
  { return m{static_cast<double>(n) * boost::units::si::micro * boost::units::si::meters}; }
  BOOST_UNITS_STATIC_CONSTANT(meters,boost::units::si::length);
  JHMI_PREFIX_TYPE(milli,meters,m)
  static constexpr auto const mm = millimeters;
  JHMI_PREFIX_TYPE(micro,meters,m)
  static constexpr auto const um = micrometers;

  //Mass
  using kg = boost::units::quantity<boost::units::si::mass, double>;

  //Area
  using square_meters = boost::units::quantity<boost::units::si::area, double>;

  //Volume
  using cubic_meters = boost::units::quantity<boost::units::si::volume, double>;
  using milliliter = boost::units::make_scaled_unit<
    boost::units::metric::liter_base_unit::unit_type,
    boost::units::scale<10, boost::units::static_rational<-3>>>::type;
  BOOST_UNITS_STATIC_CONSTANT(mL, milliliter);

  //Flow
  using flow_dimension = boost::units::derived_dimension<
          boost::units::length_base_dimension,3, boost::units::time_base_dimension,-1>::type;
  using flow = boost::units::unit<flow_dimension, boost::units::si::system>;
  using cubic_meters_per_second = boost::units::quantity<flow, double>;

  //density
  using density = boost::units::unit<boost::units::mass_density_dimension, boost::units::si::system>;
  using kg_per_cubic_meter = boost::units::quantity<density, double>;

  //Dose
  using Gy = boost::units::quantity<boost::units::si::absorbed_dose, double>;
  constexpr inline Gy operator""_Gy(long double n)
  { return Gy{static_cast<double>(n) * boost::units::si::grays}; }
  constexpr inline Gy operator""_Gy(unsigned long long n)
  { return Gy{static_cast<double>(n) * boost::units::si::grays}; }
  BOOST_UNITS_STATIC_CONSTANT(grays,boost::units::si::absorbed_dose);

  //Pressure
  using Pa = boost::units::quantity<boost::units::si::pressure, double>;
  BOOST_UNITS_STATIC_CONSTANT(pascals,boost::units::si::pressure);
  BOOST_UNITS_STATIC_CONSTANT(mmHg,boost::units::metric::mmHg_base_unit::unit_type);
  constexpr inline Pa operator""_mmHg(unsigned long long n)
  { return Pa{static_cast<double>(n) * mmHg}; }
  constexpr inline Pa operator""_mmHg(long double n)
  { return Pa{static_cast<double>(n) * mmHg}; }

  //Viscosity
  using Pa_s = boost::units::quantity<decltype(pascals * seconds), double>;

  //Energy
  using joules = boost::units::quantity<boost::units::si::energy,double>;
}

#endif
