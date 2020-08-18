#ifndef JHMI_PT3_HPP_NRC_20141031
#define JHMI_PT3_HPP_NRC_20141031

#include <boost/functional/hash.hpp>
#include <boost/units/units_fwd.hpp>
#include <cmath>
#include <iostream>
#include <type_traits>

namespace jhmi {
  namespace jhmi_detail {
    template <typename Src, typename Dst>
    struct is_lossy_convertible
    {
      static const bool value =  (std::is_integral<Dst>::value && !std::is_integral<Src>::value)
        || (std::is_integral<Src>::value && !std::is_integral<Dst>::value
            && sizeof(Src) == sizeof(Dst))
        || sizeof(Src) > sizeof(Dst);
    };

    template <typename Src, typename Dst>
    struct is_non_narrowing_conversion
    {
      static const bool value = std::is_convertible<Src,Dst>::value
        && !((std::is_arithmetic<Src>::value && std::is_arithmetic<Dst>::value)
             || is_lossy_convertible<Src,Dst>::value);
    };

  }//end jhmi_detail

  template <typename T> struct pt3
  {
    using type = T;
    constexpr pt3() : x(), y(), z() {}
    constexpr pt3(T x, T y, T z) : x(x),y(y),z(z) {}

    template <typename U>
    constexpr pt3(pt3<U> const& u, typename std::enable_if<
        jhmi_detail::is_non_narrowing_conversion<U,T>::value>::type* = nullptr)
      : x(u.x),
        y(u.y),
        z(u.z)
    {}
    template <typename U>
    constexpr explicit pt3(pt3<U> const& u, typename std::enable_if<
        !jhmi_detail::is_non_narrowing_conversion<U,T>::value>::type* = nullptr)
      : x(static_cast<T>(u.x)),
        y(static_cast<T>(u.y)),
        z(static_cast<T>(u.z))
    {}

    constexpr pt3<T>& operator+=(pt3<T> const& rhs)
    { x += rhs.x; y += rhs.y; z += rhs.z; return *this; }
    constexpr pt3<T>& operator-=(pt3<T> const& rhs)
    { x -= rhs.x; y -= rhs.y; z -= rhs.z; return *this; }

    T x, y, z;
  };
  template <typename T> constexpr auto operator+(pt3<T> const& pt)
  { return pt; }
  template <typename T> constexpr auto operator-(pt3<T> const& pt)
  { return pt3<T>{-pt.x, -pt.y, -pt.z}; }
  template <typename T, typename U> constexpr auto operator+(pt3<T> const& lhs, pt3<U> const& rhs)
  { return pt3<decltype(lhs.x+rhs.x)>{lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z}; }
  template <typename T, typename U> constexpr auto operator-(pt3<T> const& lhs, pt3<U> const& rhs)
  { return pt3<decltype(lhs.x-rhs.x)>{lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z}; }
  template <typename T, typename U> constexpr auto operator*(pt3<T> const& lhs, U rhs)
  { return pt3<decltype(lhs.x*rhs)>{lhs.x * rhs, lhs.y * rhs, lhs.z * rhs}; }
  template <typename T, typename U> constexpr auto operator*(U lhs, pt3<T> const& rhs)
  { return pt3<decltype(lhs*rhs.x)>{lhs * rhs.x, lhs * rhs.y, lhs * rhs.z}; }
  template <typename T, typename D, typename S> constexpr auto operator*(pt3<T> const& lhs, boost::units::unit<D,S> rhs)
  { return pt3<decltype(lhs.x*rhs)>{lhs.x * rhs, lhs.y * rhs, lhs.z * rhs}; }
  template <typename T, typename D, typename S> constexpr auto operator*(boost::units::unit<D,S> lhs, pt3<T> const& rhs)
  { return pt3<decltype(lhs*rhs.x)>{lhs * rhs.x, lhs * rhs.y, lhs * rhs.z}; }
  template <typename T, typename U> constexpr auto operator/(pt3<T> const& lhs, U rhs)
  { return pt3<decltype(lhs.x/rhs)>{lhs.x / rhs, lhs.y / rhs, lhs.z / rhs}; }
  template <typename T, typename D, typename S> constexpr auto operator/(pt3<T> const& lhs, boost::units::unit<D,S> rhs)
  { return pt3<decltype(lhs.x/rhs)>{lhs.x / rhs, lhs.y / rhs, lhs.z / rhs}; }
  template <typename T> constexpr auto operator==(pt3<T> const& lhs, pt3<T> const& rhs)
  { return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z; }
  template <typename T> constexpr auto operator!=(pt3<T> const& lhs, pt3<T> const& rhs)
  { return !(lhs == rhs); }
  template <typename T> constexpr bool operator<(pt3<T> const& lhs, pt3<T> const& rhs)
  { return lhs.x < rhs.x || (lhs.x == rhs.x && (lhs.y < rhs.y || (lhs.y == rhs.y && lhs.z < rhs.z))); }
  template <typename T> std::ostream& operator<<(std::ostream& out, pt3<T> const& pt)
  { out << '[' << pt.x << ',' << pt.y << ',' << pt.z << ']'; return out; }
  template <typename T> std::size_t hash_value(pt3<T> const& pt) {
    std::size_t seed = 0;
    boost::hash_combine(seed, pt.x);
    boost::hash_combine(seed, pt.y);
    boost::hash_combine(seed, pt.z);
    return seed;
  }

  template <typename T> constexpr auto abs(pt3<T> const& v)
  { using std::abs; return pt3<T>{abs(v.x), abs(v.y), abs(v.z)}; }
  template <typename T> constexpr auto ceil(pt3<T> const& v)
  { using std::ceil; return pt3<T>{ceil(v.x), ceil(v.y), ceil(v.z)}; }
  template <typename T> constexpr auto cross(pt3<T> const& lhs, pt3<T> const& rhs) {
    return pt3<T>{lhs.y * rhs.z - lhs.z * rhs.y,
                  lhs.z * rhs.x - lhs.x * rhs.z,
                  lhs.x * rhs.y - lhs.y * rhs.x};
  }
  //Overloaded since we're losing precision with normalizing code.
  template <typename T, typename U> constexpr auto cross(pt3<boost::units::quantity<T,U>> const& lhs, pt3<boost::units::quantity<T,U>> const& rhs) {
    return pt3<boost::units::quantity<T,U>>{
      boost::units::quantity<T,U>::from_value(lhs.y.value() * rhs.z.value() - lhs.z.value() * rhs.y.value()),
      boost::units::quantity<T,U>::from_value(lhs.z.value() * rhs.x.value() - lhs.x.value() * rhs.z.value()),
      boost::units::quantity<T,U>::from_value(lhs.x.value() * rhs.y.value() - lhs.y.value() * rhs.x.value())};
  }
  template <typename T> auto distance(pt3<T> const& pt)
  { using std::sqrt; return sqrt(distance_squared(pt)); }
  template <typename T> constexpr auto distance_squared(pt3<T> const& pt)
  { return pt.x * pt.x + pt.y * pt.y + pt.z * pt.z; }
  template <typename T> constexpr auto distance_squared(pt3<T> const& lhs, pt3<T> const& rhs)
  { return distance_squared(lhs - rhs); }
  template <typename T, typename U> constexpr auto dot(pt3<T> const& lhs, pt3<U> const& rhs)
  { return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z; }
  template <typename T, typename U> constexpr auto element_multiply(pt3<T> const& lhs, pt3<U> const& rhs) {
    return pt3<decltype(lhs.x * rhs.x)>{lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z};
  }
  template <typename T, typename U> constexpr auto element_divide(pt3<T> const& lhs, pt3<U> const& rhs) {
    return pt3<decltype(lhs.x / rhs.x)>{lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z};
  }
  template <typename T, typename U> constexpr auto element_modulus(pt3<T> const& lhs, pt3<U> const& rhs) {
    return pt3<decltype(lhs.x % rhs.x)>{lhs.x % rhs.x, lhs.y % rhs.y, lhs.z % rhs.z};
  }
  template <typename T> auto element_max(pt3<T> const& lhs, pt3<T> const& rhs) {
    return pt3<T>{std::max(lhs.x, rhs.x), std::max(lhs.y, rhs.y), std::max(lhs.z, rhs.z)};
  }
  template <typename T> auto element_min(pt3<T> const& lhs, pt3<T> const& rhs) {
    return pt3<T>{std::min(lhs.x, rhs.x), std::min(lhs.y, rhs.y), std::min(lhs.z, rhs.z)};
  }
  template <typename T> pt3<T> floor(pt3<T> const& pt)
  { using std::floor; return {floor(pt.x), floor(pt.y), floor(pt.z)}; }
  template <typename T> auto normalize(pt3<T> const& pt)
  { return pt / distance(pt); }
  template <typename T> auto round(pt3<T> const& pt)
  { using std::round; return pt3<T>{round(pt.x), round(pt.y), round(pt.z)}; }
  template <typename U, typename T> auto round_to(pt3<T> const& pt)
  { using std::lround; return pt3<U>(lround(pt.x), lround(pt.y), lround(pt.z)); }

  typedef pt3<int> int3;
  typedef pt3<float> flt3;
  typedef pt3<double> dbl3;
  typedef pt3<long double> ldb3;
}//jhmi
#endif
