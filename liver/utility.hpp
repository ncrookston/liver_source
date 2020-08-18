#ifndef JHMI_LIVER_UTILITY_HPP_NRC_20161011
#define JHMI_LIVER_UTILITY_HPP_NRC_20161011

#include "utility/pt3.hpp"
#include "utility/tagged_int.hpp"
#include <fmt/ostream.h>
#include <unordered_map>

namespace jhmi {
  struct load_tree_tag {};
  struct build_tree_tag {};
  static const constexpr load_tree_tag load_tree{};
  static const constexpr build_tree_tag build_tree{};

  struct cell_tag {};
  struct vessel_tag {};

  using cell_id = tagged_int<cell_tag>;
  using vessel_id = tagged_int<vessel_tag>;

  template <typename T> using vidx_to = std::unordered_map<vessel_id, T>;
  template <typename T> bool nearly_equal(T lhs, T rhs) {
    return abs(lhs - rhs).value() < 1e-6;
  }
  template <typename T> bool nearly_equal(pt3<T> const& lhs, pt3<T> const& rhs) {
    return nearly_equal(lhs.x, rhs.x) && nearly_equal(lhs.y, rhs.y) && nearly_equal(lhs.z, rhs.z);
  }
  template <typename T> bool check_close(T lhs, T rhs, double eps, vessel_id id,
          std::string const& name1, std::string const& name2) {
    double diff = std::abs(2. * (lhs - rhs) / (lhs + rhs));
    if (diff > eps) {
      fmt::print("ID {} Mismatch between {} ({}) and {} ({}), difference {: e}\n",
        id, name1, lhs, name2, rhs, diff);
      return false;
    }
    return true;
  }
}

#endif
