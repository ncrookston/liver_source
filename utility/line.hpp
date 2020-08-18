#ifndef JHMI_UTILITY_LINE_HPP_NRC_20150804
#define JHMI_UTILITY_LINE_HPP_NRC_20150804

#include "utility/cube.hpp"
#include "utility/pt3.hpp"

namespace jhmi {

  template <typename Pt> struct line {
    Pt p1, p2;
  };

  template <typename Pt> auto nearest_t(line<Pt> const& l, Pt pt) {
    return dot(pt - l.p1, l.p2 - l.p1) / distance_squared(l.p1, l.p2);
  }
  template <typename Pt, typename T> Pt lerp(line<Pt> const& l, T t) {
    if (t < 0) return l.p1;
    if (t > 1) return l.p2;
    return l.p1 + t * (l.p2 - l.p1);
  }
  template <typename Pt> auto distance_squared(line<Pt> const& l, Pt pt) {
    auto t = nearest_t(l, pt);
    return distance_squared(lerp(l, t), pt);
  }
}
#endif
