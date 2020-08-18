#ifndef JHMI_LIVER_DISTANCE_VESSEL_HPP_NRC_2016_02_02
#define JHMI_LIVER_DISTANCE_VESSEL_HPP_NRC_2016_02_02

#include "liver/utility.hpp"
#include "liver/physical_vessel.hpp"
#include "utility/line.hpp"

namespace jhmi {
  using m_sq = decltype(jhmi::m{}*jhmi::m{});
  m_sq max_distsq = std::numeric_limits<double>::max() * meters * meters;
  struct distance_vessel {
    distance_vessel(vessel_id id, line<m3> const& l)
      : id(id), l(l), loff{l.p2 - l.p1}, ld(distance_squared(loff)),
        cell{cell_id::invalid()} {}
    distance_vessel(physical_vessel const& v) : distance_vessel{v.id(), v.line()} {}
    vessel_id id;
    line<m3> l;
    m3 loff;
    m_sq ld;
    cell_id cell;
  };
  auto id(distance_vessel const& sv) { return sv.id; }
  bool operator==(distance_vessel const& lhs, distance_vessel const& rhs) {
    return lhs.id == rhs.id;
  }
  bool operator<(distance_vessel const& lhs, distance_vessel const& rhs) {
    return lhs.id < rhs.id;
  }
  cube<m3> extents(distance_vessel const& sv) { return {sv.l.p1, sv.l.p2}; }
  m_sq distance_squared(distance_vessel const& sv, m3 const& pt) {
#if 1
    auto t = dot(pt - sv.l.p1, sv.loff) / sv.ld;
    auto lpt = t < 0 ? sv.l.p1 : (t > 1 ? sv.l.p2 : sv.l.p1 + t * sv.loff);
    return distance_squared(lpt, pt);
#else
    return distance_squared(l, pt);
#endif
  }
}//jhmi

namespace std {
  template <> struct hash<jhmi::distance_vessel> {
    size_t operator()(jhmi::distance_vessel const& sv) {
      return std::hash<jhmi::vessel_id>{}(sv.id);
    }
  };
}//std

#endif
