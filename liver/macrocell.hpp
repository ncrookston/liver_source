#ifndef JHMI_MACROCELL_HPP_NRC20150429
#define JHMI_MACROCELL_HPP_NRC20150429

#include "liver/utility.hpp"
#include "utility/binary_tree.hpp"
#include "utility/bresenham.hpp"
#include "utility/cube.hpp"
#include "utility/units.hpp"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <range/v3/getlines.hpp>
#include <range/v3/range_for.hpp>
#include <fstream>

namespace jhmi {
  template <typename T> using cidx_to = std::unordered_map<cell_id, T>;
  enum class cell_type { normal, tumor };
  struct macrocell {
    m3 center;
    vessel_id parent_vessel;
    cell_type type;
    m radius;
    cell_id id;
    cubic_meters_per_second flow;
    Pa pressure;
    int3 idx;
  };

  cell_id id(macrocell const& m) {
    return m.id;
  }
  cube<m3> extents(macrocell const& m) {
    return {m.center - m.radius * dbl3{1,1,1}, m.center + m.radius * dbl3{1,1,1}};
  }
  template <typename Convert, typename F>
  inline F traverse(macrocell const& m, Convert c, F f) {
    auto u = extents(m);
    return closed_traverse(cube<decltype(c(u.ul()))>{c(u.ul()), c(u.lr())}, 1, f);
  }

  bool operator==(macrocell const& lhs, macrocell const& rhs) {
    return nearly_equal(lhs.center, rhs.center)
     && lhs.parent_vessel == rhs.parent_vessel
     && lhs.type == rhs.type
     && nearly_equal(lhs.radius, rhs.radius)
     && lhs.id == rhs.id
     && nearly_equal(lhs.flow, rhs.flow)
     && nearly_equal(lhs.pressure, rhs.pressure)
     && lhs.idx == rhs.idx;
  }
}//jhmi

#endif
