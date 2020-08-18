#ifndef JHMI_CUBE_HPP_NRC_20141121
#define JHMI_CUBE_HPP_NRC_20141121

#include "units.hpp"
namespace jhmi
{
/** An axis-aligned cube. */
template <typename Pt>
struct cube {
  cube() : ul_(), lr_() {}

  cube(Pt const& lhs, Pt const& rhs)
    : ul_{std::min(lhs.x, rhs.x), std::min(lhs.y, rhs.y), std::min(lhs.z, rhs.z)},
      lr_{std::max(lhs.x, rhs.x), std::max(lhs.y, rhs.y), std::max(lhs.z, rhs.z)}
  {}
  Pt const& ul() const { return ul_; }
  Pt const& lr() const { return lr_; }
private:
  Pt ul_, lr_;
};

template <typename Pt1, typename Pt2>
auto operator-(cube<Pt1> const& c, Pt2 const& p) {
  return cube<decltype(c.ul() + p)>{c.ul() - p, c.lr() - p};
}
template <typename Pt1, typename Pt2>
auto operator+(cube<Pt1> const& c, Pt2 const& p) {
  return cube<decltype(c.ul() + p)>{c.ul() + p, c.lr() + p};
}
template <typename Pt1, typename Pt2>
auto operator+(Pt2 const& p, cube<Pt1> const& c) { return c + p; }

template <typename Pt>
auto dimensions(cube<Pt> const& c) { return c.lr() - c.ul(); }

template <typename Pt>
auto width(cube<Pt> const& c) { return c.lr().x - c.ul().x; }
template <typename Pt>
auto height(cube<Pt> const& c) { return c.lr().y - c.ul().y; }
template <typename Pt>
auto depth(cube<Pt> const& c) { return c.lr().z - c.ul().z; }

template <typename Pt>
inline cube<Pt> expand(cube<Pt> const& c, Pt const& pt) {
  return {cube<Pt>{c.ul(),pt}.ul(), cube<Pt>{c.lr(),pt}.lr()};
}
template <typename Pt>
inline bool contains(cube<Pt> const& c, Pt const& pt) {
  return pt.x >= c.ul().x && pt.y >= c.ul().y && pt.z >= c.ul().z
      && pt.x <=  c.lr().x && pt.y <=  c.lr().y && pt.z <=  c.lr().z;
}
template <typename Pt>
inline bool contains(cube<Pt> const& lhs, cube<Pt> const& rhs) {
  return rhs.ul().x >= lhs.ul().x && rhs.ul().y >= lhs.ul().y && rhs.ul().z >= lhs.ul().z
      && rhs.lr().x <=  lhs.lr().x && rhs.lr().y <=  lhs.lr().y && rhs.lr().z <=  lhs.lr().z;
}
template <typename Pt>
inline cube<Pt> inflate(cube<Pt> const& c, Pt const& pt) {
  return {c.ul() - pt, c.lr() + pt};
}
template <typename Pt>
inline cube<Pt> unite(cube<Pt> const& lhs, cube<Pt> const& rhs) {
  return {element_min(lhs.ul(), rhs.ul()), element_max(lhs.lr(), rhs.lr())};
}
template <typename Pt>
inline cube<Pt> intersect(cube<Pt> const& lhs, cube<Pt> const& rhs) {
  auto ul = element_max(lhs.ul(), rhs.ul());
  auto lr = element_min(lhs.lr(), rhs.lr());
  if (ul.x >= lr.x || ul.y >= lr.y || ul.z >= lr.z)
    return cube<Pt>{ul,ul};
  return cube<Pt>{ul, lr};
}
template <typename Pt>
inline bool overlaps(cube<Pt> const& lhs, cube<Pt> const& rhs) {
  auto i = intersect(lhs, rhs);
  return i.ul() != i.lr();
}
template <typename Pt>
inline cube<Pt> extents(cube<Pt> const& c)
{ return c; }
template <typename Pt>
inline Pt center(cube<Pt> const& c) {
  return (c.ul() + c.lr()) / 2.;
}
template <typename Pt, typename T, typename F>
inline F traverse(cube<Pt> const& c, T t, F f) {
  for (auto z = c.ul().z; z < c.lr().z; z += t)
    for (auto y = c.ul().y; y < c.lr().y; y += t)
      for (auto x = c.ul().x; x < c.lr().x; x += t)
        f(Pt{x,y,z});
  return f;
}
template <typename Pt, typename T, typename F>
inline F closed_traverse(cube<Pt> const& c, T t, F f) {
  return traverse(cube<Pt>{c.ul(), c.lr() + Pt{t,t,t}}, t, f);
}

}//jhmi

#endif
