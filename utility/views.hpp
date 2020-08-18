#ifndef JHMI_UTILITY_VIEWS_HPP_NRC_20160514
#define JHMI_UTILITY_VIEWS_HPP_NRC_20160514

#include <range/v3/view.hpp>

namespace jhmi { namespace view {
  template <typename T>
  auto step(T start, T stop, T step) {
    namespace rv = ranges::view;
    return rv::generate([=] () mutable -> T {
      auto old_start = start;
      start += step;
      return old_start;
    }) | rv::take_while([=](T val) { return val < stop; });
  }
}}

#endif
